from enum import Enum


DATA_BITS = 120
CODE_BITS = 128
DATA_BYTES = DATA_BITS // 8  # 15
CODE_BYTES = CODE_BITS // 8  # 16


def _get_bit(buf, i):
    return (buf[i >> 3] >> (i & 7)) & 1


def _flip_bit(buf, i):
    buf[i >> 3] ^= 1 << (i & 7)


def _log2(x):
    # floor(log2(x)), matching the C helper
    return x.bit_length() - 1


def calculate_error_syndrome(code):
    """0    = clean
    1..128 = single-bit error at that LOGICAL (1-based) Hamming position
    129..255 = uncorrectable multi-bit error (best-effort flag)"""
    syndrome = 0
    p = 1
    while p <= CODE_BITS:
        par = 0
        d = 0
        for j in range(1, CODE_BITS + 1):
            if (j & (j - 1)) == 0:  # power-of-two = parity slot
                continue
            if j & p:
                par ^= _get_bit(code, d)
            d += 1
        par ^= _get_bit(code, DATA_BITS + _log2(p))  # include the parity bit itself
        if par:
            syndrome |= p
        p <<= 1
    return syndrome


def decode(code, syndrome):
    """Return corrected payload as bytes.

    Output is 128 bits (16 bytes): low 120 bits are the corrected data,
    high 8 bits are zero. Input is not modified.
    """
    out = bytearray(code[:DATA_BYTES])

    # Correct only for a single-bit *data* error: 1..128 and not a power of two.
    # syndrome == 0                -> clean, no change
    # syndrome power of two        -> parity bit flipped, data already fine
    # syndrome > 128               -> uncorrectable multi-bit error, leave as-is
    if 1 <= syndrome <= CODE_BITS and (syndrome & (syndrome - 1)) != 0:
        phys = syndrome - 2 - _log2(syndrome)  # logical -> physical data bit
        _flip_bit(out, phys)

    out.extend(b"\x00" * (CODE_BYTES - DATA_BYTES))  # pad high byte(s) with zero
    return bytes(out)


class Checksum(Enum):
    NO_ERR = 0
    BIT_ERR = 1
    CORRUPT = 2
