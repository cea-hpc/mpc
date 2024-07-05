#ifndef BITMAP_H
#define BITMAP_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mpc_common_bit.h"

//FIXME: remove and use the one from mpc_common_helper
typedef uint64_t word_t;

enum {
        BITS_PER_WORD = sizeof(word_t) * CHAR_BIT
};

#define MPC_BITMAP_MASK(i) (MPC_BIT(i) -1)

#define MPC_BITMAP_INIT \
{ \
        .bits = { 0 } \
}

#define MPC_BITMAP_INIT_ONE \
{ \
        .bits = { -1 } \
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
         })

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

#define MPC_BITMAP_SET_FIRST_N(_bitmap, _n) \
        do { \
                size_t _word_index; \
                size_t _bits_to_set = (_n); \
                memset(_bitmap.bits, 0, sizeof(_bitmap.bits)); \
                _MPC_BITMAP_FOR_EACH_WORD(_bitmap, _word_index) {\
                        if (_bits_to_set >= BITS_PER_WORD) { \
                                _bitmap.bits[_word_index] = (word_t)-1; /* Set all bits in this word */ \
                                _bits_to_set -= BITS_PER_WORD; \
                        } else { \
                                _bitmap.bits[_word_index] = (1ULL << _bits_to_set) - 1; /* Set the first _bits_to_set bits in this word */ \
                                _bits_to_set = 0; \
                        } \
                } \
        } while(0)

#define MPC_BITMAP_COPY(_dest_bitmap, _src_bitmap) \
        do { \
                memcpy((_dest_bitmap).bits, (_src_bitmap).bits, sizeof((_src_bitmap).bits)); \
        } while (0)

typedef struct bmap {
       word_t bits[_MPC_BITMAP_BITS_TO_WORDS(64)];
} bmap_t;


/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */
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
/* NOLINTEND(clang-diagnostic-unused-function) */

static inline int mpc_bitmap_popcount(bmap_t bitmap) {
        int count = 0;
        size_t word_index;
        _MPC_BITMAP_FOR_EACH_WORD(bitmap, word_index) {
                while (bitmap.bits[word_index]) {
                        bitmap.bits[word_index] &= (bitmap.bits[word_index] - 1);
                        count++;
                }
        }
        return count;
}

static inline bmap_t mpc_bitmap_copy_and(bmap_t a, bmap_t b) {
        size_t word_index;
        bmap_t out;
        _MPC_BITMAP_FOR_EACH_WORD(a, word_index) {
                out.bits[word_index] = a.bits[word_index] & b.bits[word_index];
        }
        return out;
}

#endif 
