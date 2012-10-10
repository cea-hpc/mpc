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

/************************** USING **************************/
using namespace svUnitTest;

/************************** CONSTS *************************/
#define TEST_ADDR_REGION_0 NULL
#define TEST_ADDR_REGION_1 ((char*)SCTK_REGION_SIZE)
#define TEST_ADDR_REGION_2 (TEST_ADDR_REGION_1 + SCTK_REGION_SIZE)
#define TEST_ADDR_REGION_3 (TEST_ADDR_REGION_2 + SCTK_REGION_SIZE)
#define TEST_ADDR_REGION_4 (TEST_ADDR_REGION_3 + SCTK_REGION_SIZE)

#define TEST_ADDR_REGION_0_START (TEST_ADDR_REGION_0)
#define TEST_ADDR_REGION_1_START (TEST_ADDR_REGION_1)
#define TEST_ADDR_REGION_2_START (TEST_ADDR_REGION_2)
#define TEST_ADDR_REGION_3_START (TEST_ADDR_REGION_3)
#define TEST_ADDR_REGION_4_START (TEST_ADDR_REGION_4)

#define TEST_REGION_AVAIL_SIZE (SCTK_REGION_SIZE)

/*************************** CLASS *************************/
class TestRegions : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void);
		virtual void tearDown(void);
	private:
		void test_struct_size(void);
		void test_setup_region(void);
		void test_setup_region_repeat(void);
		void test_get_region(void);
		void test_del_all_regions(void);
		void test_get_region_entry(void);
		void test_exist_NULL(void);
		void test_exist_none(void);
		void test_exist_avail(void);
		void test_setup_entry_1(void);
		void test_setup_entry_2(void);
		void test_setup_entry_3(void);
		void test_setup_entry_4(void);
		void test_get_id(void);
		void test_region_has_ref(void);
		void test_region_del_chain(void);
		void test_region_del_chain_NULL(void);
		void test_auto_setup_region(void);
};

/************************* FUNCTION ************************/
void TestRegions::testMethodsRegistration (void)
{
	setTestCaseName("TestRegions");
	SVUT_REG_TEST_METHOD(test_struct_size);
	SVUT_REG_TEST_METHOD(test_setup_region);
	SVUT_REG_TEST_METHOD(test_setup_region_repeat);
	SVUT_REG_TEST_METHOD(test_get_region);
	SVUT_REG_TEST_METHOD(test_get_region);
	SVUT_REG_TEST_METHOD(test_del_all_regions);
	SVUT_REG_TEST_METHOD(test_get_region_entry);
	SVUT_REG_TEST_METHOD(test_exist_NULL);
	SVUT_REG_TEST_METHOD(test_exist_none);
	SVUT_REG_TEST_METHOD(test_exist_avail);
	SVUT_REG_TEST_METHOD(test_setup_entry_1);
	SVUT_REG_TEST_METHOD(test_setup_entry_2);
	SVUT_REG_TEST_METHOD(test_setup_entry_3);
	SVUT_REG_TEST_METHOD(test_setup_entry_4);
	SVUT_REG_TEST_METHOD(test_get_id);
	SVUT_REG_TEST_METHOD(test_region_has_ref);
	SVUT_REG_TEST_METHOD(test_region_del_chain);
	SVUT_REG_TEST_METHOD(test_region_del_chain_NULL);
	SVUT_REG_TEST_METHOD(test_auto_setup_region);
}

/************************* FUNCTION ************************/
void TestRegions::setUp (void)
{
	/*
	#ifdef _WIN32
		void *res = VirtualAlloc((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		assert(res == (void*)SCTK_ALLOC_HEAP_BASE);
	#endif
	*/
}

/************************* FUNCTION ************************/
void TestRegions::tearDown (void)
{
	/*
	#ifdef _WIN32
		VirtualFree((void*)SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE,MEM_RELEASE);
	#endif
	*/
}

/************************* FUNCTION ************************/
void TestRegions::test_struct_size(void )
{
 	SVUT_ASSERT_EQUAL(8ul * SCTK_REGION_HEADER_ENTRIES,sizeof(sctk_alloc_region));
	SVUT_ASSERT_EQUAL(0ul,SCTK_REGION_SIZE % SCTK_MACRO_BLOC_SIZE);
	SVUT_ASSERT_GE(SCTK_REGION_HEADER_SIZE,sizeof(sctk_alloc_region));
}

