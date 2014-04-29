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

/************************** HEADERS ************************/
#include <svUnitTest/svUnitTest.h>
#include <sctk_allocator.h>
#include "test_helper.h"
#include "sctk_alloc_inlined.h"

/************************** USING **************************/
using namespace svUnitTest;

/*************************** CLASS *************************/
class TestThreadPool : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_consts(void);
		void test_sctk_alloc_thread_pool_init(void);
		void test_sctk_alloc_get_free_list(void);
		void test_sctk_alloc_free_list_insert_raw_32(void);
		void test_sctk_alloc_free_list_insert_raw_64(void);
		void test_sctk_alloc_free_list_remove(void);
		void test_sctk_alloc_find_free_chunk_1(void);
		void test_sctk_alloc_find_free_chunk_2(void);
		void test_sctk_alloc_find_free_chunk_3(void);
		void test_sctk_alloc_get_next_list_1(void);
		void test_sctk_alloc_get_next_list_2(void);
		void test_sctk_alloc_get_next_list_3(void);
		void test_sctk_alloc_free_list_empty(void);
		void test_sctk_alloc_split_free_bloc_1(void);
		void test_sctk_alloc_split_free_bloc_2(void);
		void test_sctk_alloc_split_free_bloc_3(void);
		void test_sctk_alloc_get_next_chunk_small(void);
		void test_sctk_alloc_get_next_chunk_large(void);
		void test_sctk_alloc_merge_chunk_1(void);
		void test_sctk_alloc_merge_chunk_2(void);
		void test_sctk_alloc_merge_chunk_3(void);
		void test_sctk_alloc_merge_chunk_4(void);
		void test_sctk_alloc_merge_chunk_5(void);
		void test_sctk_alloc_merge_chunk_6(void);
		void test_sctk_alloc_merge_chunk_7(void);
		void test_sctk_alloc_merge_chunk_8(void);
		void sctk_alloc_free_chunk_range(void);
		void test_sctk_alloc_split_free_bloc_align_1(void);
		void test_sctk_alloc_split_free_bloc_align_2(void);
		void test_sctk_alloc_split_free_bloc_align_3(void);
		void test_sctk_alloc_free_list_is_not_empty_quick(void);
		void test_sctk_alloc_free_list_mark_empty(void);
		void test_sctk_alloc_free_list_mark_non_empty(void);
		void test_sctk_alloc_find_first_free_non_empty_list(void);
	private:
		sctk_thread_pool pool;
};

/************************* FUNCTION ************************/
void TestThreadPool::testMethodsRegistration (void)
{
	setTestCaseName("TestThreadPool");
	SVUT_REG_TEST_METHOD(test_consts);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_thread_pool_init);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_free_list);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_insert_raw_32);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_insert_raw_64);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_remove);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_find_free_chunk_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_find_free_chunk_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_find_free_chunk_3);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_next_list_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_next_list_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_next_list_3);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_empty);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_split_free_bloc_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_split_free_bloc_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_split_free_bloc_3);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_next_chunk_small);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_get_next_chunk_large);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_3);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_4);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_5);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_6);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_7);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_merge_chunk_8);
	SVUT_REG_TEST_METHOD(sctk_alloc_free_chunk_range);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_split_free_bloc_align_1);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_split_free_bloc_align_2);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_split_free_bloc_align_3);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_is_not_empty_quick);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_mark_empty);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_free_list_mark_non_empty);
	SVUT_REG_TEST_METHOD(test_sctk_alloc_find_first_free_non_empty_list);
}

/************************* FUNCTION ************************/
void TestThreadPool::setUp (void)
{
	sctk_alloc_thread_pool_init(&pool,NULL);
}

/************************* FUNCTION ************************/
void TestThreadPool::tearDown (void)
{
}

