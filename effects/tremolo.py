import numpy as np
from audioeffect import AudioEffect
import math

class Tremolo(AudioEffect):
    def __init__(self,  rate_hz: float = 1.5, depth: float = 0.5): #depth is between 0 and 1
        self.rate_hz = rate_hz
        self.depth = depth

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        N = len(input_data)
        output = np.zeros(N)
        
        lfo_phase_increment = 2.0 * math.pi * self.rate_hz / sample_rate
        
        for i in range(N):
            lfo_val = math.sin(lfo_phase_increment * i)
            output[i] = input_data[i] * (1.0 - self.depth + self.depth*lfo_val)

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
    ovd = Tremolo(rate_hz=0.5, depth=0.8)

    #sd.play(data, fs)
    #sd.wait()

    processed = ovd.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()