/************************* FUNCTION ************************/
void TestRegions::test_setup_region(void )
{
	//setup the region
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	sctk_alloc_region * confirm = sctk_alloc_region_get(TEST_ADDR_REGION_1);
	SVUT_ASSERT_NOT_NULL(region);
	SVUT_ASSERT_SAME(confirm,region);

	//delete it
	sctk_alloc_region_del(region);
}

/************************* FUNCTION ************************/
void TestRegions::test_setup_region_repeat(void )
{
	//try to setup a region multiple times
	sctk_alloc_region * region1 = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	SVUT_ASSERT_NOT_NULL(region1);

	//resetup
	sctk_alloc_region * region2  = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	SVUT_ASSERT_NOT_NULL(region2);
	SVUT_ASSERT_SAME(region1,region2);

	//delete it
	sctk_alloc_region_del(region1);

	//check not setup (to ensute not include 2 times in list)
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(region1));
}

/************************* FUNCTION ************************/
void TestRegions::test_get_region(void )
{
	SVUT_ASSERT_NULL(sctk_alloc_region_get(TEST_ADDR_REGION_1));
	SVUT_ASSERT_NULL(sctk_alloc_region_get(TEST_ADDR_REGION_2));
	SVUT_ASSERT_NULL(sctk_alloc_region_get(TEST_ADDR_REGION_3));
	
	sctk_alloc_region * region1 = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	sctk_alloc_region * region2 = sctk_alloc_region_setup(TEST_ADDR_REGION_2);
	sctk_alloc_region * region3 = sctk_alloc_region_setup(TEST_ADDR_REGION_3);

	//check diff
	SVUT_ASSERT_NOT_NULL(region1);
	SVUT_ASSERT_NOT_NULL(region2);
	SVUT_ASSERT_NOT_NULL(region3);
	SVUT_ASSERT_NOT_SAME(region1,region2);
	SVUT_ASSERT_NOT_SAME(region2,region3);
	SVUT_ASSERT_NOT_SAME(region1,region3);
	SVUT_ASSERT_NOT_NULL(region1->entries);
	SVUT_ASSERT_NOT_NULL(region2->entries);
	SVUT_ASSERT_NOT_NULL(region3->entries);
	SVUT_ASSERT_NULL(region1->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region2->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region3->entries[0].macro_bloc);
	
	//test the good application of bit masq to get the region base addr on first region
	SVUT_ASSERT_SAME(region1, sctk_alloc_region_get(TEST_ADDR_REGION_1));
	SVUT_ASSERT_SAME(region1, sctk_alloc_region_get(TEST_ADDR_REGION_1+1024));
	SVUT_ASSERT_SAME(region1, sctk_alloc_region_get(TEST_ADDR_REGION_2-1));

	//test on another one
	SVUT_ASSERT_SAME(region2, sctk_alloc_region_get(TEST_ADDR_REGION_2));
	SVUT_ASSERT_SAME(region2, sctk_alloc_region_get(TEST_ADDR_REGION_2+1024));
	SVUT_ASSERT_SAME(region2, sctk_alloc_region_get(TEST_ADDR_REGION_3-1));

	//test on another one
	SVUT_ASSERT_SAME(region3, sctk_alloc_region_get(TEST_ADDR_REGION_3));
	SVUT_ASSERT_SAME(region3, sctk_alloc_region_get(TEST_ADDR_REGION_3+1024));
	SVUT_ASSERT_SAME(region3, sctk_alloc_region_get(TEST_ADDR_REGION_4-1));

	sctk_alloc_region_del(region1);
	sctk_alloc_region_del(region2);
	sctk_alloc_region_del(region3);
}

