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
#include <cstring>
#include "sctk_alloc_inlined.h"
#include "test_helper.h"
#include "sctk_alloc_light_mm_source.h"

/************************** MACROS *************************/
#define TEST_MACRO_BLOC_BODY (2*1024u*1024-sizeof(struct sctk_alloc_macro_bloc) - sizeof(struct sctk_alloc_chunk_header_large))

/************************** USING **************************/
using namespace svUnitTest;

/*************************** CLASS *************************/
class TestDefaultAllocChain : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
		void test_sctk_alloc_chain_default_init_1(void);
		void test_sctk_alloc_chain_default_init_2(void);
		void test_alloc_64o(void);
		void test_alloc_4ko(void);
		void test_alloc_1Mo(void);
		void test_alloc_2Mo(void);
		void test_alloc_8Mo(void);
		void test_free_64o(void);
		void test_free_4ko(void);
		void test_free_1Mo(void);
		void test_free_2Mo(void);
		void test_free_8Mo(void);
		void test_alloc_0o(void);
		void test_alloc_seq_1(void);
	protected:
		sctk_alloc_chain chain;
		sctk_alloc_mm_source_light source;
};

/************************* FUNCTION ************************/
void TestDefaultAllocChain::testMethodsRegistration (void)
{
	setTestCaseName("TestDefaultAllocChain");
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_default_init_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chain_default_init_2);
	SVUT_REG_TEST_METHOD(test_alloc_64o);
	SVUT_REG_TEST_METHOD(test_alloc_4ko);
	SVUT_REG_TEST_METHOD(test_alloc_1Mo);
	SVUT_REG_TEST_METHOD(test_alloc_2Mo);
	SVUT_REG_TEST_METHOD(test_alloc_8Mo);
	SVUT_REG_TEST_METHOD(test_free_64o);
	SVUT_REG_TEST_METHOD(test_free_4ko);
	SVUT_REG_TEST_METHOD(test_free_1Mo);
	SVUT_REG_TEST_METHOD(test_free_2Mo);
	SVUT_REG_TEST_METHOD(test_free_8Mo);
	SVUT_REG_TEST_METHOD(test_alloc_0o);
	SVUT_REG_TEST_METHOD(test_alloc_seq_1);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::setUp (void)
{
	//SCTK_ALLOC_SPY_HOOK(__sctk_alloc_spy_chain_reset());
	/*
	#ifdef _WIN32
		void *res = VirtualAlloc((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		assert(res == (void*)SCTK_ALLOC_HEAP_BASE);
	#endif
	*/
	//sctk_alloc_mm_source_light_init(&source,SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE);
	sctk_alloc_mm_source_light_init(&source,-1,SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
	sctk_alloc_chain_default_init(&chain,&source.source,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::tearDown (void)
{
	sctk_alloc_chain_destroy(&chain,true);
	//sctk_alloc_mm_source_light_cleanup(&source.source);
	/*
	#ifdef _WIN32
		VirtualFree((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RELEASE);
	#endif
	*/
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_sctk_alloc_chain_default_init_1(void )
{
	SVUT_ASSERT_NULL(chain.base_addr);
	SVUT_ASSERT_NULL(chain.end_addr);
	SVUT_ASSERT_SAME(&source,chain.source);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_sctk_alloc_chain_default_init_2(void )
{
	sctk_alloc_chain chain;
	sctk_alloc_chain_default_init(&chain,NULL,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);
	SVUT_ASSERT_NULL(chain.source);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_64o(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,64);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(64u+16,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_4ko(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,4096);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(4096u+16,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_1Mo(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,1024*1024);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(TEST_MACRO_BLOC_BODY,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_2Mo(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,2*1024*1024);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(TEST_MACRO_BLOC_BODY+SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_8Mo(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,8*1024*1024);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(TEST_MACRO_BLOC_BODY+4*SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_free_64o(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,64);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(64u+16,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));

	sctk_alloc_chain_free(&chain,res);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_free_4ko(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,4096);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(4096u+16,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));

	sctk_alloc_chain_free(&chain,res);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_free_1Mo(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,1024);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(1024u+16,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));

	sctk_alloc_chain_free(&chain,res);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_free_2Mo(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,2048);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(2048u+16,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));

	sctk_alloc_chain_free(&chain,res);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_free_8Mo(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,8*1024*1024);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)res);
	SVUT_ASSERT_NOT_NULL(res);
	SVUT_ASSERT_EQUAL(TEST_MACRO_BLOC_BODY+4*SCTK_MACRO_BLOC_SIZE,sctk_alloc_get_size(vchunk));
	SVUT_ASSERT_EQUAL(0ul,sctk_alloc_get_prev_size(vchunk));

	sctk_alloc_chain_free(&chain,res);
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_seq_1(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestDefaultAllocChain::test_alloc_0o(void )
{
	void * res = sctk_alloc_chain_alloc(&chain,0);
	SVUT_ASSERT_NULL(res);
}

SVUT_REGISTER_STANDELONE(TestDefaultAllocChain);
