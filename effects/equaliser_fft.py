import numpy as np
from audioeffect import AudioEffect
from scipy.interpolate import CubicSpline
from scipy.fftpack import hilbert

class EqualiserFFT(AudioEffect):
    def __init__(self, **kwargs):
        self.points = kwargs.get("points", [(10e3, 0)],) # [(f1, a1), (f2, a2), ...] [Hz, dB]
    

    def _interpolate(self, fft_size, fs) -> np.ndarray:
        points_sorted = sorted(self.points, key = lambda x: x[0])
        freqs, gains = zip(*points_sorted)
        log_freqs = np.log10(freqs)
        cs = CubicSpline(log_freqs, gains, bc_type="natural")

        fft_freqs = np.fft.rfftfreq(fft_size, 1/fs)
        fft_freqs[0] = 1e-9
        log_fft_freqs = np.log10(fft_freqs)

        interpolated = cs(log_fft_freqs)
        interpolated = np.clip(interpolated, -100, 40)
        linear = 10**(interpolated/20)
        log_amp = np.log(linear + 1e-12)
        phase = -hilbert(log_amp)

        return linear * np.exp(1j * phase)

    def process(self, input_data, sample_rate):
        spectrum = np.fft.rfft(input_data)
        mask = self._interpolate(len(input_data), sample_rate)
        filtered = spectrum * mask
        output = np.fft.irfft(filtered, n=len(input_data))
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

    eq = [
    (20, -60),
    (300, -40),
    (1000, 6),
    (3000, -40),
    (20000, -60)
    ]

    eqr = EqualiserFFT(points=eq)
    processed = eqr.process(data, fs)
    
    sd.play(processed, fs)
    sd.wait()

if __name__ == "__main__":
    main()
