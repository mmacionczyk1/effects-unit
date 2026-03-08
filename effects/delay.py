import numpy as np
from audioeffect import AudioEffect
from typing import Callable
from scipy import signal

class Delay(AudioEffect):
    def __init__(self, delay_len_ms: float = 1.0, delay_feedback: float = 0.3, mix_parameter: float = 0.5):
        self.delay_len_s = delay_len_ms / 1000
        self.delay_feedback = delay_feedback
        self.mix_parameter = mix_parameter

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        N = len(input_data)
        feedback_loop = np.zeros(N)
        output = np.zeros(N)
        delay_len_samples = int(sample_rate * self.delay_len_s)
        if N < delay_len_samples:
            raise KeyError("Delay is too long")
    
        for i in range(0,N):
            if i<delay_len_samples:
                feedback_loop[i] = input_data[i]
                output[i] = input_data[i]
            else:
                feedback_loop[i] = input_data[i] + self.delay_feedback * feedback_loop[i - delay_len_samples]
                output[i] = (1 - self.mix_parameter) * input_data[i] + self.mix_parameter * feedback_loop[i - delay_len_samples]
        
        return output
    


def main():
    import soundfile as sf
    import sounddevice as sd
    path = "barka_cut.wav"
    
    try:
        data, fs = sf.read(path)
    except Exception as e:
        print(f"Error: {e}")
        return
    if len(data.shape) > 1:
        data = data[:, 0]
    ovd = Delay(delay_len_ms = 500.0, delay_feedback = 0.3, mix_parameter=0.1)

    #sd.play(data, fs)
    #sd.wait()

    processed = ovd.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()