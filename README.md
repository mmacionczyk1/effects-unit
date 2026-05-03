# Guitar Effects Processor

A real-time guitar effects processor built in two stages: a Python prototype layer implementing 13 audio effects, and a C/embedded layer porting selected effects to run on an STM32F407 microcontroller at 48 kHz.

The project follows a deliberate prototype → optimise → embed workflow: each effect is first written in Python (NumPy/SciPy) operating on whole audio files, then reimplemented in C, and finally integrated into a double-buffered DMA audio pipeline on hardware.

---

## Repository structure

```
effects/          # Python effect implementations
effects_c/        # C implementations (shared library + STM32)
EffectsUnit/      # STM32 project (STM32F407)
tests/            # C tests via ctypes
build.py          # MSVC build script → cext.dll
```

---

## Effects — Python

All effects inherit from a common `AudioEffect` base class exposing three methods: `process(data, sample_rate)`, `to_dict()`, and `load_params(params)`. Every effect operates on a NumPy array and returns a NumPy array of the same length.

| Effect | Class | Key parameters |
|---|---|---|
| Overdrive / Distortion | `Overdrive` | `gain`, `function` {hard, soft, tanh, atan}, `oversampling_factor` |
| Bit Crusher | `BitCrusher` | `bit_depth`, `downsample_factor`, `mix_parameter` |
| Parametric EQ (FFT) | `EqualiserFFT` | `points` — list of (Hz, dB) control points |
| Chorus | `Chorus` | `rate_hz`, `depth_ms`, `base_delay_ms`, `mix_parameter` |
| Flanger | `Flanger` | `rate_hz`, `depth_ms`, `base_delay_ms`, `feedback`, `mix_parameter` |
| Delay / Echo | `Delay` | `delay_len_ms`, `delay_feedback`, `mix_parameter` |
| Compressor | `Compressor` | `threshold`, `ratio`, `type` {downward, upward}, `detection` {peak, rms}, `attack`, `hold`, `release` |
| Noise Gate | `NoiseGate` | `threshold_high`, `threshold_low`, `detection`, `attack`, `hold`, `release` |
| Tremolo | `Tremolo` | `rate_hz`, `depth` |
| Pitch Shifter | `PitchShifter` | `semitones`, `frame_size`, `hop_size`, `window` |
| Auto-Wah | `Auto_wah` | `fmin`, `fmax`, `q`, `sensitivity`, `speed_ms` |
| Reverb | `ReverbEffect` | `comb_delays`, `allpass_delays`, `feedback`, `apf_feedback`, `mix_parameter` |
| Ring Modulator | `RingModulator` | `frequency`, `waveform` {sine, square, sawtooth}, `mix_parameter` |

### Notable implementations

**Overdrive** — Applies one of four soft-clipping transfer functions (hard clip, cubic soft clip, tanh, arctan) after gain. Uses 4× oversampling with a Butterworth anti-aliasing filter before downsampling to suppress aliasing products introduced by the nonlinearity.

**EqualiserFFT** — Accepts arbitrary (frequency, gain_dB) control points. Performs cubic spline interpolation in log-frequency space, then applies the resulting gain curve onto input signal. 

**PitchShifter** — Phase vocoder. Computes per-bin true instantaneous frequency from successive analysis frames using phase differences, shifts the spectral peaks to the target bin positions by the pitch ratio, and reconstructs via overlap-add synthesis.

**Auto-Wah** — State-variable filter with envelope follower. The envelope of the input drives the cutoff frequency of the bandpass output between `fmin` and `fmax`. The topology (HP → BP → LP decomposition) runs sample-by-sample with no allocations.

**ReverbEffect** — Schroeder reverb: four parallel comb filters (prime-number delays in ms to avoid beating) followed by two series allpass diffusers. All delays convert to integer samples at runtime.

**Compressor / NoiseGate** — Both use exponential attack/release envelopes with a configurable hold stage. Level detection supports peak or RMS modes. The compressor implements both downward (gain reduction above threshold) and upward (gain increase below threshold) compression.

---

## Effects — C

Three effects have been ported to C and compiled into a shared library (`cext.dll` on Windows). The same source files are also compiled for the STM32 target without modification.

### Overdrive (`effects_c/overdrive.c`)

Hard and soft clipping with a Direct Form II transposed biquad anti-aliasing filter applied inline per sample. Filter coefficients are pre-computed for an 8 kHz Butterworth lowpass at 48 kHz. The `overdrive_config_t` struct holds gain, clipping mode, and block size.

```c
typedef struct {
    float gain;
    driving_functions_e driving_function; // DRIVE_SOFT | DRIVE_HARD
    uint32_t sample_size;
} overdrive_config_t;

void overdrive_process(overdrive_config_t* cfg, float* input, float* output);
```

