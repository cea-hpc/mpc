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
#include "sctk_alloc_inlined.h"
/************************** USING **************************/
using namespace svUnitTest;

/************************** CONSTS *************************/
#define BUFFER_SIZE (2*1024*1024UL)
#define BUFFER_TOTAL_SIZE (BUFFER_SIZE+16)
#define SIZE_FOR_CHUNK_256 (256-LARGE_HEADER)
#define SIZE_FOR_CHUNK_512 (512-LARGE_HEADER)
#define GET_RAND(min,max) (min + ((float)max * (rand() / (RAND_MAX + 1.0))))
#define RAND_COMBO_SIZE 256

/*************************** CLASS *************************/
class TestUserAllocChain : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_sctk_alloc_chain_user_init(void);
		void test_sctk_alloc_chain_free_1(void);
		void test_sctk_alloc_chain_free_2(void);
		void test_sctk_alloc_chain_alloc_1(void);
		void test_sctk_alloc_chain_alloc_2(void);
		void test_sctk_alloc_chain_alloc_3(void);
		void test_sctk_alloc_chain_alloc_4(void);
		void test_sctk_alloc_chain_alloc_5(void);
		void test_sctk_alloc_chain_alloc_6(void);
		void test_sctk_alloc_chain_rand_combo(void);
		void test_sctk_alloc_chain_can_destroy_1(void);
		void test_sctk_alloc_chain_can_destroy_2(void);
		void test_alloc_0o(void);
		void test_free_bad_address(void);
		void test_init_no_buffer(void);
		void test_rfq_purge_1(void);
		void test_rfq_purge_32(void);
		void test_non_aligned_end(void);
		void test_non_aligned_start(void);
	private:
		sctk_alloc_chain chain;
		char buffer[BUFFER_TOTAL_SIZE];
};

/************************* FUNCTION ************************/
void TestUserAllocChain::testMethodsRegistration (void)
{
	setTestCaseName("TestUserAllocChain");
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_user_init);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_alloc_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_alloc_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_alloc_3);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_alloc_4);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_alloc_5);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_alloc_6);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_free_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_free_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_rand_combo);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_can_destroy_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_can_destroy_2);
	SVUT_REG_TEST_METHOD(test_alloc_0o);
	SVUT_REG_TEST_METHOD(test_free_bad_address);
	SVUT_REG_TEST_METHOD(test_init_no_buffer);
	SVUT_REG_TEST_METHOD(test_rfq_purge_1);
	SVUT_REG_TEST_METHOD(test_rfq_purge_32);
	SVUT_REG_TEST_METHOD(test_non_aligned_end);
	SVUT_REG_TEST_METHOD(test_non_aligned_start);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::setUp (void)
{
	//SCTK_ALLOC_SPY_HOOK(__sctk_alloc_spy_chain_reset());
	sctk_alloc_chain_user_init(&chain,buffer,BUFFER_TOTAL_SIZE,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::tearDown (void)
{
	sctk_alloc_chain_destroy(&chain,true);
	sctk_alloc_region_del_all();
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_user_init(void )
{
	SVUT_ASSERT_SAME(buffer,chain.base_addr);
	SVUT_ASSERT_SAME(buffer+BUFFER_TOTAL_SIZE,chain.end_addr);
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(chain.pool.free_lists));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_empty(chain.pool.free_lists+SCTK_ALLOC_NB_FREE_LIST-7));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(chain.pool.free_lists+SCTK_ALLOC_NB_FREE_LIST-6));
	SVUT_ASSERT_SAME(buffer+sizeof(sctk_alloc_macro_bloc),chain.pool.free_lists[SCTK_ALLOC_NB_FREE_LIST-7].next);
	sctk_alloc_chunk_header_large * header = (sctk_alloc_chunk_header_large * )(buffer+sizeof(sctk_alloc_macro_bloc));
	SVUT_ASSERT_EQUAL(BUFFER_SIZE - sizeof(sctk_alloc_macro_bloc),sctk_alloc_get_chunk_header_large_size(header));
}