/************************* FUNCTION ************************/
void TestRegions::test_del_all_regions(void )
{
	//check no region at start
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_2));
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_3));
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_4));

	//create regions
	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_setup(TEST_ADDR_REGION_1));
	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_setup(TEST_ADDR_REGION_2));
	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_setup(TEST_ADDR_REGION_3));
	SVUT_ASSERT_NOT_NULL(sctk_alloc_region_setup(TEST_ADDR_REGION_4));

	//check new regions
	SVUT_ASSERT_TRUE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));
	SVUT_ASSERT_TRUE(sctk_alloc_region_exist(TEST_ADDR_REGION_2));
	SVUT_ASSERT_TRUE(sctk_alloc_region_exist(TEST_ADDR_REGION_3));
	SVUT_ASSERT_TRUE(sctk_alloc_region_exist(TEST_ADDR_REGION_4));

	//delete region
	sctk_alloc_region_del_all();

	//check no region at end
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_2));
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_3));
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_4));
}

/************************* FUNCTION ************************/
void TestRegions::test_get_region_entry(void )
{
	sctk_alloc_region * region1 = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	sctk_alloc_region * region2 = sctk_alloc_region_setup(TEST_ADDR_REGION_2);
	sctk_alloc_region * region3 = sctk_alloc_region_setup(TEST_ADDR_REGION_3);

	//test the good application of bit masq to get the region base addr on first region
	SVUT_ASSERT_SAME(&region1->entries[0]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_1_START));
	SVUT_ASSERT_SAME(&region1->entries[0]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_1_START + 5));
	SVUT_ASSERT_SAME(&region1->entries[1]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_1_START + 1*SCTK_MACRO_BLOC_SIZE));
	SVUT_ASSERT_SAME(&region1->entries[2]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_1_START + 2*SCTK_MACRO_BLOC_SIZE));
	SVUT_ASSERT_SAME(&region1->entries[2]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_1_START + 2*SCTK_MACRO_BLOC_SIZE+1024));

	//test on another one
	SVUT_ASSERT_SAME(&region2->entries[2]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_2_START + 2*SCTK_MACRO_BLOC_SIZE));
	SVUT_ASSERT_SAME(&region2->entries[2]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_2_START + 2*SCTK_MACRO_BLOC_SIZE+1024));

	//test on another one
	SVUT_ASSERT_SAME(&region3->entries[2]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_3_START + 2*SCTK_MACRO_BLOC_SIZE));
	SVUT_ASSERT_SAME(&region3->entries[2]  ,sctk_alloc_region_get_entry(TEST_ADDR_REGION_3_START + 2*SCTK_MACRO_BLOC_SIZE+1024));

	sctk_alloc_region_del(region1);
	sctk_alloc_region_del(region2);
	sctk_alloc_region_del(region3);
}

/************************* FUNCTION ************************/
void TestRegions::test_exist_avail(void )
{
	SVUT_ASSERT_NULL( sctk_alloc_region_get(TEST_ADDR_REGION_1) );

	//region isn't setup so need to get false
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));

	//setup region and test params
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	SVUT_ASSERT_NOT_NULL(region);

	//test availability
	SVUT_ASSERT_TRUE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));

	//remove the region
	sctk_alloc_region_del(region);

	//check if not available
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));
}

/************************* FUNCTION ************************/
void TestRegions::test_exist_NULL(void )
{
	//Bad region given so need to get False
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(NULL));
}

/************************* FUNCTION ************************/
void TestRegions::test_exist_none(void )
{
	//No available regions so need to get FALSE
	SVUT_ASSERT_FALSE(sctk_alloc_region_exist(TEST_ADDR_REGION_1));
}

