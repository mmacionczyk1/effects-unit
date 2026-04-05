import numpy as np
from audioeffect import AudioEffect
import math


class Auto_wah(AudioEffect):
    def __init__(self, **kwargs):
        self.fmin = kwargs.get("fmin", 300)
        self.fmax = kwargs.get("fmax",2500)
        self.q = 1/kwargs.get("q", 5.0)
        self.sensitivity = kwargs.get("sensitivity", 2.5)
        self.speed_ms = kwargs.get("speed_ms", 15.0)

        self.env_state = 0.0
        self.lp_state = 0.0
        self.bp_state = 0.0

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        output = np.zeros(len(input_data))
        alpha = np.exp(-1.0 / (self.speed_ms * sample_rate / 1000.0))
        for i in range(len(input_data)):
            env = alpha * self.env_state + (1 - alpha) * math.fabs(input_data[i]) * self.sensitivity
            self.env_state = min(1.0, env)
            midfreq = min(self.fmin + self.env_state * (self.fmax - self.fmin), sample_rate / 6)
            f = 2 * math.sin(math.pi * midfreq / sample_rate)

            hp = input_data[i] - self.lp_state - self.q * self.bp_state
            self.bp_state = self.bp_state + f * hp
            self.lp_state = self.lp_state + f * self.bp_state
            output[i] = self.bp_state
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
    wah = Auto_wah(fmin=300, fmax=5000, q=5.0, sensitivity=2.5, speed_ms=10.0)

    #sd.play(data, fs)
    #sd.wait()

    processed = wah.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()