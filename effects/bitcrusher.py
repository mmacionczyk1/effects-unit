import numpy as np
from audioeffect import AudioEffect
import math

class BitCrusher(AudioEffect):
    def __init__(self, **kwargs):
        self.bit_depth = kwargs.get("bit_depth", 8)
        self.downsample_factor = kwargs.get("downsample_factor", 10)
        self.mix_parameter = kwargs.get("mix_parameter", 0.5)

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        output = np.zeros(len(input_data))
        steps = 2**(self.bit_depth - 1) - 1
        temp_sample = 0.0
        for i in range(len(input_data)):
            if i % self.downsample_factor == 0:
                temp_sample = round(input_data[i] * steps) / steps
            output[i] = (1 - self.mix_parameter) * input_data[i] + self.mix_parameter * temp_sample
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
    bit = BitCrusher(
        bit_depth=4,
        downsample_factor=8,
        mix_parameter=0.8
    )

    #sd.play(data, fs)
    #sd.wait()

    processed = bit.process(data, 48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()