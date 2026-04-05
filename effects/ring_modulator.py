import numpy as np
import math
from scipy import signal
from audioeffect import AudioEffect

class RingModulator(AudioEffect):
    def __init__(self, **kwargs):
        self.frequency = kwargs.get('frequency', 500.0)
        self.waveform = kwargs.get("waveform", "sine")
        if self.waveform not in ["sine", "square", "sawtooth"]:
            raise KeyError("Unknown waveform type")
        self.mix_parameter = kwargs.get('mix_parameter', 0.5)
        self.phase = 0.0

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        output = np.zeros(len(input_data))
        delta_phase = 2 * np.pi * self.frequency / sample_rate
        for i in range(len(input_data)):
            if self.waveform == "sine":
                mod_signal = math.sin(self.phase)
            elif self.waveform == "square":
                mod_signal = signal.square(self.phase)
            elif self.waveform == "sawtooth":
                mod_signal = signal.sawtooth(self.phase)
            output[i] = (1 - self.mix_parameter) * input_data[i] + self.mix_parameter * input_data[i] * mod_signal
            self.phase = (self.phase + delta_phase) % (2 * math.pi)
        return output


def main():
    import soundfile as sf
    import sounddevice as sd
    path = "effects/barka_cut.wav"
    
    try:
        data, fs = sf.read(path)
    except Exception as e:
        print(f"Error: {e}")
        return
    if len(data.shape) > 1:
        data = data[:, 0]
    ring = RingModulator(
        frequency=800.0,
        waveform="sine",
        mix_parameter=1.0
    )

    #sd.play(data, fs)
    #sd.wait()

    processed = ring.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()