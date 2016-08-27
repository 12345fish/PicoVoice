#include "base.h"
#include "utils.h"

uint32_t sqrt32(uint32_t val)
{
    int r = 0;
    int shift;
    int x;
    
    for (shift = 0; shift < 32; shift += 2) {
        x = 0x40000000L >> shift;

        if (x + r <= val) {
            val -= x + r;
            r = (r >> 1) | x;
        } else {
            r = r >> 1;
        }
    }
    //if(r < val) r++;

    return r;
}

/*-----------------------------------*/
// 定点对数计算 from https://github.com/dmoulding/log2fix

#define INV_LOG2_E_Q1DOT31      (0x58b90bfc) // Inverse log base 2 of e
#define INV_LOG2_10_Q1DOT31     (0x268826a1) // Inverse log base 2 of 10

// This implementation is based on Clay. S. Turner's fast binary logarithm
// algorithm[1].
int32_t log2fix (uint32_t x, size_t precision)
{
    int32_t y = 0;
    int32_t b;
    uint64_t z;
    uint32_t i;

    if (precision < 1 || precision > 31) {
        return INT32_MAX; // indicates an error
    }

    if (x == 0) {
        return INT32_MIN; // represents negative infinity
    }

    while (x < (1U << precision)) {
        x <<= 1;
        y -= (1U << precision);
    }

    while (x >= (2U << precision)) {
        x >>= 1;
        y += (1U << precision);
    }

    z = x;
    b = 1U << (precision - 1);

    for (i = 0; i < precision; i++) {
        z = (z * z) >> precision;
        if (z >= (2U << precision)) {
            z >>= 1;
            y += b;
        }
        b >>= 1;
    }

    return y;
}

int32_t logfix (uint32_t x, size_t precision)
{
    uint64_t t;

    t = log2fix(x, precision) * INV_LOG2_E_Q1DOT31;

    return t >> 31;
}

int32_t log10fix (uint32_t x, size_t precision)
{
    uint64_t t;

    t = log2fix(x, precision) * INV_LOG2_10_Q1DOT31;

    return t >> 31;
}
