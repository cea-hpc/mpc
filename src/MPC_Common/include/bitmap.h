#ifndef BITMAP_H
#define BITMAP_H

#include <limits.h>

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

typedef struct bmap {
       word_t bits[_MPC_BITMAP_BITS_TO_WORDS(64)];
} bmap_t;

#endif 
