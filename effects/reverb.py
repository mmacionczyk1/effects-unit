import numpy as np
from audioeffect import AudioEffect

class CombFilter:
    def __init__(self, **kwargs):
        self.delay = kwargs.get('delay_samples', 0)
        self.feedback = kwargs.get('feedback', 0.8)
        self.buffer = np.zeros(self.delay)
        self.pointer = 0

    def process_sample(self, input_sample: float) -> float:
        delay_sample = self.buffer[self.pointer]
        output_sample = input_sample + self.feedback * delay_sample
        self.buffer[self.pointer] = output_sample
        self.pointer = (self.pointer + 1) % self.delay
        return output_sample

class AllPassFilter:
    def __init__(self, **kwargs):
        self.delay = kwargs.get('delay_samples', 0)
        self.feedback = kwargs.get('feedback', 0.8)
        self.buffer = np.zeros(self.delay)
        self.pointer = 0
    def process_sample(self, input_sample: float) -> float:
        delay_sample = self.buffer[self.pointer]
        output_sample = - self.feedback * input_sample + delay_sample
        self.buffer[self.pointer] = input_sample + self.feedback * delay_sample
        self.pointer = (self.pointer + 1) % self.delay
        return output_sample
    
class ReverbEffect(AudioEffect):
    def __init__(self, **kwargs):
        self.sample_rate = kwargs.get('sample_rate', 48000)
        self.comb_delays = kwargs.get('comb_delays', [29, 37, 43, 53]) #prime numbers in ms
        self.allpass_delays = kwargs.get('allpass_delays', [5, 3])
        self.feedback = kwargs.get('feedback', 0.8)
        self.apf_feedback = kwargs.get('apf_feedback', 0.5)
        self.mix_parameter = kwargs.get('mix_parameter', 0.4)
    
    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        self.comb_filters = [CombFilter(delay_samples=int(sample_rate * d / 1000), feedback=self.feedback) for d in self.comb_delays]
        self.all_pass_filters = [AllPassFilter(delay_samples=int(sample_rate * d / 1000), feedback=self.apf_feedback) for d in self.allpass_delays]
        output_data = np.zeros(len(input_data))
        for i in range(len(input_data)):
            comb_sum = 0
            for comb in self.comb_filters:
                comb_sum += comb.process_sample(input_data[i])
            reverb = comb_sum / len(self.comb_filters)
            for allpass in self.all_pass_filters:
                reverb = allpass.process_sample(reverb)
            output_data[i] = (1 - self.mix_parameter) * input_data[i] + self.mix_parameter * reverb
        return output_data
        


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
    rvb = ReverbEffect(comb_delays=[47, 71, 43, 51], allpass_delays=[1, 2], feedback=0.8, apf_feedback=0.5, mix_parameter=0.4)

    #sd.play(data, fs)
    #sd.wait()

    processed = rvb.process(data, sample_rate=48000)

    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()