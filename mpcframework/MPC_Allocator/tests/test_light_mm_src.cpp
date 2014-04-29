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
#include <sctk_alloc_config.h>
#include <sctk_alloc_light_mm_source.h>
#include <cstring>
#include <iostream>

extern "C"
{
#include <sctk_alloc_topology.h>
};
#include "test_helper.h"

/************************** USING **************************/
using namespace svUnitTest;
using namespace std;

/*************************** CLASS *************************/
class TestLightMMSrc : public svutTestCase
{
	public:
		TestLightMMSrc(const char * name,sctk_alloc_mm_source_light_flags mode);
		void testMethodsRegistration();
		virtual void setUp(void);
		virtual void tearDown(void);
	protected:
		void test_init(void);
		void test_setup_free_macro_bloc(void);
		void test_reg_in_cache_1(void);
		void test_reg_in_cache_2(void);
		void test_find_in_cache_ok(void);
		void test_find_in_cache_on_more(void);
		void test_find_in_cache_empty(void);
		void test_find_in_cache_to_large(void);
		void test_request_mem_1(void);
		void test_request_mem_2(void);
		void test_request_mem_3(void);
		void test_free_mem(void);
		void test_insert_segemnt(void);
		void test_new_segment(void);
		void test_force_binding(void);
		void test_size_counter_inc(void);
		void test_size_counter_dec(void);
		sctk_alloc_mm_source_light light_source;
		sctk_alloc_mm_source_light_flags mode;
		/*
SCTK_STATIC struct sctk_alloc_macro_bloc* sctk_alloc_mm_source_light_request_memory(struct sctk_alloc_mm_source* source, sctk_size_t size);
SCTK_STATIC void sctk_alloc_mm_source_light_free_memory(struct sctk_alloc_mm_source * source,struct sctk_alloc_macro_bloc * bloc);
SCTK_STATIC void sctk_alloc_mm_source_light_insert_segment(struct sctk_alloc_mm_source_light* light_source, void* base, sctk_size_t size);
SCTK_STATIC struct sctk_alloc_macro_bloc * sctk_alloc_mm_source_light_mmap_new_segment(struct sctk_alloc_mm_source_light* light_source,sctk_size_t size);
SCTK_STATIC void sctk_alloc_force_segment_binding(struct sctk_alloc_mm_source_light * light_source,void* base, sctk_size_t size,int numa_node);
SCTK_STATIC  hwloc_const_nodeset_t sctk_alloc_mm_source_light_init_nodeset(int numa_node);
*/
};

/*************************** CLASS *************************/
class TestLightMMSrcUMA : public TestLightMMSrc
{
	public:
		TestLightMMSrcUMA(void) : TestLightMMSrc("TestLightMMSrcUMA",SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT) {};
};

/*************************** CLASS *************************/
class TestLightMMSrcNUMA : public TestLightMMSrc
{
	public:
		TestLightMMSrcNUMA(void) : TestLightMMSrc("TestLightMMSrcNUMA",SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT) {};
};

/*************************** CLASS *************************/
TestLightMMSrc::TestLightMMSrc(const char * name,sctk_alloc_mm_source_light_flags mode)
	:svutTestCase(name)
{
	this->mode = mode;
}

