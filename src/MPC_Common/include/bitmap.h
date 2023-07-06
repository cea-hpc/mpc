#ifndef BITMAP_H
#define BITMAP_H

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <lcp_common.h>

typedef uint64_t word_t;

enum { 
        BITS_PER_WORD = sizeof(word_t) * CHAR_BIT 
};

#define MPC_BITMAP_MASK(i) (LCP_BIT(i) -1)

#define MPC_BITMAP_INIT \
{ \
        .bits = { 0 } \
}

#define _MPC_BITMAP_BITS_TO_WORDS(_length) \
        ((((_length) + (BITS_PER_WORD - 1)) / BITS_PER_WORD))

#define MPC_WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define MPC_BIT_OFFSET(b)  ((b) % BITS_PER_WORD)

#define _MPC_BITMAP_NUM_WORDS(_bitmap) \
        sizeof((_bitmap).bits)/sizeof((_bitmap).bits[0])

#define _MPC_BITMAP_FOR_EACH_WORD(_bitmap, _word_index) \
        for (_word_index = 0; _word_index < _MPC_BITMAP_NUM_WORDS(_bitmap); \
             _word_index++)

#define MPC_BITMAP_SET(_bitmap, _bit_index) \
        ({ \
         _bitmap.bits[MPC_WORD_OFFSET(_bit_index)] |= (1 << MPC_BIT_OFFSET(_bit_index)); \
        })

#define MPC_BITMAP_UNSET(_bitmap, _bit_index) \
        ({ \
         _bitmap.bits[MPC_WORD_OFFSET(_bit_index)] &= ~(1 << MPC_BIT_OFFSET(_bit_index)); \
         )}

#define MPC_BITMAP_GET(_bitmap, _bit_index) \
        (!!(_bitmap.bits[MPC_WORD_OFFSET(_bit_index)] & (1 << MPC_BIT_OFFSET(_bit_index))))

#define MPC_BITMAP_AND(_bm1, _bm2) \
        ({ \
                int result; \
                size_t word_index; \
                _MPC_BITMAP_FOR_EACH_WORD(_bm1, word_index) { \
                        result = ((_bm1).bits[word_index] == (_bm2).bits[word_index]); \
                } \
                result; \
        })


typedef struct bmap {
       word_t bits[_MPC_BITMAP_BITS_TO_WORDS(64)];
} bmap_t;

static inline int mpc_bitmap_is_zero(bmap_t bitmap) {
        size_t word_index;
        _MPC_BITMAP_FOR_EACH_WORD(bitmap, word_index) {
                if (bitmap.bits[word_index]) {
                        return 0;
                }
        }
        return 1;
}

static inline int mpc_bitmap_equal(bmap_t a, bmap_t b) {
        return !(memcmp(a.bits, b.bits, sizeof(a.bits)));
}




#endif 
