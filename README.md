# Guitar Effects Processor

A real-time guitar effects processor built using a **prototype → optimise → embed** workflow. Features a Python simulation layer with 13 audio effects, with selected effects ported to optimized C running on an STM32F407 microcontroller at 48 kHz.

---

## Repository Structure

```
effects/          # Python effect implementations
effects_c/        # C implementations (shared library + STM32)
EffectsUnit/      # STM32 target project (STM32F407)
tests/            # Unity unittests, C validation via Python ctypes
build.py          # MSVC compilation script for cext.dll
```

---

## Effects Layer (Python)

All effects inherit from a common `AudioEffect` base class (`process`, `to_dict`, `load_params`) and operate on NumPy arrays.

| Effect | Class | Key Parameters |
|---|---|---|
| **Overdrive** | `Overdrive` | `gain`, `function` (hard, soft, tanh, atan), `oversampling_factor` |
| **Bit Crusher** | `BitCrusher` | `bit_depth`, `downsample_factor`, `mix_parameter` |
| **Parametric EQ** | `EqualiserFFT` | `points` — list of (Hz, dB) control points |
| **Chorus / Flanger**| `Chorus` / `Flanger` | `rate_hz`, `depth_ms`, `base_delay_ms`, `feedback`, `mix` |
| **Delay / Echo** | `Delay` | `delay_len_ms`, `delay_feedback`, `mix_parameter` |
| **Dynamics** | `Compressor` / `NoiseGate`| `threshold`, `ratio`, `attack`, `hold`, `release`, `detection` (peak/RMS) |
| **Modulation** | `Tremolo` / `RingModulator`| `rate_hz`, `depth`, `waveform` (sine, square, sawtooth) |
| **Pitch Shifter** | `PitchShifter` | `semitones`, `frame_size`, `hop_size`, `window` |
| **Auto-Wah** | `Auto_wah` | `fmin`, `fmax`, `q`, `sensitivity`, `speed_ms` |
| **Reverb** | `ReverbEffect` | `comb_delays`, `allpass_delays`, `feedback`, `mix_parameter` |

### DSP Highlights
* **Overdrive:** 4× oversampling with a Butterworth anti-aliasing filter to suppress non-linear aliasing.
* **PitchShifter:** Phase vocoder using successive analysis frames, peak-shifting, and overlap-add synthesis.
* **Auto-Wah:** Sample-by-sample State-Variable Filter (HP/BP/LP) driven by an envelope follower.
* **EqualiserFFT:** Arbitrary control points mapped via cubic spline interpolation in log-frequency space.

---

## Optimized C Layer

Selected effects are ported to pure C, sharing the exact same source files between Windows (`cext.dll`) and the STM32 hardware target.

* **Overdrive (`overdrive.c`):** Inline hard/soft clipping with a Direct Form II transposed biquad anti-aliasing filter.
* **Bit Crusher (`bitcrusher.c`):** Quantization and downsampling with TPDF dither generated via an `xorshift32` PRNG.
* **Parametric EQ (`equaliser.c`):** Up to 8 cascaded biquad sections (Lowpass, Highpass, Peaking, Shelving) using Audio EQ Cookbook formulas.

### Unity C unittests
Effects written in C are tested via Unity framework (`tests/test_bitcrusher.c`, `tests/test_equaliser.c`, `tests/test_overdrive.c`).

### Python ↔ C Validation
The `tests/tests.py` script loads `cext.dll` via `ctypes` to verify the C implementation against the Python prototype (including impulse response and FFT analysis) before flashing to hardware.


---

## Embedded Target (STM32F407)

### Hardware Setup
* **MCU:** STM32F407 Discovery (168 MHz Cortex-M4F, Hardware FPU).
* **Audio Codec:** CS43L22 driven via I2S3 at 48 kHz / 16-bit.
* **Input:** ADC1 (12-bit), DMA-driven, synchronized via TIM3.

### Audio Pipeline
Uses double-buffered I2S DMA. Latency is **~5.3 ms** at 48 kHz (512-sample buffer, 256 per half-transfer).

```
[ADC Input] → U16 to Float32 → [Effect Chain Processing] → Float32 to I2S Int16 → [DAC Output]
```

The dynamic processing chain (`audio_process.c`) manages a static array of up to 8 effect slots using function pointers and compile-time configuration structs.

---

## Getting Started

### Dependencies
* **Python:** `numpy`, `scipy`, `soundfile`, `sounddevice`
* **C/Embedded:** MSVC (for Windows DLL), `arm-none-eabi-gcc`, STM32CubeF4 HAL

### Building the C Library (Windows)
Run from a Developer Command Prompt:
```bat
python build.py
```



