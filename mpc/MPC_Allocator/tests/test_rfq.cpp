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
#include "test_helper.h"

/************************** USING **************************/
using namespace svUnitTest;

/************************** CONSTS *************************/
static char * TEST_BUFFER[16*1024*1024];
static const int TEST_BLOC_SIZE = 64;


/*************************** CLASS *************************/
class TestAllocRFQ : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_init(void);
		void test_insert_1(void);
		void test_insert_2(void);
		void test_empty_true(void);
		void test_empty_false(void);
		void test_threaded_insert(void);
		void test_threaded_empty(void);
		void test_export_empty(void);
		void test_export_full(void);
		void test_threaded_export(void);

		sctk_alloc_rfq rfq;
		sctk_alloc_chain chain;
};

/************************* FUNCTION ************************/
void TestAllocRFQ::testMethodsRegistration (void)
{
	setTestCaseName("TestAllocRFQ");
	SVUT_REG_TEST_METHOD(test_init);
	SVUT_REG_TEST_METHOD(test_insert_1);
	SVUT_REG_TEST_METHOD(test_insert_2);
	SVUT_REG_TEST_METHOD(test_empty_true);
	SVUT_REG_TEST_METHOD(test_empty_false);
	SVUT_REG_TEST_METHOD(test_threaded_insert);
	SVUT_REG_TEST_METHOD(test_export_empty);
	SVUT_REG_TEST_METHOD(test_export_full);
	SVUT_REG_TEST_METHOD(test_threaded_export);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::setUp (void)
{
	sctk_alloc_rfq_init(&rfq);
	sctk_alloc_chain_user_init(&chain,TEST_BUFFER,sizeof(TEST_BUFFER),SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::tearDown (void)
{
	//to avoid propagating errors we force a cleanup
	sctk_atomics_store_ptr(&rfq.queue.head,NULL);
	sctk_atomics_store_ptr(&rfq.queue.tail,NULL);
	sctk_alloc_rfq_destroy(&rfq);
	sctk_alloc_chain_destroy(&chain,true);
	sctk_alloc_region_del_all();
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_init(void )
{
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));
	//SVUT_ASSERT_EQUAL(0,sctk_alloc_spinlock_trylock(&rfq.lock));
	//sctk_alloc_spinlock_unlock(&rfq.lock);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_insert_1(void )
{
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	sctk_alloc_rfq_entry * ptr = (sctk_alloc_rfq_entry * )sctk_alloc_chain_alloc(&chain,256);
	sctk_alloc_rfq_register(&rfq,ptr);

	SVUT_ASSERT_SAME(ptr,sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_SAME(ptr,sctk_atomics_load_ptr(&rfq.queue.tail));
	SVUT_ASSERT_NULL(ptr->entry.next);
	SVUT_ASSERT_SAME(ptr,ptr->ptr);

	sctk_alloc_chain_free(&chain,ptr);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_insert_2(void )
{
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	sctk_alloc_rfq_entry * ptr1 = (sctk_alloc_rfq_entry * )sctk_alloc_chain_alloc(&chain,256);
	sctk_alloc_rfq_entry * ptr2 = (sctk_alloc_rfq_entry * )sctk_alloc_chain_alloc(&chain,256);
	sctk_alloc_rfq_register(&rfq,ptr1);

	SVUT_ASSERT_SAME(ptr1,sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_SAME(ptr1,sctk_atomics_load_ptr(&rfq.queue.tail));
	SVUT_ASSERT_NULL(ptr1->entry.next);
	SVUT_ASSERT_SAME(ptr1,ptr1->ptr);

	sctk_alloc_rfq_register(&rfq,ptr2);

	SVUT_ASSERT_SAME(ptr1,sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_SAME(ptr2,sctk_atomics_load_ptr(&rfq.queue.tail));
	SVUT_ASSERT_NULL(ptr2->entry.next);
	SVUT_ASSERT_SAME(ptr2,ptr2->ptr);
	SVUT_ASSERT_SAME(ptr2,ptr1->entry.next);

	sctk_alloc_chain_free(&chain,ptr1);
	sctk_alloc_chain_free(&chain,ptr2);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_empty_true(void )
{
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	SVUT_ASSERT_TRUE(sctk_alloc_rfq_empty(&rfq));
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_empty_false(void )
{
	sctk_alloc_rfq_entry * ptr = (sctk_alloc_rfq_entry * )sctk_alloc_chain_alloc(&chain,256);
	sctk_alloc_rfq_register(&rfq,ptr);

	SVUT_ASSERT_NOT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NOT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	SVUT_ASSERT_FALSE(sctk_alloc_rfq_empty(&rfq));

	sctk_alloc_chain_free(&chain,ptr);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_export_empty(void )
{
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	SVUT_ASSERT_NULL(sctk_alloc_rfq_extract(&rfq));

	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_export_full(void )
{
	sctk_alloc_rfq_entry * ptr = (sctk_alloc_rfq_entry * )sctk_alloc_chain_alloc(&chain,256);
	sctk_alloc_rfq_register(&rfq,ptr);
	
	SVUT_ASSERT_NOT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NOT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	SVUT_ASSERT_NOT_NULL(sctk_alloc_rfq_extract(&rfq));

	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.head));
	SVUT_ASSERT_NULL(sctk_atomics_load_ptr(&rfq.queue.tail));

	sctk_alloc_chain_free(&chain,ptr);
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_threaded_insert(void )
{
	//prepare blocs
	int nb = sizeof(TEST_BUFFER) / TEST_BLOC_SIZE / 2;
	void ** ptr = new void * [nb];

	//allocate blocs
	for (int i = 0 ; i < nb ; ++i)
		ptr[i] = sctk_alloc_chain_alloc(&chain,TEST_BLOC_SIZE);

	//parallel register
	#pragma omp parallel for
	for (int i = 0 ; i < nb ; ++i)
		sctk_alloc_rfq_register(&rfq,ptr[i]);

	//check
	int cnt = 0;
	sctk_alloc_rfq_entry * volatile curr = (sctk_alloc_rfq_entry * volatile)sctk_atomics_load_ptr(&rfq.queue.head);
	while (curr != NULL)
	{
		curr = (sctk_alloc_rfq_entry * volatile)curr->entry.next;
		cnt++;
	}
	SVUT_ASSERT_EQUAL(nb,cnt);

	delete ptr;
}

/************************* FUNCTION ************************/
void TestAllocRFQ::test_threaded_export(void )
{
	//prepare blocs
	int nb = sizeof(TEST_BUFFER) / TEST_BLOC_SIZE / 2;
	void ** ptr = new void * [sizeof(TEST_BUFFER) / TEST_BLOC_SIZE / 2];
	int nbth;
	#pragma omp parallel
	{
		#pragma omp single
		nbth = omp_get_num_threads();
	}
	int cnt = 0;
	int nbprod = nbth - 1;

	#ifdef _WIN32
		MARK_AS_KNOWN_ERROR("Need to support OpenMP");
	#endif
	
	SVUT_ASSERT_GT(1,nbth);
	SVUT_ASSERT_GT(0,nbprod);

	int base = nb / nbprod;

	//allocate blocs
	for (int i = 0 ; i < nb ; ++i)
		ptr[i] = sctk_alloc_chain_alloc(&chain,TEST_BLOC_SIZE);

	//parallel register
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		if ( id < nbprod )
		{
			//producer
			for (int i = 0 ; i < base ; ++i)
				sctk_alloc_rfq_register(&rfq,ptr[id * base + i]);
		} else {
			while( cnt != nbprod * base )
			{
				sctk_alloc_rfq_entry * volatile curr = sctk_alloc_rfq_extract(&rfq);
				while (curr != NULL)
				{
					curr = (sctk_alloc_rfq_entry * volatile)curr->entry.next;
					cnt++;
				}
			}
		}
	}

	//check
	SVUT_ASSERT_EQUAL(base * nbprod,cnt);

	delete ptr;
}

SVUT_REGISTER_STANDELONE(TestAllocRFQ);
