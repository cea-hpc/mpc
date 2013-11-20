/********************************* INCLUDES *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include <string.h>
#include <sctk_alloc_rdtsc.h>
#include <sctk_alloc_mpscf_queue.h>
#define TEST_SIZE 20000
#define REPEAT 100

typedef unsigned long long ticks;

/********************************* FUNCTION *********************************/
void test_with_n_threads(int threads)
{
	//vars
	int i;
	size_t cnt = 0;
	int j;
	size_t expected;
	size_t test_size;
	struct sctk_mpscf_queue queue;
	ticks t0;
	ticks tflush = 0;
	ticks treg = 0;

	//errors
	assert(threads > 0);

	//count expected
	if (threads == 1)
	{
		test_size = TEST_SIZE;
		expected = TEST_SIZE;
	} else {
		test_size = TEST_SIZE / (threads-1);
		expected = test_size * (threads-1);
	}

	//info
	printf("    +-> Start test with %d thread(s)\n",threads);
	printf("        +-> Insert per thread = %lu\n",test_size);
	printf("        +-> Expected = %lu\n",expected);

	//setup queue
	sctk_mpscf_queue_init(&queue);

	//setup number of threads
	omp_set_num_threads(threads);

	//test in parallel
	#pragma omp parallel private(i,j,t0)
	{
		//allocate elements to insert
		struct sctk_mpscf_queue_entry  * entries = malloc(sizeof(struct sctk_mpscft_queue_entry *)*TEST_SIZE);
		struct sctk_mpscf_queue_entry * res;

		//get current ID
		int id = omp_get_thread_num();

		//repeat the test
		for ( j = 0 ; j < REPEAT ; j++)
		{
			//all thread insert except 0, or for sequential test, 0 does the two actions
			if (id != 0 || threads == 1)
			{

				for ( i = 0 ; i < test_size ; i++)
				{
					t0 = sctk_alloc_rdtsc();
					sctk_mpscf_queue_insert(&queue,&entries[i]);
					treg += (sctk_alloc_rdtsc() - t0);
				}
			}

			//0 flush and count
			if (id == 0)
			{
				cnt = 0;
				while (cnt < expected)
				{
					t0 = sctk_alloc_rdtsc();
					res = sctk_mpscft_queue_dequeue_all(&queue);
					if (res != NULL)
						tflush += (sctk_alloc_rdtsc() - t0);
					while (res != NULL)
					{
						cnt++;
						res = res->next;
					}
				}
			}
			#pragma omp barrier
			#pragma omp single
			{
				if (j < 5)
					printf("        +-> Count = %lu == %lu\n",cnt,expected);
				else if (j == 5)
					printf("        +-> .");
				else if (j % 100 == 0)
					printf("\n        +-> .");
				else
					printf(".");
				assert(cnt == expected);
			}
		}
		free(entries);
	}

	//print some time info
	if (REPEAT >= 5)printf("\n");
	printf("        +-> Mean cycles per register = %llu\n",treg / REPEAT / expected);
	printf("        +-> Mean cycles per flush    = %llu\n",tflush / REPEAT / expected);
	printf("        +-> OK valid for %d threads\n",threads);
	fflush(stdout);
}

/********************************* FUNCTION *********************************/
int main(int argc, char ** argv)
{
	int i;
	int threads;

	//check args
	if (argc != 2)
	{
		fprintf(stderr,"Usage : %s {bench|valid}\n",argv[0]);
		return EXIT_FAILURE;
	}

	puts("========== START TEST OF ATOMIC RFQ ===========");

	#pragma omp parallel
		#pragma omp single
			threads = omp_get_num_threads();

	//some ressources info
	printf("Use %d as max threads\n",threads);
	printf("Use less than %lu Mo of memory\n",threads * TEST_SIZE * sizeof(struct sctk_mpscf_queue) / 1024ul / 1024ul);

	//test with all number of threads
	if (strcmp(argv[1],"bench") == 0)
	{
		for ( i = 1 ; i <= threads ; i*=2)
			test_with_n_threads(i);
	} else if (strcmp(argv[1],"valid") == 0) {
		test_with_n_threads(threads);
	} else {
		fprintf(stderr,"Usage : %s {bench|valid}\n",argv[0]);
		return EXIT_FAILURE;
	}

	puts("===============================================");
	return EXIT_SUCCESS;
}
