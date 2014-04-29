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
#include <stdlib.h>
#ifdef _WIN32
	#include <intrin.h>
	#define memalign _aligned_malloc
#endif
#include <svUnitTest/svUnitTest.h>
#include <sctk_allocator.h>
#include <cstring>
#include <malloc.h>
#include "test_helper.h"
#include "sctk_alloc_inlined.h"
/************************** USING **************************/
using namespace svUnitTest;

/*************************** CLASS *************************/
class TestChunkPadding : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_struct_size(void);
		void test_sctk_alloc_chunk_body(void);
		void test_sctk_alloc_get_chunk_padded(void);
		void test_sctk_alloc_setup_chunk_padded_0(void);
		void test_sctk_alloc_setup_chunk_padded_1(void);
		void test_sctk_alloc_setup_chunk_padded_8(void);
		void test_sctk_alloc_setup_chunk_padded_16(void);
		void test_sctk_alloc_setup_chunk_padded_32(void);
		void test_sctk_alloc_setup_chunk_padded_64(void);
		void test_sctk_alloc_setup_chunk_padded_256(void);
		void test_sctk_alloc_setup_chunk_padded_1024(void);
		void test_sctk_alloc_setup_chunk_padded_2M(void);
		void test_sctk_alloc_setup_chunk_padded_4M(void);
		void test_sctk_alloc_get_chunk_unpadded_0(void);
		void test_sctk_alloc_get_chunk_unpadded_1(void);
		void test_sctk_alloc_get_chunk_unpadded_8(void);
		void test_sctk_alloc_get_chunk_unpadded_16(void);
		void test_sctk_alloc_get_chunk_unpadded_32(void);
		void test_sctk_alloc_get_chunk_unpadded_64(void);
		void test_sctk_alloc_get_chunk_unpadded_256(void);
		void test_sctk_alloc_get_chunk_unpadded_1024(void);
		void test_sctk_alloc_get_chunk_unpadded_2M(void);
		void test_sctk_alloc_get_chunk_unpadded_4M(void);
		void test_sctk_alloc_get_chunk_unpadded_bad(void);
		void test_sctk_alloc_unpadd_vchunk_8(void);
		void test_sctk_alloc_unpadd_vchunk_16(void);
		void test_sctk_alloc_unpadd_vchunk_32(void);
		void test_sctk_alloc_unpadd_vchunk_64(void);
		void test_sctk_alloc_unpadd_vchunk_256(void);
		void test_sctk_alloc_unpadd_vchunk_1024(void);
		void test_sctk_alloc_unpadd_vchunk_2M(void);
		void test_sctk_alloc_unpadd_vchunk_4M(void);
		
	private:
		void test_sctk_alloc_setup_chunk_padded(sctk_size_t boundary, sctk_size_t expected);
		void test_sctk_alloc_get_chunk_unpadded(sctk_size_t boundary);
		void test_sctk_alloc_unpad_vchunk(sctk_size_t boundary);

};

/************************* FUNCTION ************************/
void TestChunkPadding::testMethodsRegistration (void)
{
	setTestCaseName("TestChunkPadding");
	SVUT_REG_TEST_METHOD(test_struct_size);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_chunk_body);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_padded);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_0);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_8);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_16);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_32);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_64);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_256);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_1024);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_2M);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_setup_chunk_padded_4M);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_0);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_8);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_16);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_32);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_64);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_256);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_1024);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_2M);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_4M);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_chunk_unpadded_bad);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_8);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_16);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_32);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_64);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_256);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_1024);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_2M);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_unpadd_vchunk_4M);
}

/************************* FUNCTION ************************/
void TestChunkPadding::setUp (void)
{
}

