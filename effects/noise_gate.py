import numpy as np
from audioeffect import AudioEffect


class NoiseGate(AudioEffect):
    def __init__(self, **kwargs):
        self.threshold_high = kwargs.get("threshold_high", -40.0)
        self.threshold_low = kwargs.get("threshold_low", -50.0)
        if self.threshold_low >= self.threshold_high:
            raise ValueError("threshold_low must be less than threshold_high")

        detection = kwargs.get("detection", "peak")
        if detection not in ["rms", "peak"]:
            raise KeyError("Unknown detection type")
        self.detection = detection

        self.attack = kwargs.get("attack", 5.0)
        self.release = kwargs.get("release", 20.0)
        self.hold = kwargs.get("hold", 1.0)

        self.hold_counter = 0
        self._gate_open = False

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        if self.detection == "peak":
            level = np.abs(input_data)
        else:
            level = np.sqrt(np.mean(input_data**2))
            level = np.full_like(input_data, level)

        level_db = 20 * np.log10(level + 1e-9)

        alpha_a = 1 - np.exp(-1.0 / (sample_rate * (self.attack / 1000.0)))
        alpha_r = 1 - np.exp(-1.0 / (sample_rate * (self.release / 1000.0)))
        hold_samples = int(sample_rate * (self.hold / 1000.0))

        output_gain = np.zeros_like(input_data, dtype=np.float64)
        env = 1.0 if self._gate_open else 0.0

        for i in range(len(input_data)):
            db = level_db[i]
            if not self._gate_open:
                if db >= self.threshold_high:
                    self._gate_open = True
                    self.hold_counter = hold_samples
            else:
                if db >= self.threshold_low:
                    self.hold_counter = hold_samples
                elif self.hold_counter > 0:
                    self.hold_counter -= 1
                else:
                    self._gate_open = False

            target = 1.0 if self._gate_open else 0.0

            if target > env:
                env = alpha_a * target + (1 - alpha_a) * env
            else:
                env = alpha_r * target + (1 - alpha_r) * env

            output_gain[i] = env

        return input_data * output_gain
    



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

    sd.play(data, fs)
    sd.wait()

    ng = NoiseGate(threshold_high=-20.0, threshold_low=-30.0, attack=5.0, release=50.0, hold=10.0)
    #ovr = Overdrive(gain=1.0, function="atan")

    processed = ng.process(data, fs)
    processed = processed / np.max(np.abs(processed))

    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()