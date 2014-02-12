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
#include <stdio.h>
#include <sctk_alloc_posix.h>
#ifndef _WIN32
	#include <omp.h>
#endif
#include "sctk_alloc_inlined.h"
#include "test_helper.h"

/************************** USING **************************/
using namespace svUnitTest;

/************************** CONSTS *************************/
#define TEST_REAL_HEAP_BASE (SCTK_ALLOC_HEAP_BASE)
#define TEST_REAL_HEAP_SIZE (SCTK_ALLOC_HEAP_SIZE - SCTK_PAGE_SIZE)
#define TEST_MAX_THREAD 1024
#define TEST_NB_ALLOC 32*4096

/*************************** CLASS *************************/
class TestPosixMalloc : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_init(void);
		void test_simple_malloc_free(void);
		void test_simple_threaded_malloc(void);
		void test_simple_threaded_calloc(void);
		void test_simple_threaded_free(void);
		void test_massiv_threaded_malloc(void);
		void test_massiv_threaded_calloc(void);
		void test_massiv_threaded_free(void);
		void test_remote_free(void);
		void test_simple_memalign(void);
		void test_realloc_1(void);
		void test_realloc_2(void);
		void test_realloc_3(void);
		void test_realloc_4(void);
		void test_realloc_null(void);
		void test_realloc_size_0(void);
		void test_realloc_interchain(void);
};

/************************* FUNCTION ************************/
void TestPosixMalloc::testMethodsRegistration (void)
{
	setTestCaseName("TestPosixMalloc");
	SVUT_REG_TEST_METHOD(test_init);
// 	SVUT_REG_TEST_METHOD(test_simple_malloc_free);
// 	SVUT_REG_TEST_METHOD(test_simple_threaded_malloc);
// 	SVUT_REG_TEST_METHOD(test_simple_threaded_calloc);
// 	SVUT_REG_TEST_METHOD(test_simple_threaded_free);
	SVUT_REG_TEST_METHOD(test_massiv_threaded_malloc);
	SVUT_REG_TEST_METHOD(test_massiv_threaded_calloc);
	SVUT_REG_TEST_METHOD(test_massiv_threaded_free);
	SVUT_REG_TEST_METHOD(test_remote_free);
	SVUT_REG_TEST_METHOD(test_simple_memalign);
	SVUT_REG_TEST_METHOD(test_realloc_1);
	SVUT_REG_TEST_METHOD(test_realloc_2);
	SVUT_REG_TEST_METHOD(test_realloc_3);
	SVUT_REG_TEST_METHOD(test_realloc_4);
	SVUT_REG_TEST_METHOD(test_realloc_null);
	SVUT_REG_TEST_METHOD(test_realloc_size_0);
	SVUT_REG_TEST_METHOD(test_realloc_interchain);
	
}

/************************* FUNCTION ************************/
void TestPosixMalloc::setUp (void)
{
}

