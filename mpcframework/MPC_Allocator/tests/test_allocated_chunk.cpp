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
#include <unistd.h>
#include <cstring>
#include "test_helper.h"
#include "sctk_alloc_inlined.h"

/************************** USING **************************/
using namespace svUnitTest;

/*************************** CLASS *************************/
class TestAllocatedChunk : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_typedef_size(void);
		void test_struct_size(void);
		void test_consts(void);
		void test_page_size(void);
		void test_header_info_addr_small(void);
		void test_header_info_addr_large(void);
		void test_magick_number_storage(void);
		void test_sctk_alloc_get_chunk_small(void);
		void test_sctk_alloc_get_chunk_large(void);
		void test_sctk_alloc_setup_chunk_small(void);
		void test_sctk_alloc_setup_chunk_large_1(void);
		void test_sctk_alloc_setup_chunk_large_2(void);
		void test_sctk_alloc_chunk_body(void);
		void test_sctk_alloc_calc_body_size(void);
		void test_sctk_alloc_calc_chunk_size(void);
		void test_header_info_type_storage(void);
};

/************************* FUNCTION ************************/
void TestAllocatedChunk::testMethodsRegistration (void)
{
	setTestCaseName("TestAllocatedChunk");
	SVUT_REG_TEST_METHOD(test_typedef_size);
	SVUT_REG_TEST_METHOD(test_struct_size);
	SVUT_REG_TEST_METHOD(test_consts);
	SVUT_REG_TEST_METHOD(test_page_size);
	SVUT_REG_TEST_METHOD(test_header_info_addr_large);
	SVUT_REG_TEST_METHOD(test_magick_number_storage);
	SVUT_REG_TEST_METHOD(test_header_info_type_storage);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_large);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_large_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_large_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chunk_body);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_calc_body_size);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_calc_chunk_size);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::setUp (void)
{
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::tearDown (void)
{
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_consts(void )
{
	SVUT_ASSERT_TRUE(SCTK_ALLOC_SMALL_CHUNK_SIZE < 256);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_page_size(void )
{
	#ifndef _WIN32
		SVUT_ASSERT_EQUAL(sysconf(_SC_PAGESIZE),SCTK_ALLOC_PAGE_SIZE);
	#else
		_SYSTEM_INFO pageSize;
		GetSystemInfo(&pageSize);
		SVUT_ASSERT_EQUAL(pageSize.dwPageSize,SCTK_ALLOC_PAGE_SIZE);
	#endif
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_header_info_addr_large(void )
{
	sctk_alloc_chunk_header_large chunk;
	sctk_addr_t ptr = (sctk_addr_t)&chunk;
	sctk_addr_t ptrInfo = (sctk_addr_t)sctk_alloc_get_chunk_header_large_info(&chunk);
	
	#ifndef _WIN32
		sctk_addr_t ptrAddr = (sctk_addr_t)&chunk.addr;
		SVUT_ASSERT_EQUAL(7u,ptrAddr-ptr);
	#endif
	
	SVUT_ASSERT_EQUAL(sizeof(sctk_alloc_chunk_header_large)-1,ptrInfo-ptr);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_typedef_size (void)
{
	SVUT_ASSERT_EQUAL(8lu , sizeof(sctk_size_t));
	SVUT_ASSERT_EQUAL(1lu , sizeof(sctk_short_size_t));
	SVUT_ASSERT_EQUAL(8lu , sizeof(sctk_addr_t));
	SVUT_ASSERT_EQUAL(sizeof(void*) , sizeof(sctk_addr_t));

	//check out C bool compatibility definition with C++ one
	SVUT_ASSERT_EQUAL(1lu, sizeof(bool));
	SVUT_ASSERT_EQUAL(1,(unsigned char)true);
	SVUT_ASSERT_EQUAL(0,(unsigned char)false);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_struct_size (void)
{
	SVUT_ASSERT_EQUAL(1lu , sizeof(sctk_alloc_chunk_info));
	SVUT_ASSERT_EQUAL(16lu, sizeof(sctk_alloc_chunk_header_large));
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_magick_number_storage ( void )
{
	sctk_alloc_chunk_info info;
	info.unused_magik = SCTK_ALLOC_MAGIC_STATUS;
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,(int)info.unused_magik);

	sctk_alloc_chunk_header_padded chunk_small;
	sctk_alloc_get_chunk_header_padded_info(&chunk_small)->unused_magik = SCTK_ALLOC_MAGIC_STATUS;
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_padded_info(&chunk_small)->unused_magik);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_header_info_type_storage(void )
{
	sctk_alloc_chunk_info info_large;
	sctk_alloc_chunk_info info_padded;

	info_large.type = SCTK_ALLOC_CHUNK_TYPE_LARGE;
	info_padded.type = SCTK_ALLOC_CHUNK_TYPE_PADDED;

	SVUT_ASSERT_EQUAL((int)SCTK_ALLOC_CHUNK_TYPE_LARGE,(int)info_large.type);
	SVUT_ASSERT_EQUAL((int)SCTK_ALLOC_CHUNK_TYPE_PADDED,(int)info_padded.type);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_sctk_alloc_get_chunk_large ( void )
{
	struct sctk_alloc_chunk_header_large chunk;
	sctk_alloc_set_chunk_header_large_size(&chunk, 20);
	sctk_alloc_get_chunk_header_large_info(&chunk)->state = SCTK_ALLOC_CHUNK_STATE_ALLOCATED;
	sctk_alloc_get_chunk_header_large_info(&chunk)->type = SCTK_ALLOC_CHUNK_TYPE_LARGE;
	sctk_alloc_get_chunk_header_large_info(&chunk)->unused_magik = SCTK_ALLOC_MAGIC_STATUS;

	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_large_info(&chunk)->unused_magik);
	void * ptr = ((&chunk)+1);
	sctk_alloc_vchunk res = sctk_alloc_get_chunk((sctk_addr_t)ptr);

	SVUT_ASSERT_EQUAL(20ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(sctk_alloc_get_chunk_header_large_info(&chunk),res);
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_sctk_alloc_setup_chunk_large_1 ( void )
{
	char buffer[32];
	struct sctk_alloc_chunk_header_large * chunk = (sctk_alloc_chunk_header_large*)buffer;

	memset(buffer,0,sizeof(buffer));
	sctk_alloc_setup_chunk(buffer,32*1024,NULL);

	SVUT_ASSERT_EQUAL(32*1024ul,sctk_alloc_get_chunk_header_large_size(chunk));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,sctk_alloc_get_chunk_header_large_info(chunk)->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,sctk_alloc_get_chunk_header_large_info(chunk)->type);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_large_info(chunk)->unused_magik);
	SVUT_ASSERT_EQUAL(0,buffer[sizeof(sctk_alloc_chunk_header_large)]);
	SVUT_ASSERT_EQUAL(0u,sctk_alloc_get_chunk_header_large_previous_size(chunk));
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_sctk_alloc_setup_chunk_large_2 ( void )
{
	char buffer[32];
	struct sctk_alloc_chunk_header_large * chunk = (sctk_alloc_chunk_header_large*)buffer;

	memset(buffer,0,sizeof(buffer));
	sctk_alloc_setup_chunk(buffer,32*1024,buffer-32);

	SVUT_ASSERT_EQUAL(32*1024ul,sctk_alloc_get_chunk_header_large_size(chunk));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,sctk_alloc_get_chunk_header_large_info(chunk)->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,sctk_alloc_get_chunk_header_large_info(chunk)->type);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_large_info(chunk)->unused_magik);
	SVUT_ASSERT_EQUAL(0,buffer[sizeof(sctk_alloc_chunk_header_large)]);
	SVUT_ASSERT_EQUAL(32u,sctk_alloc_get_chunk_header_large_previous_size(chunk));
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_sctk_alloc_chunk_body(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_sctk_alloc_calc_body_size(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestAllocatedChunk::test_sctk_alloc_calc_chunk_size(void )
{
	SVUT_ASSERT_TODO("todo");
}

SVUT_REGISTER_STANDELONE(TestAllocatedChunk);