/************************* FUNCTION ************************/
void TestLightMMSrc::testMethodsRegistration (void)
{
	sctk_alloc_init_topology();
	SVUT_REG_TEST_METHOD(test_init);
	SVUT_REG_TEST_METHOD(test_setup_free_macro_bloc);
	SVUT_REG_TEST_METHOD(test_reg_in_cache_1);
	SVUT_REG_TEST_METHOD(test_reg_in_cache_2);
	SVUT_REG_TEST_METHOD(test_find_in_cache_ok);
	SVUT_REG_TEST_METHOD(test_find_in_cache_empty);
	SVUT_REG_TEST_METHOD(test_find_in_cache_to_large);
	SVUT_REG_TEST_METHOD(test_find_in_cache_on_more);
	SVUT_REG_TEST_METHOD(test_request_mem_1);
	SVUT_REG_TEST_METHOD(test_request_mem_2);
	SVUT_REG_TEST_METHOD(test_request_mem_3);
	SVUT_REG_TEST_METHOD(test_free_mem);
	SVUT_REG_TEST_METHOD(test_insert_segemnt);
	SVUT_REG_TEST_METHOD(test_new_segment);
	SVUT_REG_TEST_METHOD(test_force_binding);
	SVUT_REG_TEST_METHOD(test_size_counter_inc);
	SVUT_REG_TEST_METHOD(test_size_counter_dec);
	sctk_alloc_init_topology();
}

/************************* FUNCTION ************************/
void TestLightMMSrc::setUp (void)
{
	sctk_alloc_config_egg_init();
	sctk_alloc_mm_source_light_init(&light_source,0,mode);
	cout << "ok" << std::endl;
}

