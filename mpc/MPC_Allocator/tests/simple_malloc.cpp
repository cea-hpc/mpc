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
#include <stdio.h>
#include <cassert>
//#include "../src/sctk_alloc_posix.h"
//#include "../src/sctk_allocator.h"
#include <omp.h>
#include <malloc.h>
#include <Windows.h>

/************************* FUNCTION ************************/
int main(void)
{
	/*ptr = malloc(256);
	ptr = realloc(ptr,512);
	free(ptr);
	ptr = calloc(2,1024);
	free(ptr);
	free(NULL);
	ptr = realloc(NULL,512);
	realloc(ptr,0);
	ptr = malloc(0);*/
	
	//vars
	void * ptr0x7f9f01ce4010;
	void * ptr0x7f9f01ce4058;
	void * ptr0x7f9f01ce4090;
	void * ptr0x7f9f01ce40d0;
	void * ptr0x7f9f01ce4108;
	void * ptr0x7f9f01ce4128;
	void * ptr0x7f9f01ce4234;
	void * ptr0x7f9f01ce4258;
	void * ptr0x7f9f01ce443c;
	void * ptr0x7f9f01ce4644;
	void * ptr0x7f9f01ce4664;
	void * ptr0x7f9f01ce469c;
	void * ptr0x7f9f01ce46d4;
	void * ptr0x7f9f01ce470c;
	void * ptr0x7f9f01ce472c;
	void * ptr0x7f9f01ce4754;
	void * ptr0x7f9f01ce4f54;
	void * ptr0x7f9f01ce4f74;
	void * ptr0x7f9f01ce4f9c;
	void * ptr0x7f9f01ce4ff3;
	void * ptr0x7f9f01ce4fec;
	void * ptr0x7f9f01ce500c;
	void * ptr0x7f9f01ce5034;
	void * ptr0x7f9f01ce5054;
	void * ptr0x7f9f01ce5074;
	void * ptr0x7f9f01ce5094;
	void * ptr0x7f9f01ce50e4;
	void * ptr0x7f9f01ce5104;
	void * ptr0x7f9f01ce5124;
	void * ptr0x7f9f01ce51f4;
	void * ptr0x7f9f01ce543c;
	void * ptr0x7f9f01ce545d;
	void * ptr0x7f9f01ce547d;
	void * ptr0x7f9f01ce549e;
	void * ptr0x7f9f01ce54be;
	void * ptr0x7f9f01ce54df;
	void * ptr0x7f9f01ce54ff;
	void * ptr0x7f9f01ce5520;
	void * ptr0x7f9f01ce5540;
	void * ptr0x7f9f01ce5561;
	void * ptr0x7f9f01ce5581;
	void * ptr0x7f9f01ce55a2;
	void * ptr0x7f9f01ce55c2;
	void * ptr0x7f9f01ce55e3;
	void * ptr0x7f9f01ce5603;
	void * ptr0x7f9f01ce5624;
	void * ptr0x7f9f01ce5644;
	void * ptr0x7f9f01ce5145;
	void * ptr0x7f9f01ce5165;
	void * ptr0x7f9f01ce5186;
	void * ptr0x7f9f01ce51a6;
	void * ptr0x7f9f01ce51c7;
	void * ptr0x7f9f01ce5954;
	void * ptr0x7f9f01ce5975;
	void * ptr0x7f9f01ce5995;
	void * ptr0x7f9f01ce59b6;
	void * ptr0x7f9f01ce59d6;
	void * ptr0x7f9f01ce59f7;
	void * ptr0x7f9f01ce5a17;
	void * ptr0x7f9f01ce5a38;
	void * ptr0x7f9f01ce5a58;
	void * ptr0x7f9f01ce5a79;
	void * ptr0x7f9f01ce5a99;
	void * ptr0x7f9f01ce5aba;
	void * ptr0x7f9f01ce5ada;
	void * ptr0x7f9f01ce5afb;
	void * ptr0x7f9f01ce5b1b;
	void * ptr0x7f9f01ce5b3c;
	void * ptr0x7f9f01ce5b5c;
	void * ptr0x7f9f01ce5b7d;
	void * ptr0x7f9f01ce5b9d;
	void * ptr0x7f9f01ce5bbe;
	void * ptr0x7f9f01ce5bde;
	void * ptr0x7f9f01ce5bff;
	void * ptr0x7f9f01ce5c1f;
	void * ptr0x7f9f01ce5c40;
	void * ptr0x7f9f01ce5c60;
	void * ptr0x7f9f01ce5c80;
	void * ptr0x7f9f01ce5ca0;
	void * ptr0x7f9f01ce5cc0;
	void * ptr0x7f9f01ce5ce0;
	void * ptr0x7f9f01ce5d00;
	void * ptr0x7f9f01ce5d20;
	void * ptr0x7f9f01ce5d40;
	void * ptr0x7f9f01ce5d60;
	void * ptr0x7f9f01ce5d80;
	void * ptr0x7f9f01ce5da0;
	void * ptr0x7f9f01ce5dc0;
	void * ptr0x7f9f01ce5de0;
	void * ptr0x7f9f01ce5e00;
	void * ptr0x7f9f01ce5e20;
	void * ptr0x7f9f01ce5e40;
	void * ptr0x7f9f01ce5e60;
	void * ptr0x7f9f01ce5666;
	void * ptr0x7f9f01ce5686;
	void * ptr0x7f9f01ce56a7;
	void * ptr0x7f9f01ce56c7;
	void * ptr0x7f9f01ce56e8;
	void * ptr0x7f9f01ce5709;
	void * ptr0x7f9f01ce572a;
	void * ptr0x7f9f01ce574a;
	void * ptr0x7f9f01ce576b;
	void * ptr0x7f9f01ce578b;
	void * ptr0x7f9f01ce57ac;
	void * ptr0x7f9f01ce57cc;
	void * ptr0x7f9f01ce57ed;
	void * ptr0x7f9f01ce580d;
	void * ptr0x7f9f01ce582e;
	void * ptr0x7f9f01ce584e;
	void * ptr0x7f9f01ce586f;
	void * ptr0x7f9f01ce588f;
	void * ptr0x7f9f01ce58b0;
	void * ptr0x7f9f01ce58d0;
	void * ptr0x7f9f01ce58f1;
	void * ptr0x7f9f01ce5911;
	void * ptr0x7f9f01ce5932;
	void * ptr0x7f9f01ce6470;
	void * ptr0x7f9f01ce6491;
	void * ptr0x7f9f01ce64b1;
	void * ptr0x7f9f01ce64d2;
	void * ptr0x7f9f01ce64f2;
	void * ptr0x7f9f01ce6512;
	void * ptr0x7f9f01ce6532;
	void * ptr0x7f9f01ce6553;
	void * ptr0x7f9f01ce5214;
	void * ptr0x7f9f01ce5234;
	void * ptr0x7f9f01ce5254;
	void * ptr0x7f9f01ce5274;
	void * ptr0x7f9f01ce5294;
	void * ptr0x7f9f01ce52b4;
	void * ptr0x7f9f01ce6573;
	void * ptr0x7f9f01ce52d4;
	void * ptr0x7f9f01ce7583;
	void * ptr0x7f9f01ce53a4;
	void * ptr0x7f9f01ce7653;
	void * ptr0x7f9f01ce76c3;
	void * ptr0x7f9f01ce4fcc;

	ptr0x7f9f01ce4010 = malloc(568);
	ptr0x7f9f01ce4258 = malloc(120);
	
	free(ptr0x7f9f01ce4258);
	free(ptr0x7f9f01ce4010);
	
	ptr0x7f9f01ce4010 = malloc(56);
	ptr0x7f9f01ce4058 = malloc(40);
	ptr0x7f9f01ce4090 = malloc(48);
	ptr0x7f9f01ce40d0 = malloc(40);
	ptr0x7f9f01ce4108 = malloc(4);
	ptr0x7f9f01ce4128 = malloc(252);
	ptr0x7f9f01ce4234 = malloc(504);
	ptr0x7f9f01ce443c = malloc(504);
	ptr0x7f9f01ce4644 = malloc(4);
	ptr0x7f9f01ce4664 = malloc(40);
	ptr0x7f9f01ce469c = malloc(40);
	ptr0x7f9f01ce46d4 = malloc(40);
	ptr0x7f9f01ce470c = malloc(4);
	ptr0x7f9f01ce472c = malloc(24);
	ptr0x7f9f01ce4754 = malloc(2032);
	ptr0x7f9f01ce4f54 = malloc(16);
	ptr0x7f9f01ce4f74 = malloc(24);
	ptr0x7f9f01ce4f9c = malloc(32);
	
	free(ptr0x7f9f01ce4f54);
	
	ptr0x7f9f01ce4f54 = malloc(15);
	ptr0x7f9f01ce4fcc = malloc(23);
	ptr0x7f9f01ce4ff3 = malloc(568);
	
	free(ptr0x7f9f01ce4ff3);
	free(ptr0x7f9f01ce4fcc);
	
	ptr0x7f9f01ce4fcc = malloc(15);
	ptr0x7f9f01ce4fec = malloc(15);
	ptr0x7f9f01ce500c = malloc(24);
	ptr0x7f9f01ce5034 = malloc(6);
	ptr0x7f9f01ce5054 = malloc(15);
	ptr0x7f9f01ce5074 = malloc(16);
	ptr0x7f9f01ce5094 = malloc(64);
	
	free(ptr0x7f9f01ce4f9c);
	
	ptr0x7f9f01ce4f9c = malloc(11);
	ptr0x7f9f01ce50e4 = malloc(11);
	ptr0x7f9f01ce5104 = malloc(16);
	ptr0x7f9f01ce5124 = malloc(192);
	ptr0x7f9f01ce51f4 = malloc(568);
	ptr0x7f9f01ce543c = malloc(17);
	ptr0x7f9f01ce545d = malloc(7);
	ptr0x7f9f01ce547d = malloc(17);
	ptr0x7f9f01ce549e = malloc(7);
	ptr0x7f9f01ce54be = malloc(17);
	ptr0x7f9f01ce54df = malloc(8);
	ptr0x7f9f01ce54ff = malloc(17);
	ptr0x7f9f01ce5520 = malloc(9);
	ptr0x7f9f01ce5540 = malloc(17);
	ptr0x7f9f01ce5561 = malloc(6);
	ptr0x7f9f01ce5581 = malloc(17);
	ptr0x7f9f01ce55a2 = malloc(7);
	ptr0x7f9f01ce55c2 = malloc(17);
	ptr0x7f9f01ce55e3 = malloc(6);
	ptr0x7f9f01ce5603 = malloc(17);
	ptr0x7f9f01ce5624 = malloc(8);
	ptr0x7f9f01ce5644 = malloc(768);
	
	free(ptr0x7f9f01ce5124);
	
	ptr0x7f9f01ce5124 = malloc(17);
	ptr0x7f9f01ce5145 = malloc(6);
	ptr0x7f9f01ce5165 = malloc(17);
	ptr0x7f9f01ce5186 = malloc(6);
	ptr0x7f9f01ce51a6 = malloc(17);
	ptr0x7f9f01ce51c7 = malloc(9);
	ptr0x7f9f01ce5954 = malloc(17);
	ptr0x7f9f01ce5975 = malloc(8);
	ptr0x7f9f01ce5995 = malloc(17);
	ptr0x7f9f01ce59b6 = malloc(9);
	ptr0x7f9f01ce59d6 = malloc(17);
	ptr0x7f9f01ce59f7 = malloc(7);
	ptr0x7f9f01ce5a17 = malloc(17);
	ptr0x7f9f01ce5a38 = malloc(7);
	ptr0x7f9f01ce5a58 = malloc(17);
	ptr0x7f9f01ce5a79 = malloc(9);
	ptr0x7f9f01ce5a99 = malloc(17);
	ptr0x7f9f01ce5aba = malloc(7);
	ptr0x7f9f01ce5ada = malloc(17);
	ptr0x7f9f01ce5afb = malloc(6);
	ptr0x7f9f01ce5b1b = malloc(17);
	ptr0x7f9f01ce5b3c = malloc(7);
	ptr0x7f9f01ce5b5c = malloc(17);
	ptr0x7f9f01ce5b7d = malloc(9);
	ptr0x7f9f01ce5b9d = malloc(17);
	ptr0x7f9f01ce5bbe = malloc(10);
	ptr0x7f9f01ce5bde = malloc(17);
	ptr0x7f9f01ce5bff = malloc(10);
	ptr0x7f9f01ce5c1f = malloc(17);
	ptr0x7f9f01ce5c40 = malloc(8);
	ptr0x7f9f01ce5c60 = malloc(12);
	ptr0x7f9f01ce5c80 = malloc(9);
	ptr0x7f9f01ce5ca0 = malloc(12);
	ptr0x7f9f01ce5cc0 = malloc(13);
	ptr0x7f9f01ce5ce0 = malloc(12);
	ptr0x7f9f01ce5d00 = malloc(6);
	ptr0x7f9f01ce5d20 = malloc(12);
	ptr0x7f9f01ce5d40 = malloc(11);
	ptr0x7f9f01ce5d60 = malloc(11);
	ptr0x7f9f01ce5d80 = malloc(14);
	ptr0x7f9f01ce5da0 = malloc(12);
	ptr0x7f9f01ce5dc0 = malloc(7);
	ptr0x7f9f01ce5de0 = malloc(12);
	ptr0x7f9f01ce5e00 = malloc(11);
	ptr0x7f9f01ce5e20 = malloc(12);
	ptr0x7f9f01ce5e40 = malloc(6);
	ptr0x7f9f01ce5e60 = malloc(1536);
	
	free(ptr0x7f9f01ce5644);
	
	ptr0x7f9f01ce5644 = malloc(18);
	ptr0x7f9f01ce5666 = malloc(11);
	ptr0x7f9f01ce5686 = malloc(17);
	ptr0x7f9f01ce56a7 = malloc(6);
	ptr0x7f9f01ce56c7 = malloc(17);
	ptr0x7f9f01ce56e8 = malloc(17);
	ptr0x7f9f01ce5709 = malloc(17);
	ptr0x7f9f01ce572a = malloc(10);
	ptr0x7f9f01ce574a = malloc(17);
	ptr0x7f9f01ce576b = malloc(8);
	ptr0x7f9f01ce578b = malloc(17);
	ptr0x7f9f01ce57ac = malloc(7);
	ptr0x7f9f01ce57cc = malloc(17);
	ptr0x7f9f01ce57ed = malloc(11);
	ptr0x7f9f01ce580d = malloc(17);
	ptr0x7f9f01ce582e = malloc(9);
	ptr0x7f9f01ce584e = malloc(17);
	ptr0x7f9f01ce586f = malloc(8);
	ptr0x7f9f01ce588f = malloc(17);
	ptr0x7f9f01ce58b0 = malloc(7);
	ptr0x7f9f01ce58d0 = malloc(17);
	ptr0x7f9f01ce58f1 = malloc(8);
	ptr0x7f9f01ce5911 = malloc(17);
	ptr0x7f9f01ce5932 = malloc(10);
	ptr0x7f9f01ce6470 = malloc(17);
	ptr0x7f9f01ce6491 = malloc(8);
	ptr0x7f9f01ce64b1 = malloc(17);
	ptr0x7f9f01ce64d2 = malloc(8);
	ptr0x7f9f01ce64f2 = malloc(14);
	ptr0x7f9f01ce6512 = malloc(5);
	ptr0x7f9f01ce6532 = malloc(17);
	ptr0x7f9f01ce6553 = malloc(8);
	
	free(ptr0x7f9f01ce51f4);
	
	ptr0x7f9f01ce51f4 = malloc(6);
	ptr0x7f9f01ce5214 = malloc(4);
	ptr0x7f9f01ce5234 = malloc(3);
	ptr0x7f9f01ce5254 = malloc(3);
	ptr0x7f9f01ce5274 = malloc(8);
	ptr0x7f9f01ce5294 = malloc(6);
	ptr0x7f9f01ce52b4 = malloc(11);
	
	free(ptr0x7f9f01ce5234);
	free(ptr0x7f9f01ce51f4);
	free(ptr0x7f9f01ce5214);
	free(ptr0x7f9f01ce50e4);
	free(ptr0x7f9f01ce5104);
	
	ptr0x7f9f01ce50e4 = malloc(2);
	ptr0x7f9f01ce51f4 = malloc(48);
	ptr0x7f9f01ce6573 = malloc(4096);
	ptr0x7f9f01ce52d4 = malloc(192);
	ptr0x7f9f01ce7583 = malloc(192);
	ptr0x7f9f01ce53a4 = malloc(96);
	ptr0x7f9f01ce7653 = malloc(96);
	ptr0x7f9f01ce76c3 = malloc(130);
	
	malloc(32);
	
	malloc(256*1024*1024);
	//system("pause");
	
	//test_massiv_thread_malloc
	#define TEST_REAL_HEAP_BASE (SCTK_ALLOC_HEAP_BASE)
	#define TEST_REAL_HEAP_SIZE (SCTK_ALLOC_HEAP_SIZE - SCTK_PAGE_SIZE)
	#define TEST_MAX_THREAD 1024
	#define TEST_NB_ALLOC 32*4096
	puts("test new\n");
	void ** ptr = new void * [TEST_MAX_THREAD * TEST_NB_ALLOC];
	int nb;

	DWORD t0 = GetTickCount();
	for (int loop = 0 ; loop < 50 ; loop++)
	{

		#pragma omp parallel
		#pragma omp single
		nb = omp_get_num_threads();
		//nb=1;
		assert(TEST_MAX_THREAD > nb);
		#pragma omp parallel
		{
			int id = omp_get_thread_num();
			for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
			{
				ptr[id*TEST_NB_ALLOC+j] = malloc(256);
				*(char*)ptr[id*TEST_NB_ALLOC+j] = 'c';
			}
		}
		puts("line 386\n");
		#pragma omp parallel
		{
			int id = (omp_get_thread_num()) % nb;
			for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
			{
				assert(ptr[id*TEST_NB_ALLOC+j] != NULL);
				free(ptr[id*TEST_NB_ALLOC+j]);
			}
		}
		//puts("line 393\n");
		#pragma omp parallel
		{
			free(NULL);
		}
		//puts("line 398\n");
		for (int i = 0 ; i < nb ; ++i)
			for (int j = 0 ; j < TEST_NB_ALLOC ; ++j)
			{
				//SVUT_SET_CONTEXT("Thread",i);
				//SVUT_SET_CONTEXT("Bloc",j);
				//SVUT_SET_CONTEXT("BlocAddr",ptr[i*TEST_NB_ALLOC+j]);
				//assert((sctk_alloc_region_get_entry(ptr[i*TEST_NB_ALLOC+j])->macro_bloc) == NULL);
			}
		int * seb = new int;
		delete seb;

	}
	DWORD t1 = GetTickCount();
	printf("time = %llu\n",t1-t0);
	delete[] ptr;

	//system("pause");

	_aligned_malloc(100,1024);

	system("pause");
	return EXIT_SUCCESS;
}
