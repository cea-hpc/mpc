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

/************************** USING **************************/
using namespace svUnitTest;

/************************** CONSTS *************************/
#define TEST_REAL_HEAP_BASE (SCTK_ALLOC_HEAP_BASE)
#define TEST_REAL_HEAP_SIZE (SCTK_ALLOC_HEAP_SIZE - SCTK_ALLOC_PAGE_SIZE)

/*************************** CLASS *************************/
class TestFullAllocChain : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_reclaim_mm_1(void);
		void test_reclaim_mm_2(void);
		void test_reclaim_mm_3(void);
		void test_free_macro_bloc_1(void);
		void test_free_macro_bloc_2(void);
		void test_bug_seq_1(void);
		void test_mem_opti_seq_1(void);
		void test_region_entry_1(void);
		void test_region_entry_2(void);
		void test_region_entry_3(void);
	private:
		sctk_alloc_chain chain;
		sctk_alloc_mm_source_default source;
};

/************************* FUNCTION ************************/
void TestFullAllocChain::testMethodsRegistration (void)
{
	setTestCaseName("TestFullAllocChain");
	SVUT_REG_TEST_METHOD(test_reclaim_mm_1);
	SVUT_REG_TEST_METHOD(test_reclaim_mm_2);
	SVUT_REG_TEST_METHOD(test_reclaim_mm_3);
	SVUT_REG_TEST_METHOD(test_free_macro_bloc_1);
	SVUT_REG_TEST_METHOD(test_free_macro_bloc_2);
	SVUT_REG_TEST_METHOD(test_bug_seq_1);
	SVUT_REG_TEST_METHOD(test_mem_opti_seq_1);
	SVUT_REG_TEST_METHOD(test_region_entry_1);
	SVUT_REG_TEST_METHOD(test_region_entry_2);
	SVUT_REG_TEST_METHOD(test_region_entry_3);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::setUp (void)
{
	#ifdef _WIN32
		MARK_AS_KNOWN_ERROR("Depends on default memory source, not ported.\n");
	#endif

	//SCTK_ALLOC_SPY_HOOK(__sctk_alloc_spy_chain_reset());
	/*
	#ifdef _WIN32
		void *res = VirtualAlloc((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		assert(res == (void*)SCTK_ALLOC_HEAP_BASE);
	#endif
	*/
	sctk_alloc_chain_user_init(&chain,NULL,0,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);
	sctk_alloc_mm_source_default_init(&source,SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE);
	chain.source = &source.source;
}

/************************* FUNCTION ************************/
void TestFullAllocChain::tearDown (void)
{
	sctk_alloc_chain_destroy(&chain,true);
	source.source.cleanup(&source.source);
	#ifdef _WIN32
		VirtualFree((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RELEASE);
	#endif
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_reclaim_mm_1(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,64);
	SVUT_ASSERT_NOT_NULL(ptr);
	SVUT_ASSERT_GT((char*)TEST_REAL_HEAP_BASE,ptr);
	SVUT_ASSERT_LT((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE,ptr);

	sctk_alloc_chain_alloc(&chain,64);
	SVUT_ASSERT_NOT_NULL(ptr);
	SVUT_ASSERT_GT((char*)TEST_REAL_HEAP_BASE,ptr);
	SVUT_ASSERT_LT((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE,ptr);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_reclaim_mm_2(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,1024*1024);
	SVUT_ASSERT_NOT_NULL(ptr);
	SVUT_ASSERT_GT((char*)TEST_REAL_HEAP_BASE,ptr);
	SVUT_ASSERT_LT((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE,ptr);

	ptr = sctk_alloc_chain_alloc(&chain,1024*1024);
	SVUT_ASSERT_NOT_NULL(ptr);
	SVUT_ASSERT_GT((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE,ptr);
	SVUT_ASSERT_LT((char*)TEST_REAL_HEAP_BASE+2*SCTK_MACRO_BLOC_SIZE,ptr);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_reclaim_mm_3(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,2*SCTK_ALLOC_HEAP_SIZE);
	SVUT_ASSERT_NULL(ptr);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_free_macro_bloc_1(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,1024*1024);
	SVUT_ASSERT_NOT_NULL(ptr);
	SVUT_ASSERT_GT((char*)TEST_REAL_HEAP_BASE,ptr);
	SVUT_ASSERT_LT((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE,ptr);

	sctk_alloc_macro_bloc * macro = sctk_alloc_get_macro_bloc(ptr);
	SVUT_ASSERT_SAME((char*)TEST_REAL_HEAP_BASE,macro);
	SVUT_ASSERT_EQUAL((sctk_size_t)SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_chunk_header_large_size(&macro->header));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,sctk_alloc_get_chunk_header_large_info(&macro->header)->state);

	sctk_alloc_chain_free(&chain,ptr);
	SVUT_ASSERT_EQUAL((sctk_size_t)TEST_REAL_HEAP_SIZE,sctk_alloc_get_chunk_header_large_size(&macro->header));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_FREE,sctk_alloc_get_chunk_header_large_info(&macro->header)->state);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_free_macro_bloc_2(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,64);
	ptr = sctk_alloc_chain_alloc(&chain,64);
	SVUT_ASSERT_NOT_NULL(ptr);

	sctk_alloc_macro_bloc * macro = sctk_alloc_get_macro_bloc(ptr);
	SVUT_ASSERT_SAME((void*)TEST_REAL_HEAP_BASE,macro);
	SVUT_ASSERT_EQUAL((sctk_size_t)SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_chunk_header_large_size(&macro->header));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,sctk_alloc_get_chunk_header_large_info(&macro->header)->state);

	sctk_alloc_chain_free(&chain,ptr);

	SVUT_ASSERT_EQUAL((sctk_size_t)SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_chunk_header_large_size(&macro->header));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,sctk_alloc_get_chunk_header_large_info(&macro->header)->
	state);
}

/************************* FUNCTION ************************/
//Bug identified in version 318a72fb7d39d04ade5e11f565f09b051e61e033
//error on mapping status when splitting a non mapped macro bloc
void TestFullAllocChain::test_bug_seq_1(void )
{
	void * ptr[9];
	ptr[0] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[1] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[2] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[3] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[4] = sctk_alloc_chain_alloc(&chain,1024*1024);

	sctk_alloc_chain_free(&chain,ptr[1]);
	sctk_alloc_chain_free(&chain,ptr[2]);
	sctk_alloc_chain_free(&chain,ptr[3]);

	ptr[1] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[2] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[3] = sctk_alloc_chain_alloc(&chain,1024*1024);

	ptr[5] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[6] = sctk_alloc_chain_alloc(&chain,1024*1024);
	ptr[7] = sctk_alloc_chain_alloc(&chain,1024*1024);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_mem_opti_seq_1(void )
{
	void * blocs[5];
	void * nblocs[3];
	blocs[0] = sctk_alloc_chain_alloc(&chain,1024*1024);
	blocs[1] = sctk_alloc_chain_alloc(&chain,1024*1024);
	blocs[2] = sctk_alloc_chain_alloc(&chain,1024*1024);
	blocs[3] = sctk_alloc_chain_alloc(&chain,1024*1024);
	blocs[4] = sctk_alloc_chain_alloc(&chain,1024*1024);

	sctk_alloc_chain_free(&chain,blocs[1]);
	sctk_alloc_chain_free(&chain,blocs[2]);
	sctk_alloc_chain_free(&chain,blocs[3]);

	nblocs[0] = sctk_alloc_chain_alloc(&chain,1024*1024);
	nblocs[1] = sctk_alloc_chain_alloc(&chain,1024*1024);
	nblocs[2] = sctk_alloc_chain_alloc(&chain,1024*1024);

	SVUT_ASSERT_SAME(blocs[1],nblocs[0]);
	SVUT_ASSERT_SAME(blocs[2],nblocs[1]);
	SVUT_ASSERT_SAME(blocs[3],nblocs[2]);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_region_entry_1(void )
{
	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);

	void * bloc = sctk_alloc_chain_alloc(&chain,1024*1024);

	SVUT_ASSERT_SAME((char*)TEST_REAL_HEAP_BASE + sizeof(sctk_alloc_chunk_header_large) + sizeof(sctk_alloc_macro_bloc),bloc);

	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);
	SVUT_ASSERT_SAME(&chain,sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc->chain);

	sctk_alloc_chain_free(&chain,bloc);

	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_region_entry_2(void )
{
	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE)->macro_bloc);
	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE)->macro_bloc);

	void * bloc = sctk_alloc_chain_alloc(&chain,2*SCTK_MACRO_BLOC_SIZE);

	SVUT_ASSERT_SAME((char*)TEST_REAL_HEAP_BASE + sizeof(sctk_alloc_chunk_header_large) + sizeof(sctk_alloc_macro_bloc),bloc);

	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE)->macro_bloc);
	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE)->macro_bloc);
	SVUT_ASSERT_SAME(&chain,sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE)->macro_bloc->chain);
	SVUT_ASSERT_SAME(&chain,sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE)->macro_bloc->chain);

	sctk_alloc_chain_free(&chain,bloc);

	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);
	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((char*)TEST_REAL_HEAP_BASE+SCTK_MACRO_BLOC_SIZE)->macro_bloc);
}

/************************* FUNCTION ************************/
void TestFullAllocChain::test_region_entry_3(void )
{
	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);

	void * bloc = sctk_alloc_chain_alloc(&chain,1024*1024);

	SVUT_ASSERT_SAME((char*)TEST_REAL_HEAP_BASE + sizeof(sctk_alloc_chunk_header_large) + sizeof(sctk_alloc_macro_bloc),bloc);

	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);
	SVUT_ASSERT_SAME(&chain,sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc->chain);

	sctk_alloc_chain_free(&chain,bloc);

	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry((void*)TEST_REAL_HEAP_BASE)->macro_bloc);
}

SVUT_REGISTER_STANDELONE(TestFullAllocChain);
