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
#include <sctk_alloc_internal_spinlock.h>

/************************** USING **************************/
using namespace svUnitTest;

/*************************** CLASS *************************/
class TestInternalSpinlock : public svutTestCase
{
	public:
		void testMethodsRegistration(void);
		virtual void setUp(void) {};
		virtual void tearDown(void) {};
	protected:
		void test_init(void);
		void test_init_static(void);
		void test_lock(void);
		void test_unlock(void);
		void test_trylock(void);
		void test_safe(void);
};

/************************* FUNCTION ************************/
void TestInternalSpinlock::testMethodsRegistration (void)
{
	setTestCaseName("TestAllocRFQ");
	SVUT_REG_TEST_METHOD(test_init);
	SVUT_REG_TEST_METHOD(test_init_static);
	SVUT_REG_TEST_METHOD(test_lock);
	SVUT_REG_TEST_METHOD(test_unlock);
	SVUT_REG_TEST_METHOD(test_trylock);
	SVUT_REG_TEST_METHOD(test_safe);
}

/************************* FUNCTION ************************/
void TestInternalSpinlock::test_init(void )
{
	sctk_alloc_internal_spinlock_t lock;
	sctk_alloc_internal_spinlock_init(&lock);
	SVUT_ASSERT_FALSE(sctk_alloc_internal_spinlock_is_lock(&lock));
}

/************************* FUNCTION ************************/
void TestInternalSpinlock::test_init_static(void )
{
	sctk_alloc_internal_spinlock_t lock = SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER;
	SVUT_ASSERT_FALSE(sctk_alloc_internal_spinlock_is_lock(&lock));
}

/************************* FUNCTION ************************/
void TestInternalSpinlock::test_lock(void )
{
	sctk_alloc_internal_spinlock_t lock = SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER;
	SVUT_ASSERT_FALSE(sctk_alloc_internal_spinlock_is_lock(&lock));
	sctk_alloc_internal_spinlock_lock(&lock);
	SVUT_ASSERT_TRUE(sctk_alloc_internal_spinlock_is_lock(&lock));
}

/************************* FUNCTION ************************/
void TestInternalSpinlock::test_unlock(void )
{
	sctk_alloc_internal_spinlock_t lock = SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER;
	SVUT_ASSERT_FALSE(sctk_alloc_internal_spinlock_is_lock(&lock));
	sctk_alloc_internal_spinlock_lock(&lock);
	SVUT_ASSERT_TRUE(sctk_alloc_internal_spinlock_is_lock(&lock));
	sctk_alloc_internal_spinlock_unlock(&lock);
	SVUT_ASSERT_FALSE(sctk_alloc_internal_spinlock_is_lock(&lock));
}

/************************* FUNCTION ************************/
void TestInternalSpinlock::test_trylock(void )
{
	sctk_alloc_internal_spinlock_t lock = SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER;
	int res = sctk_alloc_internal_spinlock_trylock(&lock);
	SVUT_ASSERT_EQUAL(0,res);
	SVUT_ASSERT_TRUE(sctk_alloc_internal_spinlock_is_lock(&lock));
	res = sctk_alloc_internal_spinlock_trylock(&lock);
	SVUT_ASSERT_NOT_EQUAL(0,res);
	SVUT_ASSERT_TRUE(sctk_alloc_internal_spinlock_is_lock(&lock));
}

/************************* FUNCTION ************************/
void TestInternalSpinlock::test_safe(void )
{
	sctk_alloc_internal_spinlock_t lock = SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER;
	unsigned long cnt = 0;
	#pragma omp parallel for shared(cnt,lock)
	for (long i = 0 ; i < 10000000 ; i++)
	{
		sctk_alloc_internal_spinlock_lock(&lock);
		cnt++;
		sctk_alloc_internal_spinlock_unlock(&lock);
	}

	SVUT_ASSERT_EQUAL(10000000,cnt);
}

/************************* FUNCTION ************************/
SVUT_REGISTER_STANDELONE(TestInternalSpinlock);
