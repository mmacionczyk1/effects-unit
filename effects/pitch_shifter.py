import numpy as np
from audioeffect import AudioEffect
from scipy import signal

class PitchShifter(AudioEffect):
    def __init__(self, **kwargs):
        self.semitones = kwargs.get("semitones", 0.0)
        self.frame_size = kwargs.get("frame_size", 2048)
        self.hop_size = kwargs.get("hop_size", None)
        self.window_type = kwargs.get("window", "hann")

    @property
    def ratio(self) -> float:
        return 2.0 ** (self.semitones / 12.0)

    def set_semitones(self, semitones: float) -> None:
        self.semitones = semitones

    def _instantaneous_frequency(self, frame: np.ndarray) -> np.ndarray:
        analytic = signal.hilbert(frame)
        inst_phase = np.unwrap(np.angle(analytic))
        return np.gradient(inst_phase)

    def _true_bin_frequencies(self, ana_phase: np.ndarray, prev_ana_phase: np.ndarray, hop: int, N: int) -> np.ndarray:
        K = N // 2 + 1
        omega = 2.0 * np.pi * np.arange(K) / N
        delta = ana_phase - prev_ana_phase - omega * hop
        delta -= 2.0 * np.pi * np.round(delta / (2.0 * np.pi))
        return omega + delta / hop

    def _shift_bins(self, mag: np.ndarray, true_omega: np.ndarray, ratio: float) -> tuple[np.ndarray, np.ndarray]:
        K = len(mag)
        src = np.arange(K) / ratio
        k0 = np.floor(src).astype(int)
        k1 = k0 + 1
        frac = src - k0

        valid = (k0 >= 0) & (k1 < K)
        edge  = (k0 >= 0) & (k0 < K) & ~valid

        s_mag = np.zeros(K)
        s_omega = np.zeros(K)

        s_mag[valid] = (1.0 - frac[valid]) * mag[k0[valid]] + frac[valid] * mag[k1[valid]]
        s_omega[valid] = ((1.0 - frac[valid]) * true_omega[k0[valid]]   +   frac[valid] * true_omega[k1[valid]]) * ratio

        s_mag[edge] = mag[k0[edge]]
        s_omega[edge] = true_omega[k0[edge]] * ratio

        return s_mag, s_omega

    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        ratio = self.ratio
        N = self.frame_size
        H = self.hop_size if self.hop_size is not None else N // 4
        K = N // 2 + 1

        window = signal.get_window(self.window_type, N)
        window_sq = window ** 2

        x = np.pad(input_data.astype(np.float64), (N, N))
        L = len(x)
        out  = np.zeros(L)
        norm = np.zeros(L)

        prev_ana_phase = np.zeros(K)
        syn_phase      = np.zeros(K)

        for start in range(0, L - N, H):
            frame = x[start : start + N] * window
            spectrum = np.fft.rfft(frame)
            mag = np.abs(spectrum)
            ana_phase = np.angle(spectrum)

            true_omega = self._true_bin_frequencies(ana_phase, prev_ana_phase, H, N)
            prev_ana_phase = ana_phase.copy()


            hilbert_if = self._instantaneous_frequency(frame)
            env_weight = np.abs(signal.hilbert(frame))
            global_if = np.average(hilbert_if, weights=env_weight + 1e-12)
            true_omega[0] = np.clip(global_if, 0.0, np.pi)
            true_omega[K - 1] = np.clip(global_if * (K - 1), 0.0, np.pi)

            s_mag, s_omega = self._shift_bins(mag, true_omega, ratio)

            syn_phase += s_omega * H
            syn_spectrum = s_mag * np.exp(1j * syn_phase)
            out_frame = np.fft.irfft(syn_spectrum) * window

            out[start : start + N]  += out_frame
            norm[start : start + N] += window_sq

        safe = norm > 1e-8
        out[safe] /= norm[safe]

        return out[N : N + len(input_data)]
    



    
def main():
    import soundfile as sf
    import sounddevice as sd
    from overdrive import Overdrive
    import matplotlib.pyplot as plt
    path = "effects/pianoman_mono.wav"
    
    try:
        data, fs = sf.read(path)
    except Exception as e:
        print(f"Error: {e}")
        return
    if len(data.shape) > 1:
        data = data[:, 0]
    psh = PitchShifter(semitones=15)
    #sd.play(data, fs)
    #sd.wait()

    processed = psh.process(data, fs)
    
    processed = processed / np.max(np.abs(processed))
    sd.play(processed, fs)
    sd.wait()


if __name__ == "__main__":
    main()