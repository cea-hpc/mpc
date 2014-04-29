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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

/************************** HEADERS ************************/
#include <svUnitTest/svUnitTest.h>
#include <cstring>
#include <sctk_alloc_inlined.h>
#include <omp.h>
#ifdef _WIN32
	#include <windows.h>
#endif

/************************** CONSTS *************************/
#define LARGE_HEADER (sizeof(sctk_alloc_chunk_header_large))

/************************* FUNCTION ************************/
bool asserterOperatorEqual(const sctk_alloc_vchunk & v1,const sctk_alloc_vchunk & v2)
{
	return (v1 == v2);
}

std::string asserterToString(const sctk_alloc_vchunk & v1)
{
	std::stringstream res;
	res << "bloc = " << v1 << " ; ";
	if (v1->type == SCTK_ALLOC_CHUNK_TYPE_LARGE)
		res << "size = " << sctk_alloc_get_size(v1) << " ; ";
	else
		res << "size = 0 ; ";
	return res.str();
}

extern "C" {
/** Replace the default file name for chain spying. **/
void sctk_alloc_spy_chain_get_fname(struct sctk_alloc_chain * chain,char * fname)
{
	strcpy(fname,"/dev/null");
}
}

#if defined(_WIN32) && !defined(_MSC_VER)
	int omp_get_num_threads(void) { return 1;}
	int omp_get_thread_num (void) { return 0;}
#endif

#endif
