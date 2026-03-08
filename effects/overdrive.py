import numpy as np
from audioeffect import AudioEffect
from typing import Callable
from scipy import signal


class Overdrive(AudioEffect):
    def __init__(self, **kwargs):
        self.gain  = kwargs.get("gain", 5.0)
        self.functions = {
            "hard": lambda x: np.clip(x, -1.0, 1.0),
            "tanh": np.tanh,
            "atan": lambda x: (2/np.pi) * np.arctan(x),
            "soft": lambda x: np.where(np.abs(x) < 1, x - (x**3 / 3), np.sign(x) * (2/3))
        }
        fname = kwargs.get("function", "tanh")
        self.function = self.functions.get(fname, self.functions["tanh"])
        self.oversampling_factor = kwargs.get("oversampling_factor", 4)

    
    def add_function(self, name: str, fun: Callable[[float], float]) -> None:
        self.functions[name] = fun

    def select_function(self, name: str) -> None:
        if name not in self.functions:
            raise KeyError("Unknown overdrive function")
        else:
            self.function = self.functions[name]

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        num_samples = len(input_data) * self.oversampling_factor
        upsampled = signal.resample(input_data, num_samples)
        
        amplified = upsampled * self.gain
        distorted = self.function(amplified)
        
        sos = signal.butter(4, 10000, 'lp', fs=sample_rate * self.oversampling_factor, output='sos')
        filtered = signal.sosfilt(sos, distorted)
        
        output = signal.resample(filtered, len(input_data))
        return output



def main():
    import soundfile as sf
    import sounddevice as sd
    path = "effects/pianoman_mono.wav"
    
    try:
        data, fs = sf.read(path)
    except Exception as e:
        print(f"Error: {e}")
        return
    if len(data.shape) > 1:
        data = data[:, 0]
    ovd = Overdrive(gain=100.0, function="hard")

    #sd.play(data, fs)
    #sd.wait()

    processed = ovd.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()