/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_alloc_1(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	sctk_alloc_vchunk chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);

	ptr = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_EQUAL(buffer+256+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_alloc_2(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,BUFFER_SIZE);
	SVUT_ASSERT_NULL(ptr);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_alloc_3(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	sctk_alloc_vchunk chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);

	ptr = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_EQUAL(buffer+256+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);
	
	ptr = sctk_alloc_chain_alloc(&chain,BUFFER_SIZE-512);
	SVUT_ASSERT_NULL(ptr);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_alloc_4(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	sctk_alloc_vchunk chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);

	ptr = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+256+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);

	ptr = sctk_alloc_chain_alloc(&chain,BUFFER_SIZE-512-LARGE_HEADER-sizeof(sctk_alloc_macro_bloc));
	SVUT_ASSERT_NOT_NULL(ptr);
	chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(BUFFER_SIZE-512-sizeof(sctk_alloc_macro_bloc),sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+512+LARGE_HEADER + sizeof(sctk_alloc_macro_bloc),ptr);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_alloc_5(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,BUFFER_SIZE-LARGE_HEADER-sizeof(sctk_alloc_macro_bloc));
	SVUT_ASSERT_NOT_NULL(ptr);
	sctk_alloc_vchunk chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(BUFFER_SIZE-sizeof(sctk_alloc_macro_bloc),sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+LARGE_HEADER+sizeof(sctk_alloc_macro_bloc),ptr);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_alloc_6(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,12);
	SVUT_ASSERT_NOT_NULL(ptr);
	sctk_alloc_vchunk chunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
	SVUT_ASSERT_EQUAL(32u,sctk_alloc_get_size(chunk));
	SVUT_ASSERT_SAME(buffer+LARGE_HEADER+sizeof(sctk_alloc_macro_bloc),ptr);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_free_1(void )
{
	sctk_alloc_chunk_header_large * header = (sctk_alloc_chunk_header_large * )(buffer + sizeof(sctk_alloc_macro_bloc));
	SVUT_ASSERT_EQUAL(BUFFER_SIZE - sizeof(sctk_alloc_macro_bloc),sctk_alloc_get_chunk_header_large_size(header));
	
	void * ptr = sctk_alloc_chain_alloc(&chain,256);

	SVUT_ASSERT_EQUAL(256+LARGE_HEADER,sctk_alloc_get_chunk_header_large_size(header));
	
	sctk_alloc_chain_free(&chain,ptr);

	SVUT_ASSERT_EQUAL(BUFFER_SIZE-sizeof(sctk_alloc_macro_bloc),sctk_alloc_get_chunk_header_large_size(header));
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_free_2(void )
{
	sctk_alloc_vchunk vchunk;
	void * ptr1 = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	void * ptr2 = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);

	SVUT_ASSERT_SAME(buffer+LARGE_HEADER+sizeof(sctk_alloc_macro_bloc),ptr1);
	SVUT_ASSERT_SAME(buffer+256+LARGE_HEADER+sizeof(sctk_alloc_macro_bloc),ptr2);

	vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr1);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(vchunk));
	
	sctk_alloc_chain_free(&chain,ptr1);

	vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr1);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(vchunk));
	
	ptr1 = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);

	vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr1);
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_SAME(buffer+LARGE_HEADER+sizeof(sctk_alloc_macro_bloc),ptr1);

	sctk_alloc_chain_free(&chain,ptr1);
	ptr1 = sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_512);
	SVUT_ASSERT_SAME(buffer+512+LARGE_HEADER+sizeof(sctk_alloc_macro_bloc),ptr1);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_rand_combo(void )
{
	sctk_size_t size;
	sctk_alloc_vchunk vchunk;
	int pos;
	void * ptr[RAND_COMBO_SIZE];
	srand(0);

	memset(ptr,0,sizeof(ptr));

	for (int i = 0 ; i < 40960 ; ++i)
	{
		pos = (int)GET_RAND(0,RAND_COMBO_SIZE);
		size =(size_t) GET_RAND(0,2*BUFFER_SIZE/RAND_COMBO_SIZE);
		SVUT_ASSERT_TRUE(pos < RAND_COMBO_SIZE);
		if (ptr[pos] != NULL)
			sctk_alloc_chain_free(&chain,ptr[pos]);
		ptr[pos] = sctk_alloc_chain_alloc(&chain,size);
		if (ptr[pos] != NULL)
		{
			vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr[pos]);
			if (sctk_alloc_get_size(vchunk) < size+sizeof(sctk_alloc_chunk_header_large)) abort();
			SVUT_ASSERT_TRUE(sctk_alloc_get_size(vchunk) >= size+sizeof(sctk_alloc_chunk_header_large));
		}
	}

	for (int i = 0 ; i < RAND_COMBO_SIZE ; ++i)
		if (ptr[i] != NULL)
			sctk_alloc_chain_free(&chain,ptr[i]);

	SVUT_ASSERT_SAME(buffer+sizeof(sctk_alloc_macro_bloc),chain.pool.free_lists[SCTK_ALLOC_NB_FREE_LIST-7].next);
	sctk_alloc_chunk_header_large * header = (sctk_alloc_chunk_header_large * )(buffer+sizeof(sctk_alloc_macro_bloc));
	SVUT_ASSERT_EQUAL(BUFFER_SIZE-sizeof(sctk_alloc_macro_bloc),sctk_alloc_get_chunk_header_large_size(header));
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_can_destroy_1(void )
{
	SVUT_ASSERT_TRUE(sctk_alloc_chain_can_destroy(&chain));
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_sctk_alloc_chain_can_destroy_2(void )
{
	sctk_alloc_chain_alloc(&chain,SIZE_FOR_CHUNK_256);
	SVUT_ASSERT_FALSE(sctk_alloc_chain_can_destroy(&chain));
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_alloc_0o(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,0);
	SVUT_ASSERT_NULL(res);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_free_bad_address(void )
{
	char * res = (char*)calloc(256,1);
	sctk_alloc_chain_free(&chain,res+32);
	free(res);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_init_no_buffer(void )
{
	sctk_alloc_chain local_chain;
	sctk_alloc_chain_user_init(&local_chain,NULL,0,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);

	for (int i = 0 ; i < SCTK_ALLOC_NB_FREE_LIST ; ++i)
		SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(local_chain.pool.free_lists+i));

	SVUT_ASSERT_NULL(sctk_alloc_chain_alloc(&local_chain,42));

	sctk_alloc_chain_destroy(&local_chain,true);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_rfq_purge_1(void )
{
	void * ptr = sctk_alloc_chain_alloc(&chain,256);
	sctk_alloc_rfq_register(&chain.rfq,ptr);

	sctk_alloc_chain_purge_rfq(&chain);

	void * ptr2 = sctk_alloc_chain_alloc(&chain,256);
	SVUT_ASSERT_SAME(ptr,ptr2);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_rfq_purge_32(void )
{
	void * ptr[32];
	void * ptr2[32];
	for (int i = 0 ; i < 32 ; ++i)
	{
		ptr[i] = sctk_alloc_chain_alloc(&chain,256);
		sctk_alloc_rfq_register(&chain.rfq,ptr[i]);
	}

	sctk_alloc_chain_purge_rfq(&chain);

	for (int i = 0 ; i < 32 ; ++i)
	{
		SVUT_SET_CONTEXT("iteration",i);
		ptr2[i] = sctk_alloc_chain_alloc(&chain,256);
		SVUT_ASSERT_SAME(ptr[i],ptr2[i]);
	}
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_non_aligned_end(void)
{
	void * buffer = malloc(4096);
	void * ptr[1024];
	sctk_alloc_chain chain;

	//setup with non aligned size
	sctk_alloc_chain_user_init(&chain,buffer,4095,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);

	//try to allocate all
	for (int i = 0 ; i < 1024 ; i++)
		ptr[i] = sctk_alloc_chain_alloc(&chain,8);
	SVUT_ASSERT_NULL(ptr[1023]);

	//now free
	for (int i = 0 ; i < 1024 ; i++)
		sctk_alloc_chain_free(&chain,ptr[i]);
	
	free(buffer);
}

/************************* FUNCTION ************************/
void TestUserAllocChain::test_non_aligned_start(void)
{
	void * buffer = malloc(4096);
	void * ptr[1024];
	sctk_alloc_chain chain;

	//setup with non aligned size
	sctk_alloc_chain_user_init(&chain,(char*)buffer+1,4095,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);

	//try to allocate all
	for (int i = 0 ; i < 1024 ; i++)
		ptr[i] = sctk_alloc_chain_alloc(&chain,8);
	SVUT_ASSERT_NULL(ptr[1023]);

	//now free
	for (int i = 0 ; i < 1024 ; i++)
		sctk_alloc_chain_free(&chain,ptr[i]);

	free(buffer);
}

SVUT_REGISTER_STANDELONE(TestUserAllocChain);