### Bit Crusher (`effects_c/bitcrusher.c`)

Quantises to a configurable bit depth and holds each quantised value for `downsample_factor` samples. Triangular probability density function  dither is added before quantisation using two samples from an xorshift32 PRNG. State (counter, held sample, RNG state) is persisted across blocks in the config struct for seamless block-by-block processing.

### Parametric EQ (`effects_c/equaliser.c`)

Up to `EQ_MAX_BANDS` (default 8) independent biquad sections, each independently enableable. Supports five filter types — lowpass, highpass, peaking, low shelf, high shelf — with coefficients derived from the Audio EQ Cookbook formulas. Bands can be set, enabled, and disabled at runtime.

```c
void eq_set_band(equaliser_t* eq, uint32_t index,
                 eq_type_t type, float f0, float Q, float gain_db);
void eq_enable_band(equaliser_t* eq, uint32_t index);
void eq_process(equaliser_t* eq, float* x, float* y);
```

### Python ↔ C bridge

`tests/tests.py` loads `cext.dll` via `ctypes`, mirrors every C struct as a `ctypes.Structure`, and calls the effect functions directly. This lets the C implementation be tested and plotted (impulse response → FFT → dB plot) from Python without any wrapper code generation.

`build.py` drives the MSVC toolchain to compile `effects_c/` into the DLL, writing intermediate `.obj` files to `build/` and the final library to `effects/cext.dll`.

---

## Embedded — STM32F407

### Hardware

- **MCU:** STM32F407 (168 MHz Cortex-M4F, single-precision FPU)  
- **DAC:** CS43L22 stereo audio codec, driven over I2S3 at 48 kHz / 16-bit  
- **ADC:** ADC1, 12-bit, DMA, triggered by TIM3 (synchronised with I2S) at 48 kHz — used for analog input  
- **Peripherals:** I2C1 (CS43L22 control)

### Audio pipeline

The pipeline uses double buffering. The STM32 HAL I²S DMA fires two callbacks per period:

```
HAL_I2S_TxHalfCpltCallback  →  audio_pipeline(0)  // process first half
HAL_I2S_TxCpltCallback      →  audio_pipeline(1)  // process second half
```

Inside `audio_pipeline()`:

```
ADC DMA buffer (uint16)
    → convert_adc_u16f_buf()     12-bit unsigned → normalised float [-1, 1]
    → run_effect_chain()         apply enabled effects in order
    → convert_fi16_stereo()      float → int16 interleaved stereo
    → I2S DMA output buffer      → CS43L22 DAC
```

Buffer size is 512 samples (256 per half), giving a pipeline latency of ~5.3 ms at 48 kHz. A GPIO pin (PC1) is toggled around `run_effect_chain()` for oscilloscope CPU-load measurement.

### Effect chain

`audio_process.c` manages a static array of up to 8 effect slots. Each slot holds a function pointer, a `set_size` callback (called before each block to update the block length field in the config struct), and a `void*` config pointer. Effects are added and enabled at startup:

```c
effect_chain_add(equaliser_wrapper, equaliser_set_size, &eq);
effect_chain_add(overdrive_wrapper, overdrive_set_size, &od_cfg);
effect_chain_add(bitcrusher_wrapper, bitcrusher_set_size, &btc_cfg);
```

Slots can be individually enabled or disabled at runtime via `effect_chain_set_enabled()`.

---

## Dependencies

**Python**
- `numpy`, `scipy` — signal processing
- `soundfile` — WAV I/O
- `sounddevice` — audio playback

**C / Embedded**
- MSVC (Windows) — DLL build
- STM32 HAL (STM32CubeF4) — peripheral drivers
- arm-none-eabi-gcc — embedded target

---

## Building the C library (Windows)

```bat
python build.py
```

Requires MSVC (`cl.exe`) on PATH (e.g. via a Developer Command Prompt). Produces `effects/cext.dll`.

---

## Roadmap / WIP

**Porting remaining effects to C**  
The chorus, flanger, delay, compressor, noise gate, tremolo, auto-wah, reverb, ring modulator, and pitch shifter are currently Python-only. The plan is to port them to C using the same config-struct + function-pointer pattern already established for overdrive, bitcrusher, and equaliser — keeping the implementations usable both as a desktop shared library and on the STM32 target without modification.

**Real-time parameter control and USB audio**  
Currently effect parameters are set at compile time. The plan is to implement a USB CDC (virtual serial port) protocol allowing parameters to be changed on the fly. A further option is USB Audio Class, which would expose the device as a standard USB audio interface, removing the need for a separate analog input path

**Custom PCB** *(long-term)*  
Once the firmware is stable, replacing the Discovery board with a dedicated PCB — proper input/output conditioning, a jack connector, a hardware bypass relay, and a compact form factor. 

---

## License

MIT