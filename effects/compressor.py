import numpy as np
from audioeffect import AudioEffect


class Compressor(AudioEffect):
    def __init__(self, **kwargs):
        self.threshold = kwargs.get("threshold", -6.0)
        type = kwargs.get("type", "downward")
        if type not in ["downward", "upward"]:
            raise KeyError("Unknown compressor type")
        self.type = type
        self.ratio = kwargs.get("ratio", 2.0)
        detection = kwargs.get("detection", "peak")
        if detection not in ["rms", "peak"]:
            raise KeyError("Unknown compressor detection type")
        self.detection = detection
        self.attack = kwargs.get("attack", 5.0)
        self.release = kwargs.get("release", 20.0)
        self.hold = kwargs.get("hold", 1.0)

        self.hold_counter = 0

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        if self.detection == "peak":
            level = np.abs(input_data)
        else:
            level = np.sqrt(np.mean(input_data**2))
            level = np.full_like(input_data, level)

        level_db = 20 * np.log10(level + 1e-9)
        over_threshold = level_db - self.threshold
        
        if self.type == "downward":
            target_gain_db = np.where(over_threshold > 0, -(over_threshold * (1 - 1/self.ratio)), 0.0)
        else: 
            under_threshold = self.threshold - level_db
            target_gain_db = np.where(under_threshold > 0, under_threshold * (1 - 1/self.ratio), 0.0)

        target_gain_linear = 10**(target_gain_db / 20)
        
        alpha_a = 1 - np.exp(-1.0 / (sample_rate * (self.attack / 1000.0)))
        alpha_r = 1 - np.exp(-1.0 / (sample_rate * (self.release / 1000.0)))
        hold_samples = int(sample_rate * (self.hold / 1000.0))
        
        output_gain = np.zeros_like(target_gain_linear)
        env = 0.0

        for i in range(len(target_gain_linear)):
            target = target_gain_linear[i]
            
            if target < env:
                env = alpha_a * target + (1 - alpha_a) * env
                self.hold_counter = hold_samples
            else:
                if self.hold_counter > 0:
                    self.hold_counter -= 1
                else:
                    env = alpha_r * target + (1 - alpha_r) * env
            
            output_gain[i] = env

        return input_data * output_gain
    



def main():
    import soundfile as sf
    import sounddevice as sd
    from overdrive import Overdrive

    path = "effects/pianoman_mono.wav"
     
    try:
        data, fs = sf.read(path)
    except Exception as e:
        print(f"Error: {e}")
        return

    if len(data.shape) > 1:
        data = data[:, 0]

    cmp = Compressor(ratio=10.0, type="downward", threshold=-20)
    #ovr = Overdrive(gain=1.0, function="atan")

    processed = cmp.process(data, fs)
    processed = processed / np.max(np.abs(processed))

    sd.play(processed, fs)
    sd.wait()
if __name__ == "__main__":
    main()