/************************* FUNCTION ************************/
void TestRegions::test_setup_entry_1(void )
{
	sctk_alloc_chain chain;
	chain.flags = SCTK_ALLOC_CHAIN_FLAGS_DEFAULT;
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	SVUT_ASSERT_NOT_NULL(sctk_mmap(TEST_ADDR_REGION_1_START,SCTK_MACRO_BLOC_SIZE));
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_setup_macro_bloc(TEST_ADDR_REGION_1_START,SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_region_set_entry(&chain,macro_bloc);

	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del_chain(region,&chain);
	sctk_alloc_region_del(region);
	sctk_munmap(TEST_ADDR_REGION_1_START, SCTK_MACRO_BLOC_SIZE);
}

/************************* FUNCTION ************************/
void TestRegions::test_setup_entry_2(void )
{
	sctk_alloc_chain chain;
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	#ifdef _WIN32
		MARK_AS_KNOWN_ERROR("Unknown Error...");
	#endif
	SVUT_ASSERT_NOT_NULL(sctk_mmap(TEST_ADDR_REGION_1_START,3*SCTK_MACRO_BLOC_SIZE));
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_setup_macro_bloc(TEST_ADDR_REGION_1_START,3*SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_region_set_entry(&chain,macro_bloc);

	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[0].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[1].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del_chain(region,&chain);
	sctk_alloc_region_del(region);
	sctk_munmap(TEST_ADDR_REGION_1_START, SCTK_MACRO_BLOC_SIZE);
}

/************************* FUNCTION ************************/
void TestRegions::test_setup_entry_3(void )
{
	sctk_alloc_chain chain;
	chain.flags = SCTK_ALLOC_CHAIN_FLAGS_DEFAULT;
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	SVUT_ASSERT_NOT_NULL(sctk_mmap(TEST_ADDR_REGION_1_START+SCTK_MACRO_BLOC_SIZE,3*SCTK_MACRO_BLOC_SIZE));
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_setup_macro_bloc(TEST_ADDR_REGION_1_START+SCTK_MACRO_BLOC_SIZE,3*SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_region_set_entry(&chain,macro_bloc);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[1].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[2].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del_chain(region,&chain);
	sctk_alloc_region_del(region);
	sctk_munmap(TEST_ADDR_REGION_1_START+SCTK_MACRO_BLOC_SIZE,3*SCTK_MACRO_BLOC_SIZE);
}

/************************* FUNCTION ************************/
void TestRegions::test_setup_entry_4(void )
{;
	sctk_alloc_chain chain;
	chain.flags = SCTK_ALLOC_CHAIN_FLAGS_DEFAULT;
	sctk_alloc_region * region1 = sctk_alloc_region_setup(TEST_ADDR_REGION_1);
	sctk_alloc_region * region2 = sctk_alloc_region_setup(TEST_ADDR_REGION_2);

	SVUT_ASSERT_NULL(region1->entries[SCTK_REGION_HEADER_ENTRIES-3].macro_bloc);
	SVUT_ASSERT_NULL(region1->entries[SCTK_REGION_HEADER_ENTRIES-2].macro_bloc);
	SVUT_ASSERT_NULL(region1->entries[SCTK_REGION_HEADER_ENTRIES-1].macro_bloc);
	SVUT_ASSERT_NULL(region2->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region2->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region2->entries[2].macro_bloc);

	SVUT_ASSERT_NOT_NULL(sctk_mmap(TEST_ADDR_REGION_2_START - 2*SCTK_MACRO_BLOC_SIZE,4*SCTK_MACRO_BLOC_SIZE));
	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_setup_macro_bloc(TEST_ADDR_REGION_2_START - 2*SCTK_MACRO_BLOC_SIZE,4*SCTK_MACRO_BLOC_SIZE);
	sctk_alloc_region_set_entry(&chain,macro_bloc);

	SVUT_ASSERT_NULL(region1->entries[SCTK_ALLOC_MAX_REGIONS-3].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region1->entries[SCTK_REGION_HEADER_ENTRIES-2].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region1->entries[SCTK_REGION_HEADER_ENTRIES-1].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region2->entries[0].macro_bloc);
	SVUT_ASSERT_EQUAL(macro_bloc,region2->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region2->entries[2].macro_bloc);

	sctk_alloc_region_del_chain(region1,&chain);
	sctk_alloc_region_del_chain(region2,&chain);
	sctk_alloc_region_del(region1);
	sctk_alloc_region_del(region2);
}

/************************* FUNCTION ************************/
void TestRegions::test_get_id(void )
{
	SVUT_ASSERT_EQUAL(1,sctk_alloc_region_get_id(TEST_ADDR_REGION_1));
	SVUT_ASSERT_EQUAL(1,sctk_alloc_region_get_id(TEST_ADDR_REGION_1+4096));
	SVUT_ASSERT_EQUAL(1,sctk_alloc_region_get_id(TEST_ADDR_REGION_2-1));

	SVUT_ASSERT_EQUAL(2,sctk_alloc_region_get_id(TEST_ADDR_REGION_2));
	SVUT_ASSERT_EQUAL(2,sctk_alloc_region_get_id(TEST_ADDR_REGION_2+4096));
	SVUT_ASSERT_EQUAL(2,sctk_alloc_region_get_id(TEST_ADDR_REGION_3-1));

	SVUT_ASSERT_EQUAL(3,sctk_alloc_region_get_id(TEST_ADDR_REGION_3));
	SVUT_ASSERT_EQUAL(3,sctk_alloc_region_get_id(TEST_ADDR_REGION_3+4096));
	SVUT_ASSERT_EQUAL(3,sctk_alloc_region_get_id(TEST_ADDR_REGION_4-1));
}

/************************* FUNCTION ************************/
void TestRegions::test_region_del_chain(void )
{
	sctk_alloc_chain chain1;
	sctk_alloc_chain chain2;
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);

	sctk_alloc_macro_bloc macro_bloc_1;
	sctk_alloc_macro_bloc macro_bloc_2;
	macro_bloc_1.chain = &chain1;
	macro_bloc_2.chain = &chain2;

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	region->entries[1].macro_bloc = &macro_bloc_1;
	region->entries[2].macro_bloc = &macro_bloc_2;
	region->entries[3].macro_bloc = &macro_bloc_1;

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_1,region->entries[1].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_2,region->entries[2].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_1,region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del_chain(region,&chain1);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_2,region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del_chain(region,&chain2);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);

	sctk_alloc_region_del(region);
}

/************************* FUNCTION ************************/
void TestRegions::test_region_del_chain_NULL(void )
{
	sctk_alloc_chain chain1;
	sctk_alloc_chain chain2;
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_macro_bloc macro_bloc_1;
	sctk_alloc_macro_bloc macro_bloc_2;
	macro_bloc_1.chain = &chain1;
	macro_bloc_2.chain = &chain2;

	region->entries[1].macro_bloc = &macro_bloc_1;
	region->entries[2].macro_bloc = &macro_bloc_2;
	region->entries[3].macro_bloc = &macro_bloc_1;

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_1,region->entries[1].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_2,region->entries[2].macro_bloc);
	SVUT_ASSERT_SAME(&macro_bloc_1,region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del_chain(region,NULL);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	sctk_alloc_region_del(region);
}

/************************* FUNCTION ************************/
void TestRegions::test_region_has_ref(void )
{
	sctk_alloc_chain chain1;
	sctk_alloc_region * region = sctk_alloc_region_setup(TEST_ADDR_REGION_1);

	SVUT_ASSERT_NULL(region->entries[0].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[1].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[2].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[3].macro_bloc);
	SVUT_ASSERT_NULL(region->entries[4].macro_bloc);

	SVUT_ASSERT_FALSE(sctk_alloc_region_has_ref(region));

	sctk_alloc_macro_bloc macro_bloc_1;
	macro_bloc_1.chain = &chain1;
	region->entries[1].macro_bloc = &macro_bloc_1;

	SVUT_ASSERT_TRUE(sctk_alloc_region_has_ref(region));

	sctk_alloc_region_del_chain(region,NULL);
	sctk_alloc_region_del(region);
}

/************************* FUNCTION ************************/
void TestRegions::test_auto_setup_region(void )
{
	sctk_alloc_chain chain;
	sctk_alloc_region * region = sctk_alloc_region_get(TEST_ADDR_REGION_1);
	SVUT_ASSERT_NOT_NULL(sctk_mmap(TEST_ADDR_REGION_1_START, SCTK_ALLOC_PAGE_SIZE));

	SVUT_ASSERT_NULL(region);

	sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_setup_macro_bloc(TEST_ADDR_REGION_1,1024);
	sctk_alloc_region_set_entry(&chain,macro_bloc);
	region = sctk_alloc_region_get(TEST_ADDR_REGION_1);

	SVUT_ASSERT_NOT_NULL(region);

	sctk_alloc_region_del_chain(region,NULL);
	sctk_alloc_region_del(region);
}

SVUT_REGISTER_STANDELONE(TestRegions);
