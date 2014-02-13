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
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

/************************** HEADERS ************************/
#include <svUnitTest/svUnitTest.h>
#include <sctk_allocator.h>
#include <cstdlib>
#include <cstring>
#include "test_helper.h"
#include "sctk_alloc_light_mm_source.h"
#include "sctk_alloc_on_node.h"
#include "sctk_alloc_numa_stat.h"
#include "sctk_alloc_topology.h"

/************************** USING **************************/
using namespace svUnitTest;

/************************* FUNCTION ************************/
SVUT_DECLARE_FLAT_SETUP(TestMallocOnNode)
{
	sctk_alloc_init_topology();
}

/************************* FUNCTION ************************/
SVUT_DECLARE_FLAT_TEAR_DOWN(TestMallocOnNode)
{
	sctk_malloc_on_node_reset();
}

/************************* FUNCTION ************************/
SVUT_DECLARE_FLAT_TEST(TestMallocOnNode,test_init)
{
	sctk_malloc_on_node_init(2);
}

/************************* FUNCTION ************************/
SVUT_DECLARE_FLAT_TEST(TestMallocOnNode,test_alloc_on_0)
{
	const sctk_size_t size = 4*1024*1024;
	sctk_malloc_on_node_init(2);
	void * ptr = sctk_malloc_on_node(size,0);
	SVUT_ASSERT_NOT_NULL(ptr);
	memset(ptr,0,size);
	assert_numa(ptr,size,0,100,"Faild to allocate completely the array on NUMA node 0.");
}

/************************* FUNCTION ************************/
//use default main implementation
SVUT_USE_DEFAULT_MAIN