/************************* FUNCTION ************************/
void TestThreadPool::test_consts(void )
{
	//until support of small blocks in free list
	SVUT_ASSERT_TRUE(SCTK_ALLOC_SMALL_CHUNK_SIZE <= 32);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_thread_pool_init(void )
{
	sctk_thread_pool pool;

	sctk_alloc_thread_pool_init(&pool,NULL);
	for (int i = 0 ; i < SCTK_ALLOC_NB_FREE_LIST ; ++i)
	{
		SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(pool.free_lists+i));
		SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+i));
	}
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_get_free_list(void )
{
	sctk_alloc_free_list_t * lst;

	lst = sctk_alloc_get_free_list(&pool,64);
	SVUT_ASSERT_EQUAL(1,lst - pool.free_lists);

	lst = sctk_alloc_get_free_list(&pool,1024);
	SVUT_ASSERT_EQUAL(31,lst - pool.free_lists);

	lst = sctk_alloc_get_free_list(&pool,1025);
	SVUT_ASSERT_EQUAL(32,lst - pool.free_lists);

	lst = sctk_alloc_get_free_list(&pool,1023);
	SVUT_ASSERT_EQUAL(31,lst - pool.free_lists);

	lst = sctk_alloc_get_free_list(&pool,1024*1024*1024UL);
	SVUT_ASSERT_EQUAL(43,lst - pool.free_lists);

	lst = sctk_alloc_get_free_list(&pool,-1UL);
	SVUT_ASSERT_EQUAL(43,lst - pool.free_lists);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_insert_raw_32(void )
{
	char buffer[32];
	sctk_alloc_free_list_t * flist = pool.free_lists;

	//precheck
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,flist));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(flist));
	SVUT_ASSERT_SAME(flist->prev,flist->next);
	SVUT_ASSERT_SAME(flist,flist->prev);

	//action
	sctk_alloc_free_list_insert_raw(&pool,buffer,32,NULL);

	//postcheck
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,flist));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_empty(flist));
	SVUT_ASSERT_SAME(flist->prev,flist->next);
	SVUT_ASSERT_SAME(buffer,flist->prev);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_FREE,(int)sctk_alloc_get_chunk_header_large_info(&flist->prev->header)->state);
	SVUT_ASSERT_EQUAL(0u,sctk_alloc_get_chunk_header_large_previous_size(&flist->prev->header));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_insert_raw_64(void )
{
	char buffer[64];
	sctk_alloc_free_list_t * flist = pool.free_lists+1;

	//precheck
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,flist));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(flist));
	SVUT_ASSERT_SAME(flist->prev,flist->next);
	SVUT_ASSERT_SAME(flist,flist->prev);

	//action
	sctk_alloc_free_list_insert_raw(&pool,buffer,64,NULL);

	//postcheck
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,flist));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_empty(flist));
	SVUT_ASSERT_SAME(flist->prev,flist->next);
	SVUT_ASSERT_SAME(buffer,flist->prev);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_FREE,(int)sctk_alloc_get_chunk_header_large_info(&flist->prev->header)->state);
	SVUT_ASSERT_EQUAL(0u,sctk_alloc_get_chunk_header_large_previous_size(&flist->prev->header));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_remove(void )
{
	struct sctk_alloc_free_chunk * chunk = NULL;
	sctk_alloc_free_list_t * flist = pool.free_lists;
	char buffer[32];

	//setup
	sctk_alloc_free_list_insert_raw(&pool,buffer,32,NULL);

	//precheck
	SVUT_ASSERT_SAME(flist->prev,buffer);

	//action
	chunk = flist->prev;
	sctk_alloc_free_list_remove(&pool,chunk);

	//postcheck
	SVUT_ASSERT_SAME(buffer,chunk);
	SVUT_ASSERT_NULL(chunk->next);
	SVUT_ASSERT_NULL(chunk->prev);
	SVUT_ASSERT_SAME(flist->prev,flist->next);
	SVUT_ASSERT_SAME(flist,flist->prev);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,(int)sctk_alloc_get_chunk_header_large_info(&chunk->header)->state);
	SVUT_ASSERT_EQUAL(0u,sctk_alloc_get_chunk_header_large_previous_size(&chunk->header));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_find_free_chunk_1(void )
{
	struct sctk_alloc_free_chunk * chunk = NULL;

	chunk = sctk_alloc_find_free_chunk(&pool,128);
	SVUT_ASSERT_NULL(chunk);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_find_free_chunk_2(void )
{
	struct sctk_alloc_free_chunk * chunk = NULL;
	char  buffer[128];

	sctk_alloc_free_list_insert_raw(&pool,buffer,128,NULL);
	chunk = sctk_alloc_find_free_chunk(&pool,64);

	SVUT_ASSERT_SAME(buffer,chunk);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,(int)sctk_alloc_get_chunk_header_large_info(&chunk->header)->state);

	chunk = sctk_alloc_find_free_chunk(&pool,64);
	SVUT_ASSERT_NULL(chunk);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_find_free_chunk_3(void )
{
	struct sctk_alloc_free_chunk * chunk = NULL;
	char  buffer[128];

	sctk_alloc_free_list_insert_raw(&pool,buffer,128,NULL);
	chunk = sctk_alloc_find_free_chunk(&pool,128);

	SVUT_ASSERT_NULL(chunk);

	chunk = sctk_alloc_find_free_chunk(&pool,64);
	SVUT_ASSERT_SAME(buffer,chunk);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,(int) sctk_alloc_get_chunk_header_large_info(&chunk->header)->state);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_empty(void )
{
	sctk_alloc_free_list_t lst;
	sctk_alloc_free_list_t elem;

	lst.next = &lst;
	lst.prev = &lst;
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(&lst));

	lst.next = &elem;
	lst.prev = &elem;
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_empty(&lst));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_get_next_list_1(void )
{
	sctk_alloc_free_list_t * flist = pool.free_lists;
	flist = sctk_alloc_get_next_list(&pool,flist);
	SVUT_ASSERT_SAME(pool.free_lists+1,flist);
	flist = sctk_alloc_get_next_list(&pool,flist);
	SVUT_ASSERT_SAME(pool.free_lists+2,flist);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_get_next_list_2(void )
{
	sctk_alloc_free_list_t * flist = pool.free_lists+pool.nb_free_lists-2;
	flist = sctk_alloc_get_next_list(&pool,flist);
	SVUT_ASSERT_SAME(pool.free_lists+pool.nb_free_lists-1,flist);
	flist = sctk_alloc_get_next_list(&pool,flist);
	SVUT_ASSERT_NULL(flist);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_get_next_list_3(void )
{
	sctk_alloc_free_list_t * flist = NULL;
	flist = sctk_alloc_get_next_list(&pool,flist);
	SVUT_ASSERT_NULL(flist);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_1(void )
{
	char buffer[7*64+16];

	//setup 3 allocated chunk + our one + 3 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 1*64,64,buffer+0*64);
	sctk_alloc_setup_chunk(buffer+ 2*64,64,buffer+1*64);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 3*64,64,buffer+2*64);
	sctk_alloc_vchunk chunkNext = sctk_alloc_setup_chunk(buffer+ 4*64,64,buffer+3*64);
	sctk_alloc_setup_chunk(buffer+ 5*64,64,buffer+4*64);
	sctk_alloc_setup_chunk(buffer+ 6*64,64,buffer+5*64);
	sctk_alloc_create_stopper(buffer+7*64,buffer + 6*64);

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(64ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+3*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(chunkNext));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_2(void )
{
	char buffer[7*64+16];

	//setup 3 allocated chunk + our one + 1 free + 2 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 1*64,64,buffer+0*64);
	sctk_alloc_setup_chunk(buffer+ 2*64,64,buffer+1*64);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 3*64,64,buffer+2*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 4*64,64,buffer+3*64);
	sctk_alloc_vchunk chunkNext = sctk_alloc_setup_chunk(buffer+ 5*64,64,buffer+4*64);
	sctk_alloc_setup_chunk(buffer+ 6*64,64,buffer+5*64);
	sctk_alloc_create_stopper(buffer+7*64,buffer + 6*64);

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(128ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+3*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(128u,sctk_alloc_get_prev_size(chunkNext));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_3(void )
{
	char buffer[7*64+16];

	//setup 3 allocated chunk + our one + 1 free + 2 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 1*64,64,buffer + 0*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 2*64,64,buffer + 1*64);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 3*64,64,buffer + 2*64);
	sctk_alloc_vchunk chunkNext = sctk_alloc_setup_chunk(buffer+ 4*64,64,buffer + 3*64);
	sctk_alloc_setup_chunk(buffer+ 5*64,64,buffer + 4*64);
	sctk_alloc_setup_chunk(buffer+ 6*64,64,buffer + 5*64);
	sctk_alloc_create_stopper(buffer+7*64,buffer + 6*64);

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(128ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+2*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(128u,sctk_alloc_get_prev_size(chunkNext));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_4(void )
{
	char buffer[7*64+16];

	//setup 3 allocated chunk + our one + 1 free + 2 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 1*64,64,buffer + 0*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 2*64,64,buffer + 1*64);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 3*64,64,buffer + 2*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 4*64,64,buffer + 3*64);
	sctk_alloc_vchunk chunkNext = sctk_alloc_setup_chunk(buffer+ 5*64,64,buffer + 4*64);
	sctk_alloc_setup_chunk(buffer+ 6*64,64,buffer + 5*64);
	sctk_alloc_create_stopper(buffer+7*64,buffer + 6*64);

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(3*64ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+2*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(3*64u,sctk_alloc_get_prev_size(chunkNext));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_5(void )
{
	char buffer[7*64+16];

	//setup 3 allocated chunk + our one + 1 free + 2 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 1*64,64,buffer + 0*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 2*64,64,buffer + 1*64);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 3*64,64,buffer + 2*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 4*64,64,buffer + 3*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 5*64,64,buffer + 4*64);
	sctk_alloc_vchunk chunkNext = sctk_alloc_setup_chunk(buffer+ 6*64,64,buffer + 5*64);
	sctk_alloc_create_stopper(buffer+7*64,buffer + 6*64);

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(5*64ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+1*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(5*64u,sctk_alloc_get_prev_size(chunkNext));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_6(void )
{
	char buffer[7*64+16];
	
	//setup 3 allocated chunk + our one + 1 free + 2 others allocated
	
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 0*64,64,NULL);
	sctk_alloc_vchunk first = sctk_alloc_get_chunk((sctk_addr_t)buffer+sizeof(sctk_alloc_chunk_header_large));
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 1*64,64,buffer + 0*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 2*64,64,buffer + 1*64);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 3*64,64,buffer + 2*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 4*64,64,buffer + 3*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 5*64,64,buffer + 4*64);
	sctk_alloc_free_list_insert_raw(&pool,buffer+ 6*64,64,buffer + 5*64);
	sctk_alloc_create_stopper(buffer+7*64,buffer + 6*64);
	sctk_alloc_chunk_header_large * stopper = (sctk_alloc_chunk_header_large *)(buffer+7*64);

	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_empty(pool.free_lists+1));

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(7*64ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+0*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_empty(pool.free_lists+1));
	SVUT_ASSERT_EQUAL(0u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(7*64u,sctk_alloc_get_chunk_header_large_previous_size(stopper));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_7(void )
{
	char buffer[7*64];

	//setup 3 allocated chunk + our one + 3 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_vchunk chunkNext = sctk_alloc_setup_chunk(buffer+ 1*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 2*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 3*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 4*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 5*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 6*64,64,NULL);
	sctk_alloc_vchunk chunk = first;

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(64ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+0*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
	SVUT_ASSERT_EQUAL(0u,sctk_alloc_get_prev_size(res));
	SVUT_ASSERT_EQUAL(64u,sctk_alloc_get_prev_size(chunkNext));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_merge_chunk_8(void )
{
	char buffer[7*64+16];

	//setup 3 allocated chunk + our one + 3 others allocated
	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 1*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 2*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 3*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 4*64,64,NULL);
	sctk_alloc_setup_chunk(buffer+ 5*64,64,NULL);
	sctk_alloc_vchunk chunk = sctk_alloc_setup_chunk(buffer+ 6*64,64,NULL);
	sctk_alloc_create_stopper(buffer+7*64,NULL);

	sctk_alloc_vchunk res = sctk_alloc_merge_chunk(&pool,chunk,first,(sctk_addr_t)buffer+sizeof(buffer));

	SVUT_ASSERT_EQUAL(64ul,sctk_alloc_get_size(res));
	SVUT_ASSERT_SAME(buffer+6*64,sctk_alloc_get_large(res));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,res->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,res->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,res->type);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_get_next_chunk_large(void )
{
	char buffer[2*64];

	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer+ 0*64,64,NULL);
	sctk_alloc_vchunk second = sctk_alloc_setup_chunk(buffer+ 1*64,64,NULL);

	sctk_alloc_vchunk chunk = sctk_alloc_get_next_chunk(first);
	SVUT_ASSERT_EQUAL(second,chunk);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_split_free_bloc_1(void )
{
	//split in the middle
	char buffer[2*128+16];

	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer,256,NULL);
	sctk_alloc_create_stopper(buffer+256,buffer);

	SVUT_ASSERT_SAME(buffer,sctk_alloc_get_large(first));
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(first));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,first->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,first->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,first->type);
	
	sctk_alloc_vchunk second = sctk_alloc_split_free_bloc(&first,128-16);

	SVUT_ASSERT_SAME(buffer,sctk_alloc_get_large(first));
	SVUT_ASSERT_EQUAL(128u,sctk_alloc_get_size(first));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,first->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,first->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,first->type);

	SVUT_ASSERT_SAME(buffer+128,sctk_alloc_get_large(second));
	SVUT_ASSERT_EQUAL(128u,sctk_alloc_get_size(second));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,second->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,second->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,second->type);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_split_free_bloc_2(void )
{
	//split in the middle
	char buffer[2*128];

	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer,256,NULL);

	SVUT_ASSERT_SAME(buffer,sctk_alloc_get_large(first));
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(first));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,first->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,first->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,first->type);

	sctk_alloc_vchunk second = sctk_alloc_split_free_bloc(&first,256-32);

	SVUT_ASSERT_SAME(buffer,sctk_alloc_get_large(first));
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(first));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,first->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,first->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,first->type);

	SVUT_ASSERT_NULL(second);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_split_free_bloc_3(void )
{
	//split in the middle
	char buffer[2*128+16];

	sctk_alloc_vchunk first = sctk_alloc_setup_chunk(buffer,256,NULL);
	sctk_alloc_create_stopper(buffer+256,buffer);

	SVUT_ASSERT_SAME(buffer,sctk_alloc_get_large(first));
	SVUT_ASSERT_EQUAL(256u,sctk_alloc_get_size(first));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,first->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,first->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,first->type);

	sctk_alloc_vchunk second = sctk_alloc_split_free_bloc(&first,10);

	SVUT_ASSERT_SAME(buffer,sctk_alloc_get_large(first));
	SVUT_ASSERT_EQUAL(32u,sctk_alloc_get_size(first));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,first->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,first->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,first->type);

	SVUT_ASSERT_SAME(buffer+32,sctk_alloc_get_large(second));
	SVUT_ASSERT_EQUAL(256u-32u,sctk_alloc_get_size(second));
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_STATE_ALLOCATED,second->state);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_MAGIC_STATUS,second->unused_magik);
	SVUT_ASSERT_EQUAL(SCTK_ALLOC_CHUNK_TYPE_LARGE,second->type);
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_get_next_chunk_small(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestThreadPool::sctk_alloc_free_chunk_range(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_split_free_bloc_align_1(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_split_free_bloc_align_2(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_split_free_bloc_align_3(void )
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_is_not_empty_quick(void )
{
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+2));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+8));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+9));

	pool.free_list_status[2] = true;
	pool.free_list_status[9] = true;

	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+2));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+8));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+9));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_mark_empty(void )
{
	for (int i = 0 ; i < 32 ; ++i)
		pool.free_list_status[i] = true;
	
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+2));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+3));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+8));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+9));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+10));

	sctk_alloc_free_list_mark_empty(&pool,pool.free_lists+2);
	sctk_alloc_free_list_mark_empty(&pool,pool.free_lists+9);

	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+2));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+3));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+8));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+9));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+10));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_free_list_mark_non_empty(void )
{
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+2));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+3));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+8));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+9));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+10));

	sctk_alloc_free_list_mark_non_empty(&pool,pool.free_lists+2);
	sctk_alloc_free_list_mark_non_empty(&pool,pool.free_lists+9);

	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+1));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+2));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+3));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+8));
	SVUT_ASSERT_TRUE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+9));
	SVUT_ASSERT_FALSE(sctk_alloc_free_list_is_not_empty_quick(&pool,pool.free_lists+10));
}

