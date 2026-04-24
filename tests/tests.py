import ctypes
import numpy as np
import matplotlib.pyplot as plt
from enum import IntEnum

lib = ctypes.CDLL('./effects/cext.dll')

SAMPLE_RATE = 44100

# structs, enums ─────────────────────────────────────────────

class BitcrusherConfig(ctypes.Structure):
    _fields_ = [
        ('bit_depth', ctypes.c_uint32),
        ('downsample_factor', ctypes.c_uint32),
        ('mix_parameter', ctypes.c_float),
        ('dither_level', ctypes.c_float),
        ('sample_size', ctypes.c_uint32),
        ('rng_state', ctypes.c_uint32),
        ('tmp_sample', ctypes.c_float),
        ('counter', ctypes.c_uint32),
    ]

class BiquadT(ctypes.Structure):
    _fields_ = [
        ('a1', ctypes.c_float), ('a2', ctypes.c_float),
        ('b0', ctypes.c_float), ('b1', ctypes.c_float), ('b2', ctypes.c_float),
        ('s1', ctypes.c_float), ('s2', ctypes.c_float),
    ]


class EqFilterType(IntEnum):
    EQ_FILTER_LOW_PASS = 0
    EQ_FILTER_HIGH_PASS = 1
    EQ_FILTER_PEAKING = 2
    EQ_FILTER_LOW_SHELF = 3
    EQ_FILTER_HIGH_SHELF = 4

eq_filter_type_map = {
    EqFilterType.EQ_FILTER_LOW_PASS: "LPF",
    EqFilterType.EQ_FILTER_HIGH_PASS: "HPF",
    EqFilterType.EQ_FILTER_PEAKING: "PK",
    EqFilterType.EQ_FILTER_LOW_SHELF: "LSH",
    EqFilterType.EQ_FILTER_HIGH_SHELF: "HSH"
}


EQ_MAX_BANDS = 8

class EqBandConfig(ctypes.Structure):
    _fields_ = [
        ('type', ctypes.c_int),
        ('f0', ctypes.c_float),
        ('Q', ctypes.c_float),
        ('db_gain', ctypes.c_float),
        ('enabled', ctypes.c_bool),
    ]

class EqualiserT(ctypes.Structure):
    _fields_ = [
        ('sample_rate', ctypes.c_uint32),
        ('bands', EqBandConfig * EQ_MAX_BANDS),
        ('biquads', BiquadT * EQ_MAX_BANDS),
    ]

DRIVE_SOFT = 0
DRIVE_HARD = 1

class OverdriveConfig(ctypes.Structure):
    _fields_ = [
        ('gain', ctypes.c_float),
        ('driving_function', ctypes.c_int),
        ('sample_size', ctypes.c_uint32),
    ]

# function signatures ─────────────────────────────────────────────


lib.process_bitcrusher.argtypes = [
    ctypes.POINTER(BitcrusherConfig),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
]
lib.process_bitcrusher.restype = None

lib.eq_reset.argtypes = [ctypes.POINTER(EqualiserT)]
lib.eq_reset.restype = None

lib.eq_process.argtypes = [
    ctypes.POINTER(EqualiserT),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
    ctypes.c_uint32,
]
lib.eq_process.restype = None

lib.eq_set_band.argtypes = [
    ctypes.POINTER(EqualiserT),
    ctypes.c_uint32,
    ctypes.c_int,
    ctypes.c_float,
    ctypes.c_float,
    ctypes.c_float,
]
lib.eq_set_band.restype = None

lib.eq_enable_band.argtypes = [ctypes.POINTER(EqualiserT), ctypes.c_uint32]
lib.eq_enable_band.restype = None
lib.eq_disable_band.argtypes = [ctypes.POINTER(EqualiserT), ctypes.c_uint32]
lib.eq_disable_band.restype = None

lib.process_overdrive.argtypes = [
    ctypes.POINTER(OverdriveConfig),
    ctypes.POINTER(ctypes.c_float),
    ctypes.POINTER(ctypes.c_float),
]
lib.process_overdrive.restype = None

# ─────────────────────────────────────────────


def test_eq():
    f0 = 1000
    Q = 0.707
    gain = 0.0
    N = 16384
    ftype = EqFilterType.EQ_FILTER_LOW_PASS
    x = np.zeros(N, dtype=np.float32)
    x[0] = 1.0
    y = np.zeros(N, dtype=np.float32)

    eq = EqualiserT()
    eq.sample_rate = SAMPLE_RATE
    lib.eq_reset(ctypes.byref(eq))
    lib.eq_set_band(ctypes.byref(eq), 0, ftype, ctypes.c_float(f0), ctypes.c_float(Q), ctypes.c_float(gain))
    lib.eq_enable_band(ctypes.byref(eq), 0)

    lib.eq_process(ctypes.byref(eq), x.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), 
                    y.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), N)
    
    f_fft = np.fft.rfftfreq(N, d=1/SAMPLE_RATE)
    y_fft = 20*np.log10(np.abs(np.fft.rfft(y)) + 1e-10)

    plt.semilogx(f_fft, y_fft)
    plt.title(f"{eq_filter_type_map[ftype]}, Q={Q}, f0={f0}, gain={gain}")
    plt.xlabel('f [Hz]')
    plt.ylabel('|H(f)| [dB]')
    plt.grid(ls="--", alpha=0.5)
    plt.xlim(20, 20000)
    plt.show()


if __name__ == "__main__":
    test_eq()
