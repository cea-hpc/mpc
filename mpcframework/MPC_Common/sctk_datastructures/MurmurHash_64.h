#include <stdint.h>

static inline uint64_t sctk_murmur_hash(const uint64_t key, int len,
                                        uint32_t seed) {
  const uint64_t m = 0xc6a4a7935bd1e995llu;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t *data = (const uint64_t *)key;
  const uint64_t *end = data + (len / 8);

  while (data != end) {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char *data2 = (const unsigned char *)data;

  switch (len & 7) {
  case 7:
    h ^= ((uint64_t)data2[6]) << 48;
  case 6:
    h ^= ((uint64_t)data2[5]) << 40;
  case 5:
    h ^= ((uint64_t)data2[4]) << 32;
  case 4:
    h ^= ((uint64_t)data2[3]) << 24;
  case 3:
    h ^= ((uint64_t)data2[2]) << 16;
  case 2:
    h ^= ((uint64_t)data2[1]) << 8;
  case 1:
    h ^= ((uint64_t)data2[0]);
    h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

static inline uint64_t simple_mix_hash(uint64_t val) {
  /* This is MURMUR Hash under MIT
   * https://code.google.com/p/smhasher/ */
  uint64_t h = val;

  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdllu;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53llu;
  h ^= h >> 33;

  return h;
}