/************************* FUNCTION ************************/
void TestThreadPool::test_sctk_alloc_find_first_free_non_empty_list(void )
{
	sctk_alloc_free_list_mark_non_empty(&pool,pool.free_lists+2);
	sctk_alloc_free_list_mark_non_empty(&pool,pool.free_lists+9);
	sctk_alloc_free_list_mark_non_empty(&pool,pool.free_lists+16);

	sctk_alloc_free_list_t * list1 = sctk_alloc_find_first_free_non_empty_list(&pool,pool.free_lists);
	sctk_alloc_free_list_t * list2 = sctk_alloc_find_first_free_non_empty_list(&pool,pool.free_lists+1);
	sctk_alloc_free_list_t * list3 = sctk_alloc_find_first_free_non_empty_list(&pool,pool.free_lists+2);
	sctk_alloc_free_list_t * list4 = sctk_alloc_find_first_free_non_empty_list(&pool,pool.free_lists+3);
	sctk_alloc_free_list_t * list5 = sctk_alloc_find_first_free_non_empty_list(&pool,pool.free_lists+10);
	sctk_alloc_free_list_t * list6 = sctk_alloc_find_first_free_non_empty_list(&pool,pool.free_lists+32);

	SVUT_ASSERT_EQUAL(2,list1-pool.free_lists);
	SVUT_ASSERT_EQUAL(2,list2-pool.free_lists);
	SVUT_ASSERT_EQUAL(2,list3-pool.free_lists);
	SVUT_ASSERT_EQUAL(9,list4-pool.free_lists);
	SVUT_ASSERT_EQUAL(16,list5-pool.free_lists);
	SVUT_ASSERT_NULL(list6);
}

SVUT_REGISTER_STANDELONE(TestThreadPool);