/************************* FUNCTION ************************/
void TestPosixMalloc::tearDown (void)
{
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_init(void)
{
	sctk_free(NULL);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_simple_malloc_free(void )
{
	void * ptr = sctk_malloc(256);
	SVUT_ASSERT_NOT_NULL(ptr);
	sctk_free(ptr);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_simple_threaded_malloc(void )
{
	void * ptr[TEST_MAX_THREAD];
	
	int nb = omp_get_num_threads();
	SVUT_ASSERT_LT(TEST_MAX_THREAD,nb);
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		ptr[id] = sctk_malloc(256);
	}

	for (int i = 0 ; i < nb ; ++i)
	{
		SVUT_ASSERT_NOT_NULL(ptr[i]);
		sctk_free(ptr[i]);
	}

	//flush rfq
	#pragma omp parallel
	{
		sctk_free(NULL);
	}
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_simple_threaded_calloc(void )
{
	SVUT_ASSERT_TODO("TODO");
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_simple_threaded_free(void )
{
	void * ptr[TEST_MAX_THREAD];

	int nb = omp_get_num_threads();
	SVUT_ASSERT_LT(TEST_MAX_THREAD,nb);

	for (int i = 0 ; i < nb ; ++i)
	{
		ptr[i] = sctk_malloc(256);
		SVUT_ASSERT_NOT_NULL(ptr[i]);
	}

	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		sctk_free(ptr[id]);
	}

	//flush rfq
	#pragma omp parallel
	{
		sctk_free(NULL);
	}
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_massiv_threaded_malloc(void )
{
	void ** ptr = new void * [TEST_MAX_THREAD * TEST_NB_ALLOC];
	int nb;

	#pragma omp parallel
	#pragma omp single
	nb = omp_get_num_threads();
	SVUT_ASSERT_LT(TEST_MAX_THREAD,nb);
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
			ptr[id*TEST_NB_ALLOC+j] = sctk_malloc(256);
	}

	for (int i = 0 ; i < nb ; ++i)
		for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
		{
			SVUT_ASSERT_NOT_NULL(ptr[i*TEST_NB_ALLOC+j]);
			sctk_free(ptr[i*TEST_NB_ALLOC+j]);
		}

	#pragma omp parallel
	{
		sctk_free(NULL);
	}

	for (int i = 0 ; i < nb ; ++i)
		for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
		{
			SVUT_SET_CONTEXT("Thread",i);
			SVUT_SET_CONTEXT("Bloc",j);
			SVUT_SET_CONTEXT("BlocAddr",ptr[i*TEST_NB_ALLOC+j]);
			SVUT_ASSERT_NULL(sctk_alloc_region_get_entry(ptr[i*TEST_NB_ALLOC+j])->macro_bloc);
		}

	delete[] ptr;
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_massiv_threaded_free(void )
{
	void ** ptr = new void * [TEST_MAX_THREAD * TEST_NB_ALLOC];
	int nb;

	memset(ptr,0,sizeof(void*)*TEST_MAX_THREAD * TEST_NB_ALLOC);

	#pragma omp parallel
	#pragma omp single
	nb = omp_get_num_threads();
	SVUT_ASSERT_LT(TEST_MAX_THREAD,nb);

	for (int i = 0 ; i < nb ; ++i)
		for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
		{
			ptr[i*TEST_NB_ALLOC+j]=sctk_malloc(256);
			SVUT_ASSERT_NOT_NULL(ptr[i*TEST_NB_ALLOC+j]);
			SVUT_ASSERT_NOT_NULL(sctk_alloc_region_get_entry(ptr[i*TEST_NB_ALLOC+j]));
		}

	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
			sctk_free(ptr[id*TEST_NB_ALLOC+j]);
	}
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_massiv_threaded_calloc(void )
{
	SVUT_ASSERT_TODO("TODO");
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_remote_free(void )
{
	#ifdef _WIN32
		MARK_AS_KNOWN_ERROR("Need OpenMP");
	#endif

	void ** ptr = new void * [TEST_NB_ALLOC];

	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry(ptr[0])->macro_bloc);
	
	#pragma omp parallel num_threads(2)
	{
		int id = omp_get_thread_num();
		//allocate on thread 0
		if (id == 0)
		{
			for (int i = 0 ; i < TEST_NB_ALLOC ; ++i)
				ptr[i] = sctk_malloc(256);
		}
		#pragma omp barrier
		//remote free bloc blocs of thread 0 from thread 1
		if (id == 1)
		{
			for (int i = 0 ; i < TEST_NB_ALLOC ; ++i)
				sctk_free(ptr[i]);
		}
	}

	//check that we didn't readly done the free
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr[0]);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,vchunk->state);
	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_get_entry(ptr[0]));
	
	//start real free on the alocation chain of thread 1 by doing an action.
	#pragma omp parallel num_threads(2)
	{
		int id = omp_get_thread_num();
		if (id == 0)
			sctk_free(NULL);
	}

	//check if freed
	SVUT_ASSERT_NULL(sctk_alloc_region_get_entry(ptr[0])->macro_bloc);
	
	delete[] ptr;
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_simple_memalign(void )
{
	void * ptr;
	sctk_size_t align[] = {16,32,64,128,256,1024,2048,4096,2*1024*1024,4*1024*1024,8*1024*1024};
	for (unsigned int i = 0 ; i < sizeof(align)/sizeof(sctk_size_t) ; ++i)
	{
		SVUT_SET_CONTEXT("boundary",align[i]);
		ptr = sctk_memalign(align[i],256);
		SVUT_ASSERT_EQUAL(0ul,(sctk_addr_t)ptr%align[i]);
		sctk_free(ptr);
	}
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_1(void)
{
	unsigned char * ptr = (unsigned char*)sctk_malloc(64);

	for (int i = 0 ; i < 64 ; i++)
		ptr[i] = i;

	ptr = (unsigned char *)sctk_realloc(ptr,128);

	for (int i = 0 ; i < 64 ; i++)
		SVUT_ASSERT_EQUAL(i,ptr[i]);

	sctk_free(ptr);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_2(void)
{
	unsigned char * ptr = (unsigned char *)sctk_malloc((1024+512)*1024);

	for (int i = 0 ; i < (1024+512)*1024 ; i++)
		ptr[i] = i % 256;

	unsigned char * ptr2 = (unsigned char *)sctk_realloc(ptr,(1024+512+128)*1024);

	SVUT_ASSERT_SAME(ptr,ptr2);
	for (int i = 0 ; i < (1024+512)*1024 ; i++)
		SVUT_ASSERT_EQUAL(i%256,ptr2[i]);

	sctk_free(ptr2);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_3(void)
{
	unsigned char * ptr = (unsigned char *)sctk_malloc((1024+512)*1024);
	unsigned char * ptrtmp = (unsigned char *)sctk_malloc((1024+512)*1024);

	for (int i = 0 ; i < (1024+512)*1024 ; i++)
		ptr[i] = i % 256;

	unsigned char * ptr2 = (unsigned char *)sctk_realloc(ptr,(2*1024)*1024);

	SVUT_ASSERT_NOT_SAME(ptr,ptr2);
	for (int i = 0 ; i < (1024+512)*1024 ; i++)
		SVUT_ASSERT_EQUAL(i%256,ptr2[i]);

	sctk_free(ptrtmp);
	sctk_free(ptr2);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_4(void)
{
	unsigned char * ptr = (unsigned char*)sctk_realloc(NULL,128);

	for (int i = 0 ; i < 128 ; i++)
		ptr[i] = i;

	unsigned char * ptr2 = (unsigned char *)sctk_realloc(ptr,128);
	SVUT_ASSERT_SAME(ptr,ptr2);
	unsigned char * ptr3 = (unsigned char *)sctk_realloc(ptr,128);
	SVUT_ASSERT_SAME(ptr,ptr3);

	for (int i = 0 ; i < 128 ; i++)
		SVUT_ASSERT_EQUAL(i,ptr3[i]);

	sctk_free(ptr3);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_null(void)
{
	unsigned char * ptr = (unsigned char *)sctk_realloc(NULL,128);
	SVUT_ASSERT_NOT_NULL(ptr);
	for (int i = 0 ; i < 128; i++)
		ptr[i] = i % 256;
	sctk_free(ptr);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_size_0(void)
{
	sctk_malloc(64);
	unsigned char * ptr = (unsigned char *)sctk_malloc(128);
	SVUT_ASSERT_NOT_NULL(ptr);
	for (int i = 0 ; i < 128 ; i++)
		ptr[i] = i % 256;
	sctk_realloc(ptr,0);
	void * ptr2 = sctk_malloc(128);
	SVUT_ASSERT_SAME(ptr,ptr2);
	sctk_free(ptr2);
}

/************************* FUNCTION ************************/
void TestPosixMalloc::test_realloc_interchain(void)
{
	SVUT_ASSERT_TODO("todo");
}

SVUT_REGISTER_STANDELONE(TestPosixMalloc);
