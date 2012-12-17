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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpcomp.h>
#include "sctk.h"
#include "sctk_alloc.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "mpcmicrothread_internal.h"
#include "mpcomp_internal.h"

/* TODO clean the following includes */
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static volatile unsigned long perform_check = 1;

/*
   TODO this checkpointing might not work in a nested-openmp environment! 
 */

static void *
mpcomp_master_checkpointing (void *s)
{
  int restarted;
  unsigned long perform;

  perform = perform_check;

  sctk_nodebug
    ("mpcomp_master_checkpointing: Entering for micro-VP 0 (saveid=%d)",
     perform);

  fprintf (stderr, "OpenMP Checkpoint %lu\n", perform);

  /* Call the message-passing checkpointing */
  MPC_Checkpoint ();

  sctk_nodebug ("mpcomp_master_checkpointing: MPC_Checkpoint done");

  /* Check if this is a restart-mode */
  MPC_Restarted (&restarted);

  sctk_nodebug ("mpcomp_master_checkpointing: Restarting? %d", restarted);

  if (restarted == 1)
    {
      DIR *d;
      struct dirent *direntry;
      char *dirname;
      int i, j;

      sctk_microthread_t *current_task;

      current_task = sctk_thread_getspecific (sctk_microthread_key);
      sctk_assert (current_task != NULL);

      dirname = sctk_store_dir;

      sctk_nodebug
	("mpcomp_master_checkpointing: Starting restoring saveid=%d in directory %s",
	 perform, dirname);

      /* For each micro-VP */
      for (i = 1; i < current_task->__nb_vp; i++)
	{
	  char name[1024];
	  sctk_thread_t pid = current_task->__list[i].pid;
	  int found = 0;
	  int found_vp = -1;

	  d = opendir (dirname);

	  do
	    {
	      char *current_filename;

	      direntry = readdir (d);

	      current_filename = direntry->d_name;
	      /* Try to find a match for every VP */
	      for (j = 0; j < sctk_get_processor_number (); j++)
		{
		  sprintf (name, "%d_%d_%p_%lu", sctk_get_task_rank (),
			   j, pid, perform);
		  if (strncmp (current_filename, name, strlen (name)) == 0)
		    {
		      found = 1;
		      found_vp = j;
		      sctk_nodebug
			("mpcomp_master_checkpointing: Found match for VP %d",
			 j);
		      break;
		    }
		}
	    }
	  while (found == 0 && direntry != NULL);

	  closedir (d);

	  if (found)
	    {
	      char fullname[1024];
	      sprintf (fullname, "%s/%s", sctk_store_dir, name);
	      sctk_nodebug
		("mpcomp_master_checkpointing: Restoring <%s> on vp=%d",
		 fullname, j);
	      sctk_thread_restore (pid, fullname, j);
	      sctk_nodebug ("mpcomp_master_checkpointing: ... done!");
	    }
	}

      sctk_nodebug ("mpcomp_master_checkpointing: Restore done!");


    }

  return NULL;
}

static void *
mpcomp_slave_checkpointing (void *s)
{
  char name[1024];
  sctk_thread_t pid;
  unsigned long perform;

  pid = ((sctk_microthread_vp_t *) s)->pid;

  perform = perform_check;

  sctk_nodebug ("mpcomp_slave_checkpointing[%d]: Entering for micro-VP %p",
		perform, pid);

  /* Remove the old files -> step - 2 */
  if (perform > 3)
    {
      sprintf (name, "%s/%d_%d_%p_%lu", sctk_store_dir, sctk_get_task_rank (),
	       sctk_get_processor_rank (), pid, perform - 3);
      remove (name);
    }

  sprintf (name, "%d_%d_%p_%lu", sctk_get_task_rank (),
	   sctk_get_processor_rank (), pid, perform);

  sctk_nodebug ("mpcomp_slave_checkpointing[%d]: Dump in file -> <%s>",
		perform, name);

  sctk_thread_dump (name);

  sctk_nodebug ("mpcomp_slave_checkpointing[%d]: Dump done!", perform);

  return NULL;
}

int
__mpcomp_checkpoint ()
{

  if (sctk_check_point_restart_mode)
    {
      //~ mpcomp_thread_info_t *current_info;
      sctk_microthread_t *current_task;
      int i;
      long step;
      int res;
      int perform;

      /* Initialize the OpenMP environment (call several times, but really executed
       * once) */
      __mpcomp_init ();

      perform = perform_check;

      sctk_nodebug ("mpcomp_checkpoint[%d]: Entering", perform);

      /* Retrieve the information (microthread structure and current region) */
      //~ current_info = sctk_thread_getspecific (mpcomp_thread_info_key);
      //~ sctk_assert (current_info != NULL);

      /* Check if we are not in a parallel region */
      //~ if (current_info->depth != 0)
	//~ {
	  //~ sctk_error
	    //~ ("OpenMP checkpointing has to be called in a sequential part.");
	//~ }

      current_task = sctk_thread_getspecific (sctk_microthread_key);
      sctk_assert (current_task != NULL);

      res = sctk_microthread_add_task (mpcomp_master_checkpointing, NULL, 0,
				       &step, current_task,
				       MPC_MICROTHREAD_LAST_TASK);
      sctk_assert (res == 0);

      sctk_nodebug ("mpcomp_checkpoint[%d]: Master task added", perform);

      //~ for (i = 1; i < current_info->icvs.nmicrovps_var; i++)
      
	{
		i = 1;
	  long step_slave;

	  sctk_nodebug
	    ("mpcomp_checkpoint[%d]: Adding slave task for micro-vp %d",
	     perform, i);

	  res = sctk_microthread_add_task (mpcomp_slave_checkpointing,
					   &(current_task->__list[i]), i,
					   &(step_slave), current_task,
					   MPC_MICROTHREAD_LAST_TASK);
	  sctk_assert (res == 0);
	}

      sctk_nodebug ("mpcomp_checkpoint[%d]: Starting parallel exec...",
		    perform);

      /* Launch the execution of this microthread structure */
      sctk_microthread_parallel_exec (current_task,
				      MPC_MICROTHREAD_DONT_WAKE_VPS);

      sctk_nodebug ("mpcomp_checkpoint[%d]: Parallel execution done...",
		    perform);

      perform_check = perform + 1;


      /* Fill every micro VP with the functions:
         - mpcomp_master_checkpointing (for the first micro VP)
         - mpcomp_slave_checkpointing (for the rest of the micro VPs)
       */
    }

  return 0;
}