/************************* FUNCTION ************************/
void TestChunkPadding::tearDown (void)
{
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_struct_size(void )
{
	SVUT_ASSERT_EQUAL(8ul,sizeof(sctk_alloc_chunk_header_padded));
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_chunk_body(void )
{
	struct sctk_alloc_chunk_header_padded chunk;
	sctk_alloc_set_chunk_header_padded_padding(&chunk,20);
	sctk_alloc_get_chunk_header_padded_info(&chunk)->state = SCTK_ALLOC_CHUNK_STATE_ALLOCATED;
	sctk_alloc_get_chunk_header_padded_info(&chunk)->type = SCTK_ALLOC_CHUNK_TYPE_PADDED;
	sctk_alloc_get_chunk_header_padded_info(&chunk)->unused_magik = SCTK_ALLOC_MAGIC_STATUS;

	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_padded_info(&chunk)->unused_magik);
	void * ptr = ((&chunk)+1);
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);

	SVUT_ASSERT_SAME((&chunk)+1,sctk_alloc_chunk_body(vchunk));
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_padded(void )
{
	struct sctk_alloc_chunk_header_padded chunk;
	sctk_alloc_set_chunk_header_padded_padding(&chunk,20);
	sctk_alloc_get_chunk_header_padded_info(&chunk)->state = SCTK_ALLOC_CHUNK_STATE_ALLOCATED;
	sctk_alloc_get_chunk_header_padded_info(&chunk)->type = SCTK_ALLOC_CHUNK_TYPE_PADDED;
	sctk_alloc_get_chunk_header_padded_info(&chunk)->unused_magik = SCTK_ALLOC_MAGIC_STATUS;

	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_padded_info(&chunk)->unused_magik);
	void * ptr = ((&chunk)+1);
	sctk_alloc_vchunk res = sctk_alloc_get_chunk((sctk_addr_t)ptr);

	SVUT_ASSERT_SAME(&chunk,sctk_alloc_get_padded(res));
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_0(void )
{
	test_sctk_alloc_get_chunk_unpadded(0);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_1(void )
{
	test_sctk_alloc_get_chunk_unpadded(1);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_8(void )
{
	test_sctk_alloc_get_chunk_unpadded(8);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_16(void )
{
	test_sctk_alloc_get_chunk_unpadded(16);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_32(void )
{
	test_sctk_alloc_get_chunk_unpadded(32);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_64(void )
{
	test_sctk_alloc_get_chunk_unpadded(64);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_256(void )
{
	test_sctk_alloc_get_chunk_unpadded(256);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_1024(void )
{
	test_sctk_alloc_get_chunk_unpadded(1024);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_2M(void )
{
	test_sctk_alloc_get_chunk_unpadded(2048*1024);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_4M(void )
{
	test_sctk_alloc_get_chunk_unpadded(4096*1024);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_0(void )
{
	test_sctk_alloc_setup_chunk_padded(0,0);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_1(void )
{
	test_sctk_alloc_setup_chunk_padded(1,0);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_8(void )
{
	test_sctk_alloc_setup_chunk_padded(8,4);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_16(void )
{
	test_sctk_alloc_setup_chunk_padded(16,4);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_32(void )
{
	test_sctk_alloc_setup_chunk_padded(32,4);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_64(void )
{
	test_sctk_alloc_setup_chunk_padded(64,36);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_1024(void )
{
	test_sctk_alloc_setup_chunk_padded(1024,996);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_2M(void )
{
	test_sctk_alloc_setup_chunk_padded(2048*1024,2*1024*1024-16-8-4);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_4M(void )
{
	test_sctk_alloc_setup_chunk_padded(4096*1024,4*1024*1024-16-8-4);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded_256(void )
{
	test_sctk_alloc_setup_chunk_padded(256,228);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_8(void )
{
	test_sctk_alloc_unpad_vchunk(8);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_16(void )
{
	test_sctk_alloc_unpad_vchunk(16);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_32(void )
{
	test_sctk_alloc_unpad_vchunk(32);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_64(void )
{
	test_sctk_alloc_unpad_vchunk(64);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_256(void )
{
	test_sctk_alloc_unpad_vchunk(256);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_1024(void )
{
	test_sctk_alloc_unpad_vchunk(1024);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_2M(void )
{
	test_sctk_alloc_unpad_vchunk(2048*1024);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpadd_vchunk_4M(void )
{
	test_sctk_alloc_unpad_vchunk(4096*1024);
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded(sctk_size_t boundary)
{
	SVUT_SET_CONTEXT("boundary",boundary);

	//setup buffer
	const sctk_size_t base_align_and_size = 8*1024*1024;
	char * rbuffer = (char*) memalign(base_align_and_size,base_align_and_size);
	char * buffer = rbuffer + 4;

	//setup std header
	memset(buffer,0,sizeof(buffer));
	sctk_alloc_vchunk vchunk = sctk_alloc_setup_chunk(buffer,base_align_and_size-4,NULL);
	SVUT_ASSERT_SAME(buffer, sctk_alloc_get_ptr(vchunk));

	//apply padding
	sctk_alloc_vchunk vchunkPad = sctk_alloc_setup_chunk_padded(vchunk,boundary);

	//unpad it
	void * ptr = sctk_alloc_chunk_body(vchunkPad);
	sctk_alloc_vchunk vchunkUnpad = sctk_alloc_get_chunk_unpadded((sctk_addr_t)ptr);

	//check
	SVUT_ASSERT_SAME(sctk_alloc_get_ptr(vchunk),sctk_alloc_get_ptr(vchunkUnpad));
	SVUT_ASSERT_SAME(vchunk,vchunkUnpad);
	SVUT_ASSERT_EQUAL(sctk_alloc_get_size(vchunk),sctk_alloc_get_size(vchunkUnpad));
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_unpad_vchunk(sctk_size_t boundary)
{
	SVUT_SET_CONTEXT("boundary",boundary);
	//setup buffer
	const sctk_size_t base_align_and_size = 8*1024*1024;
	char * rbuffer = (char*) memalign(base_align_and_size,base_align_and_size);
	char * buffer = rbuffer + 4;

	//setup std header
	memset(buffer,0,sizeof(buffer));
	sctk_alloc_vchunk vchunk = sctk_alloc_setup_chunk(buffer,base_align_and_size-4,NULL);
	SVUT_ASSERT_SAME(buffer, sctk_alloc_get_ptr(vchunk));

	//apply padding
	sctk_alloc_vchunk vchunkPad = sctk_alloc_setup_chunk_padded(vchunk,boundary);

	//unpad it
	sctk_alloc_vchunk vchunkUnpad = sctk_alloc_unpadd_vchunk(sctk_alloc_get_padded(vchunkPad));

	//check
	SVUT_ASSERT_SAME(sctk_alloc_get_ptr(vchunk),sctk_alloc_get_ptr(vchunkUnpad));
	SVUT_ASSERT_SAME(vchunk,vchunkUnpad);
	SVUT_ASSERT_EQUAL(sctk_alloc_get_size(vchunk),sctk_alloc_get_size(vchunkUnpad));
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_setup_chunk_padded(sctk_size_t boundary,sctk_size_t expected)
{
	const sctk_size_t base_align_and_size = 8*1024*1024;
	char * rbuffer = (char*)memalign(base_align_and_size,base_align_and_size);
	char * buffer = rbuffer + 4;
	sctk_alloc_vchunk vchunk;
	SVUT_SET_CONTEXT("boundary",boundary);
	SVUT_ASSERT_EQUAL(0ul,(sctk_size_t)rbuffer%(base_align_and_size));
	SVUT_ASSERT_EQUAL(4ul,(sctk_size_t)buffer%(base_align_and_size));

	//setup std header
	memset(buffer,0,sizeof(buffer));
	vchunk = sctk_alloc_setup_chunk(buffer,base_align_and_size-4,NULL);
	SVUT_ASSERT_SAME(buffer, sctk_alloc_get_ptr(vchunk));
	SVUT_SET_CONTEXT("init_align",(sctk_addr_t)sctk_alloc_chunk_body(vchunk)%(base_align_and_size));
	if (boundary != 0)
	{
		SVUT_SET_CONTEXT("init_relativ_align",(sctk_addr_t)sctk_alloc_chunk_body(vchunk)%boundary);
	}
	vchunk = sctk_alloc_setup_chunk_padded(vchunk,boundary);

	//check alignement
	if (boundary > 1)
	{
		SVUT_ASSERT_EQUAL(0ul,(sctk_addr_t)sctk_alloc_chunk_body(vchunk)%boundary);
		SVUT_SET_CONTEXT("final_align",(sctk_addr_t)sctk_alloc_chunk_body(vchunk)%(base_align_and_size));
		SVUT_SET_CONTEXT("final_relativ_align",(sctk_addr_t)sctk_alloc_chunk_body(vchunk)%boundary);
	}
	
	if (expected == 0)
	{
		SVUT_ASSERT_SAME(buffer, sctk_alloc_get_ptr(vchunk));
	} else {
		//calc with boundary
		buffer += sizeof(sctk_alloc_chunk_header_large);
		buffer += expected;

		struct sctk_alloc_chunk_header_padded * chunk = (sctk_alloc_chunk_header_padded*)(buffer);

		SVUT_ASSERT_EQUAL(expected,sctk_alloc_get_chunk_header_padded_padding(sctk_alloc_get_padded(vchunk)));
		SVUT_ASSERT_SAME(chunk,sctk_alloc_get_padded(vchunk));
		SVUT_ASSERT_EQUAL(expected,sctk_alloc_get_chunk_header_padded_padding(chunk));
		SVUT_ASSERT_EQUAL((int)SCTK_ALLOC_CHUNK_STATE_ALLOCATED,(int)sctk_alloc_get_chunk_header_padded_info(chunk)->state);
		SVUT_ASSERT_EQUAL((int)SCTK_ALLOC_CHUNK_TYPE_PADDED,(int)sctk_alloc_get_chunk_header_padded_info(chunk)->type);
		SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,sctk_alloc_get_chunk_header_padded_info(chunk)->unused_magik);
		SVUT_ASSERT_SAME(chunk,sctk_alloc_get_ptr(vchunk));

		//sctk_free(rbuffer);
	}
}

/************************* FUNCTION ************************/
void TestChunkPadding::test_sctk_alloc_get_chunk_unpadded_bad(void )
{
	sctk_alloc_vchunk vchunk = sctk_alloc_get_chunk_unpadded((sctk_addr_t)this+12);
	SVUT_ASSERT_NULL(vchunk);
}


SVUT_REGISTER_STANDELONE(TestChunkPadding);
