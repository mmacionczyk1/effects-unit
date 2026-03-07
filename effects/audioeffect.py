import numpy as np

class AudioEffect:
    def process(self, input_data: np.ndarray, sample_rate: int) -> np.ndarray:
        raise NotImplementedError("Class must implement process()")

    def to_dict(self) -> dict:
        raise NotImplementedError("Class must implement JSON export")

    def load_params(self, params: dict):
        raise NotImplementedError("Class must implement JSON import")
