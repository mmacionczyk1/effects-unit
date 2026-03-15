import numpy as np
import math
from audioeffect import AudioEffect

class Chorus(AudioEffect):
    def __init__(self, rate_hz: float = 1.5, depth_ms: float = 2.0, base_delay_ms: float = 20.0, mix_parameter: float = 0.5):
        self.rate_hz = rate_hz
        self.depth_ms = depth_ms
        self.base_delay_ms = base_delay_ms
        self.mix_parameter = mix_parameter

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        N = len(input_data)
        output = np.zeros(N)
        
        lfo_phase_increment = 2.0 * math.pi * self.rate_hz / sample_rate
        
        for i in range(N):
            lfo_val = math.sin(lfo_phase_increment * i)
            
            current_delay_ms = self.base_delay_ms + self.depth_ms * lfo_val
            delay_samples = current_delay_ms * sample_rate / 1000.0

            int_delay = int(delay_samples)
            frac_delay = delay_samples - int_delay
            
            idx1 = i - int_delay
            idx2 = i - int_delay - 1
            
            if idx2 >= 0:
                delayed_sample = (1.0 - frac_delay) * input_data[idx1] + frac_delay * input_data[idx2]
            elif idx1 >= 0:
                delayed_sample = input_data[idx1]
            else:
                delayed_sample = 0.0
            output[i] = (1.0 - self.mix_parameter) * input_data[i] + self.mix_parameter * delayed_sample
            
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
    ovd = Chorus(rate_hz=0.2, depth_ms=10.0, base_delay_ms=50.0, mix_parameter=0.5)

    #sd.play(data, fs)
    #sd.wait()

    processed = ovd.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
        main()