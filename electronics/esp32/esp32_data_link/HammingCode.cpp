#include <stdint.h>
#include <string.h>

#define DATA_BITS   120
#define CODE_BITS   128
#define DATA_BYTES  (DATA_BITS / 8)   // 15
#define CODE_BYTES  (CODE_BITS / 8)   // 16

/* ---- bit helpers (little-endian within each byte) ---- */
static inline uint8_t get_bit(const uint8_t* b, int i) {
    return (b[i >> 3] >> (i & 7)) & 1u;
}
static inline void set_bit(uint8_t* b, int i, uint8_t v) {
    if (v) b[i >> 3] |=  (uint8_t)(1u << (i & 7));
    else   b[i >> 3] &= (uint8_t)~(1u << (i & 7));
}
static inline void flip_bit(uint8_t* b, int i) {
    b[i >> 3] ^= (uint8_t)(1u << (i & 7));
}

/* ---- encode: 15 bytes in -> 16 bytes out ---- */
void encode(const uint8_t* input, uint8_t* output) {
    memset(output, 0, CODE_BYTES);

    /* place data bits in non-power-of-two positions */
    int d = 0;
    for (int pos = 1; pos <= CODE_BITS; pos++) {
        if ((pos & (pos - 1)) == 0) continue;   // power of two = parity slot
        if (get_bit(input, d)) set_bit(output, pos - 1, 1);
        d++;
    }

    /* compute the 8 parity bits */
    for (int p = 1; p <= CODE_BITS; p <<= 1) {
        uint8_t par = 0;
        for (int j = 1; j <= CODE_BITS; j++) {
            if (j & p) par ^= get_bit(output, j - 1);
        }
        set_bit(output, p - 1, par);
    }
}