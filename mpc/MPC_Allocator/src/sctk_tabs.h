/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifdef __cplusplus
extern "C"
{
#endif
#if 0
#define max_chunk_sizes 42
#define max_chunk_sizes_max 43
  static const int chunk_sizes[max_chunk_sizes_max] = {
    32, 64, 96, 128, 160, 192, 224, 256, 288, 320,
    352, 384, 416, 448, 480, 512, 544, 576, 608, 640,
    672, 704, 736, 768, 800, 832, 864, 896, 928, 960,
    992, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144,
    524288, 1048576, SCTK_PAGES_ALLOC_SIZE
  };
  static mpc_inline unsigned short sctk_get_chunk_tab (sctk_size_t size)
  {
    unsigned long i;

    if (size <= 1024)
      {
	i = (size - 1) / 32;
      }
    else
      {
	if (size <= 2048)
	  i = 32 + 0;
	else if (size <= 4096)
	  i = 32 + 1;
	else if (size <= 8192)
	  i = 32 + 2;
	else if (size <= 16384)
	  i = 32 + 3;
	else if (size <= 32768)
	  i = 32 + 4;
	else if (size <= 65536)
	  i = 32 + 5;
	else if (size <= 131072)
	  i = 32 + 6;
	else if (size <= 262144)
	  i = 32 + 7;
	else if (size <= 524288)
	  i = 32 + 8;
	else if (size <= 1048576)
	  i = 32 + 9;
	else
	  i = max_chunk_sizes - 1;
      }
    return (unsigned short) i;
  }
#else
#define max_chunk_sizes 42
#define max_chunk_sizes_max 43
  static const size_t chunk_sizes[max_chunk_sizes_max] = {
    32, 48, 64, 80, 96, 112, 128, 144, 160, 176,
    192, 208, 224, 240, 256, 272, 288, 304, 320, 336,
    352, 368, 384, 400, 416, 432, 448, 464, 480, 496,
    512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144,
    524288, 1048576, SCTK_PAGES_ALLOC_SIZE
  };
  static mpc_inline unsigned short sctk_get_chunk_tab (sctk_size_t size)
  {
    unsigned long i;

    if (size <= 512)
      {
	i = (size - 1) / 16;
	i--;
      }
    else
      {
	if (size <= 1024)
	  i = 31 + 0;
	else if (size <= 2048)
	  i = 31 + 1;
	else if (size <= 4096)
	  i = 31 + 2;
	else if (size <= 8192)
	  i = 31 + 3;
	else if (size <= 16384)
	  i = 31 + 4;
	else if (size <= 32768)
	  i = 31 + 5;
	else if (size <= 65536)
	  i = 31 + 6;
	else if (size <= 131072)
	  i = 31 + 7;
	else if (size <= 262144)
	  i = 31 + 8;
	else if (size <= 524288)
	  i = 31 + 9;
	else if (size <= 1048576)
	  i = 31 + 10;
	else
	  i = max_chunk_sizes - 1;
      }
    return (unsigned short) i;
  }

#endif
#ifdef __cplusplus
}				/* end of extern "C" */
#endif
