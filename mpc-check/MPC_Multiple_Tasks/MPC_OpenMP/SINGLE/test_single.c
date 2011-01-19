/* ############################# MPC License ############################## */
/* # Thu Apr 24 18:48:27 CEST 2008                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* #                                                                      # */
/* # This file is part of the MPC Library.                                # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <assert.h>
#include <omp.h>
#include <omp_abi.h>
#include <stdio.h>

/*
   Test of the non-nested single 'single' construct
   (i.e., there is no nesting and there is an explicit barrier after each
   single construct)
 */

void *
run (void *arg)
{
  if (__mpcomp_do_single ())
    {
      fprintf (stdout, "Single hello 1 from thread %d\n",
	       mpcomp_get_thread_num ());
    }

  __mpcomp_barrier ();

  if (__mpcomp_do_single ())
    {
      fprintf (stdout, "Single hello 2 from thread %d\n",
	       mpcomp_get_thread_num ());
    }

  __mpcomp_barrier ();
  __mpcomp_barrier ();

  if (__mpcomp_do_single ())
    {
      fprintf (stdout, "Single hello 3 from thread %d\n",
	       mpcomp_get_thread_num ());
    }

  __mpcomp_barrier ();

  if (__mpcomp_do_single ())
    {
      fprintf (stdout, "Single hello 4 from thread %d\n",
	       mpcomp_get_thread_num ());
    }

  return NULL;
}

int
main (int argc, char **argv)
{
  assert (mpcomp_get_thread_num () == 0);
  assert (mpcomp_get_num_threads () == 1);
  assert (mpcomp_in_parallel () == 0);

  fprintf (stdout, "I am %d on %d max %d cpus %d\n",
	   mpcomp_get_thread_num (),
	   mpcomp_get_num_threads (),
	   mpcomp_get_max_threads (), mpcomp_get_num_procs ());


  {
    __mpcomp_start_parallel_region (10, run, NULL);
  }

  fprintf (stdout, "ALL DONE\n");
  return 0;
}