/************************* FUNCTION ************************/
void TestLightMMSrc::tearDown (void)
{
	OPA_store_int(&light_source.counter,0);
	sctk_alloc_mm_source_light_cleanup((sctk_alloc_mm_source*)&light_source);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_init(void )
{
	
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_setup_free_macro_bloc(void )
{
	char buffer[1024];
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(buffer,1024);
	SVUT_ASSERT_EQUAL(1024u,bloc->size);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_reg_in_cache_1(void )
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,bloc->size);

	//check
	SVUT_ASSERT_NULL(light_source.cache);

	//insert in source
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	//check
	SVUT_ASSERT_SAME(ptr,light_source.cache);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_reg_in_cache_2(void )
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	void * ptr2 = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc2 = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr2,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,bloc->size);
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,bloc2->size);

	//check
	SVUT_ASSERT_NULL(light_source.cache);

	//insert in source
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	//check
	SVUT_ASSERT_SAME(ptr,light_source.cache);

	//insert in source
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc2);

	//check
	SVUT_ASSERT_SAME(ptr2,light_source.cache);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_find_in_cache_ok(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	sctk_alloc_macro_bloc * selected_bloc = sctk_alloc_mm_source_light_find_in_cache(&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_SAME(selected_bloc,bloc);

	//ok didn't need to test the re-registration here.
	SVUT_ASSERT_EQUAL(1,OPA_load_int(&light_source.counter));
	OPA_store_int(&light_source.counter,0);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_find_in_cache_on_more(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	sctk_alloc_macro_bloc * selected_bloc = sctk_alloc_mm_source_light_find_in_cache(&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_SAME(selected_bloc,bloc);

	selected_bloc = sctk_alloc_mm_source_light_find_in_cache(&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_NULL(selected_bloc);

	//ok didn't need to test the re-registration here.
	SVUT_ASSERT_EQUAL(1,OPA_load_int(&light_source.counter));
	OPA_store_int(&light_source.counter,0);
}


/************************* FUNCTION ************************/
void TestLightMMSrc::test_find_in_cache_empty(void)
{
	sctk_alloc_macro_bloc * bloc = sctk_alloc_mm_source_light_find_in_cache(&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_NULL(bloc);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_find_in_cache_to_large(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	sctk_alloc_macro_bloc * selected_bloc = sctk_alloc_mm_source_light_find_in_cache(&light_source,2*SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_NULL(selected_bloc);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_request_mem_1(void)
{
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_mm_source_light_request_memory((sctk_alloc_mm_source*)&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_NOT_NULL(macro_bloc);

	//ok didn't need to test the re-registration here.
	SVUT_ASSERT_EQUAL(1,OPA_load_int(&light_source.counter));
	OPA_store_int(&light_source.counter,0);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_request_mem_2(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	//request mem
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_mm_source_light_request_memory((sctk_alloc_mm_source*)&light_source,SCTK_MACRO_BLOC_SIZE);
	if (sctk_alloc_config()->keep_max > 0)
	{
		SVUT_ASSERT_SAME(ptr,macro_bloc);
	} else {
		SVUT_ASSERT_NOT_NULL(macro_bloc);
	}

	//ok didn't need to test the re-registration here.
	SVUT_ASSERT_EQUAL(1,OPA_load_int(&light_source.counter));
	OPA_store_int(&light_source.counter,0);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_request_mem_3(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_free_macro_bloc * bloc = sctk_alloc_mm_source_light_setup_free_macro_bloc(ptr,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_mm_source_light_reg_in_cache(&light_source,bloc);

	//request mem
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_mm_source_light_request_memory((sctk_alloc_mm_source*)&light_source,2*SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_NOT_SAME(ptr,macro_bloc);

	//ok didn't need to test the re-registration here.
	SVUT_ASSERT_EQUAL(1,OPA_load_int(&light_source.counter));
	OPA_store_int(&light_source.counter,0);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_free_mem(void)
{
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_mm_source_light_request_memory((sctk_alloc_mm_source*)&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_NOT_NULL(macro_bloc);

	//check
	SVUT_ASSERT_EQUAL(1,OPA_load_int(&light_source.counter));
	SVUT_ASSERT_NULL(light_source.cache);

	//free
	sctk_alloc_mm_source_light_free_memory((sctk_alloc_mm_source*)&light_source,macro_bloc);

	//free it
	if (sctk_alloc_config()->keep_max > 0)
	{
		SVUT_ASSERT_NOT_NULL(light_source.cache);
	} else {
		SVUT_ASSERT_NULL(light_source.cache);
	}
	SVUT_ASSERT_EQUAL(0,OPA_load_int(&light_source.counter));
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_insert_segemnt(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);	

	//check
	SVUT_ASSERT_NULL(light_source.cache);

	//insert in source
	sctk_alloc_mm_source_light_insert_segment(&light_source,ptr,SCTK_MACRO_BLOC_SIZE);

	//check
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,light_source.cache->size);
	SVUT_ASSERT_SAME(ptr,light_source.cache);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_new_segment(void)
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_force_binding(void)
{
	SVUT_ASSERT_TODO("todo");
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_size_counter_inc(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);

	//check
	SVUT_ASSERT_NULL(light_source.cache);
	SVUT_ASSERT_EQUAL(0,light_source.size);

	//insert in source
	sctk_alloc_mm_source_light_insert_segment(&light_source,ptr,SCTK_MACRO_BLOC_SIZE);

	//check
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,light_source.cache->size);
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,light_source.size);
	SVUT_ASSERT_SAME(ptr,light_source.cache);
}

/************************* FUNCTION ************************/
void TestLightMMSrc::test_size_counter_dec(void)
{
	//mmap a buffer
	void * ptr = sctk_mmap(NULL,SCTK_MACRO_BLOC_SIZE);

	//check
	SVUT_ASSERT_NULL(light_source.cache);
	SVUT_ASSERT_EQUAL(0,light_source.size);

	//insert in source
	sctk_alloc_mm_source_light_insert_segment(&light_source,ptr,SCTK_MACRO_BLOC_SIZE);

	//check
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,light_source.cache->size);
	SVUT_ASSERT_EQUAL(SCTK_MACRO_BLOC_SIZE,light_source.size);
	SVUT_ASSERT_SAME(ptr,light_source.cache);

	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_mm_source_light_request_memory((sctk_alloc_mm_source*)&light_source,SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_SAME(ptr,macro_bloc);
	SVUT_ASSERT_EQUAL(0,light_source.size);
}

SVUT_REGISTER_TEST_CASE(TestLightMMSrcUMA);
SVUT_REGISTER_TEST_CASE(TestLightMMSrcNUMA);

SVUT_USE_DEFAULT_MAIN;
