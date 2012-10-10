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
#include "test_helper.h"

/************************** USING **************************/
using namespace svUnitTest;

/************************** CONSTS *************************/
#define TEST_REAL_HEAP_BASE (SCTK_ALLOC_HEAP_BASE)
#define TEST_REAL_HEAP_SIZE (SCTK_ALLOC_HEAP_SIZE - SCTK_ALLOC_PAGE_SIZE)

/*************************** CLASS *************************/
class TestMMSourceDefault : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_types(void);
		void test_consts(void);
		void test_init(void);
		void test_request_mm_1(void);
		void test_request_mm_2(void);
		void test_request_mm_oom(void);
		void test_free_mm_1(void);
		void test_cleanup(void);
		void test_get_macro_bloc(void);
		void test_bug_seq_1(void);
		void test_bug_seq_2(void);
		void test_bug_seq_3(void);
		void test_mem_opti_seq_1(void);
	private:
		sctk_alloc_mm_source_default source;
};

/************************* FUNCTION ************************/
void TestMMSourceDefault::testMethodsRegistration (void)
{
	setTestCaseName("TestMMSourceDefault");
	SVUT_REG_TEST_METHOD(test_types);
	SVUT_REG_TEST_METHOD(test_consts);
	SVUT_REG_TEST_METHOD(test_init);
	SVUT_REG_TEST_METHOD(test_request_mm_1);
	SVUT_REG_TEST_METHOD(test_request_mm_2);
	SVUT_REG_TEST_METHOD(test_request_mm_oom);
	SVUT_REG_TEST_METHOD(test_free_mm_1);
	SVUT_REG_TEST_METHOD(test_cleanup);
	SVUT_REG_TEST_METHOD(test_get_macro_bloc);
	SVUT_REG_TEST_METHOD(test_bug_seq_1);
	SVUT_REG_TEST_METHOD(test_bug_seq_2);
	SVUT_REG_TEST_METHOD(test_bug_seq_3);
	SVUT_REG_TEST_METHOD(test_mem_opti_seq_1);
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::setUp (void)
{
	#ifdef _WIN32
		void *res = VirtualAlloc((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		assert(res == (void*)SCTK_ALLOC_HEAP_BASE);
	#endif
	sctk_alloc_mm_source_default_init(&source,SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE);
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::tearDown (void)
{
	sctk_alloc_mm_source_default_cleanup(&source.source);
	#ifdef _WIN32
		VirtualFree((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RELEASE);
	#endif
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_types(void )
{
	struct sctk_alloc_mm_source_default * ptr = NULL;
	struct sctk_alloc_macro_bloc * mbloc = NULL;
	struct sctk_alloc_free_macro_bloc * freembloc = NULL;
	SVUT_ASSERT_SAME(ptr,&ptr->source);
	SVUT_ASSERT_SAME(mbloc,&mbloc->header);
	SVUT_ASSERT_SAME(freembloc,&freembloc->header);
	SVUT_ASSERT_EQUAL(0u,sizeof(sctk_alloc_macro_bloc)%16);
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_init(void )
{
	SVUT_ASSERT_SAME((void*)SCTK_ALLOC_HEAP_BASE,source.heap_addr);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_HEAP_SIZE,source.heap_size);
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(source.pool.free_lists+14));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_empty(source.pool.free_lists+15));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(source.pool.free_lists+16));
	for (int i = 0 ; i < SCTK_ALLOC_NB_FREE_LIST ; ++i)
	{
		SVUT_SET_CONTEXT("iteration",i);
		SVUT_SET_CONTEXT("bloc_size",source.pool.alloc_free_sizes[i]);
		if (i!=15)
			SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(source.pool.free_lists+i));
	}

	struct sctk_alloc_macro_bloc* bloc = (sctk_alloc_macro_bloc*)TEST_REAL_HEAP_BASE;
	SVUT_ASSERT_EQUAL(TEST_REAL_HEAP_SIZE,sctk_alloc_get_chunk_header_large_size(&bloc->header));
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_consts(void )
{
	SVUT_ASSERT_EQUAL(0u,SCTK_ALLOC_HEAP_BASE % SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_EQUAL(0u,SCTK_ALLOC_HEAP_SIZE % SCTK_MACRO_BLOC_SIZE);
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_free_mm_1(void )
{
	struct sctk_alloc_macro_bloc* bloc = sctk_alloc_mm_source_default_request_memory(&source.source,4*1024*1024);
	SVUT_ASSERT_SAME((void*)TEST_REAL_HEAP_BASE,bloc);
	SVUT_ASSERT_EQUAL(4*1024*1024ul,sctk_alloc_get_chunk_header_large_size(&bloc->header));

	//try to access to check if mmap has been done.
	char * ptr = (char*)bloc + 20*SCTK_ALLOC_PAGE_SIZE;
	*ptr = 'z';

	sctk_alloc_mm_source_default_free_memory(&source.source,bloc);
	SVUT_ASSERT_EQUAL(TEST_REAL_HEAP_SIZE,sctk_alloc_get_chunk_header_large_size(&bloc->header));
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_request_mm_1(void )
{
	struct sctk_alloc_macro_bloc* bloc = sctk_alloc_mm_source_default_request_memory(&source.source,4*1024*1024);
	SVUT_ASSERT_SAME((void*)TEST_REAL_HEAP_BASE,bloc);
	SVUT_ASSERT_EQUAL(4*1024*1024ul,sctk_alloc_get_chunk_header_large_size(&bloc->header));

	//try to access to check if mmap has been done.
	char * ptr = (char*)bloc + 20*SCTK_ALLOC_PAGE_SIZE;
	*ptr = 'z';
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_request_mm_2(void )
{
	struct sctk_alloc_macro_bloc* bloc = sctk_alloc_mm_source_default_request_memory(&source.source,4*1024*1024);
	SVUT_ASSERT_SAME((void*)TEST_REAL_HEAP_BASE,bloc);
	SVUT_ASSERT_EQUAL(4*1024*1024ul,sctk_alloc_get_chunk_header_large_size(&bloc->header));

	//try to access to check if mmap has been done.
	char * ptr = (char*)bloc + 20*SCTK_ALLOC_PAGE_SIZE;
	*ptr = 'z';
	
	bloc = sctk_alloc_mm_source_default_request_memory(&source.source,4*1024*1024);
	SVUT_ASSERT_SAME((char*)TEST_REAL_HEAP_BASE+4*1024*1024ul,bloc);
	SVUT_ASSERT_EQUAL(4*1024*1024ul,sctk_alloc_get_chunk_header_large_size(&bloc->header));

	//try to access to check if mmap has been done.
	ptr = (char*)bloc + 20*SCTK_ALLOC_PAGE_SIZE;
	*ptr = 'z';
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_request_mm_oom(void )
{
	struct sctk_alloc_macro_bloc* bloc = sctk_alloc_mm_source_default_request_memory(&source.source,2*SCTK_ALLOC_HEAP_SIZE);
	SVUT_ASSERT_NULL(bloc);
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_cleanup(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestMMSourceDefault::test_get_macro_bloc(void )
{
	SVUT_ASSERT_SAME((char*)SCTK_ALLOC_HEAP_BASE,sctk_alloc_get_macro_bloc((char*)SCTK_ALLOC_HEAP_BASE + 512));
	SVUT_ASSERT_SAME((char*)SCTK_ALLOC_HEAP_BASE,sctk_alloc_get_macro_bloc((char*)SCTK_ALLOC_HEAP_BASE));
	SVUT_ASSERT_SAME((char*)SCTK_ALLOC_HEAP_BASE+SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_macro_bloc((char*)SCTK_ALLOC_HEAP_BASE + SCTK_MACRO_BLOC_SIZE));
	SVUT_ASSERT_SAME((char*)SCTK_ALLOC_HEAP_BASE+2*SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_macro_bloc((char*)SCTK_ALLOC_HEAP_BASE + 2*SCTK_MACRO_BLOC_SIZE + 512));
}

/************************* FUNCTION ************************/
//Bug identified in version 318a72fb7d39d04ade5e11f565f09b051e61e033
//error on mapping status when splitting a non mapped macro bloc
void TestMMSourceDefault::test_bug_seq_1(void )
{
	struct sctk_alloc_macro_bloc * blocs[5];
	blocs[0] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[4] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);

	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[1]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[2]);
// 	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[3]);

	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	//the next line create a bug
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
// 	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
}

/************************* FUNCTION ************************/
//Bug identified in version 318a72fb7d39d04ade5e11f565f09b051e61e033
//error on mapping status when splitting a non mapped macro bloc
void TestMMSourceDefault::test_bug_seq_2(void )
{
	struct sctk_alloc_macro_bloc * blocs[5];
	blocs[0] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[4] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);

	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[1]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[2]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[3]);

	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	//the next line create a bug
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
}

/************************* FUNCTION ************************/
//Bug identified in version 318a72fb7d39d04ade5e11f565f09b051e61e033
//error on mapping status when splitting a non mapped macro bloc
void TestMMSourceDefault::test_bug_seq_3(void )
{
	struct sctk_alloc_macro_bloc * blocs[8];
	blocs[0] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[4] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);

	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[1]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[2]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[3]);

	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[5] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	
	blocs[6] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[7] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
}

/************************* FUNCTION ************************/
//Valid some memory optimisation pattern for macro bloc resuse.
void TestMMSourceDefault::test_mem_opti_seq_1(void )
{
	struct sctk_alloc_macro_bloc * blocs[5];
	struct sctk_alloc_macro_bloc * nblocs[3];
	blocs[0] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[3] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	blocs[4] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);

	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[1]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[2]);
	sctk_alloc_mm_source_default_free_memory(&source.source,blocs[3]);

	nblocs[0] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	nblocs[1] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);
	nblocs[2] = sctk_alloc_mm_source_default_request_memory(&source.source,2*1024*1024);

	SVUT_ASSERT_SAME(blocs[1],nblocs[0]);
	SVUT_ASSERT_SAME(blocs[2],nblocs[1]);
	SVUT_ASSERT_SAME(blocs[3],nblocs[2]);
}


SVUT_REGISTER_STANDELONE(TestMMSourceDefault);
