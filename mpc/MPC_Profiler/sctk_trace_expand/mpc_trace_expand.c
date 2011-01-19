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
/* #                                                                      # */
/* ######################################################################## */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include "sctk_config.h"
#include "sctk_trace.h"
#ifdef MPC_USE_ZLIB
#include <zlib.h>
#endif

FILE *in_header;
FILE *in_trace;
FILE *out_trace;

int sctk_process_rank;
int sctk_process_number;
int compression;
int print_per_thread = 0;

char *start_point = NULL;
char *end_point = NULL;
int require_print = 1;

char *tmp_buffer_compress;
void **thread_list = NULL;
unsigned long thread_list_size = 0;
int print_thread_list = 0;
void *thread_to_check = NULL;

static void
add_thread (void *th)
{
  int i;
  for (i = 0; i < thread_list_size; i++)
    {
      if (thread_list[i] == th)
	{
	  return;
	}
    }
  thread_list_size++;
  thread_list = realloc (thread_list, thread_list_size * sizeof (void *));
  thread_list[thread_list_size - 1] = th;
}

typedef struct sctk_items_s
{
  int i;
  long adress;
  int size;
  char *name;
  unsigned long nb_call;
  double average_time;
  double last_time;
  struct sctk_items_s *related;
} sctk_items_t;

sctk_items_t *tab = NULL;
int max_i;

static inline int sctk_trace_read (FILE * fd_in, FILE * fd, void *thread);

static void
print_func_stat ()
{
  int i;
  fprintf (stdout, "Statistics:\n");
  for (i = 0; i < max_i; i++)
    {
      if (memcmp (tab[i].name, "START_MPC_TRACE__", 17) == 0)
	{
	  if (tab[i].nb_call)
	    tab[i].average_time = tab[i].average_time / tab[i].nb_call;
	  fprintf (stdout, "\t%-50s %10lu (%30.0f)\n", &(tab[i].name[17]),
		   tab[i].nb_call, tab[i].average_time);
	}
      else
	{
	  if (tab[i].related)
	    {
	      if (tab[i].related->nb_call != tab[i].nb_call)
		fprintf (stderr,
			 "Error for function %s: nb start != nb end\n",
			 tab[i].name);
	    }
	}
    }
}

static void
build_thread_stats_init ()
{
  int i;
  for (i = 0; i < max_i; i++)
    {
      tab[i].nb_call = 0;
      tab[i].average_time = 0;
    }
}

static void
build_thread_stats (void *thread)
{
  if (thread != NULL)
    {
      fseek (in_trace, 0, SEEK_SET);
      fprintf (stderr, "View Thread %lx\n", (unsigned long) thread);
      build_thread_stats_init ();
      while (sctk_trace_read (in_trace, out_trace, thread));
    }
}

static sctk_items_t *
build_related (char *name, int m)
{
  if (memcmp (name, "START_MPC_TRACE__", 17) == 0)
    {
      return NULL;
    }
  if (memcmp (name, "END___MPC_TRACE__", 17) == 0)
    {
      int i;
      sctk_items_t *tmp;
      static char tmp_name[4096];
      sprintf (tmp_name, "START_MPC_TRACE__%s", &(name[17]));
      for (i = 0; i < m; i++)
	{
	  if (strcmp (tmp_name, tab[i].name) == 0)
	    {
	      tmp = &(tab[i]);
	    }
	}
      assert (tmp != NULL);
      return tmp;
    }
  if (memcmp (name, "POINT_MPC_TRACE__", 17) == 0)
    {
      return NULL;
    }
  assert (0);
  return NULL;
}

static void
read_file_header ()
{
  int sizeof_long;
  char Prototype[4096];
  int max_name;
  int i;
  fscanf (in_header, "%d\n", &sctk_process_rank);
  fprintf (stderr, "sctk_process_rank %d\n", sctk_process_rank);
  fscanf (in_header, "%d\n", &sizeof_long);
  fprintf (stderr, "sizeof long %d\n", sizeof_long);
  if (sizeof_long != sizeof (long))
    {
      fprintf (stderr,
	       "Unable to read this file: sizeof long are different (%d != %d)\n",
	       (int) sizeof_long, (int) sizeof (long));
      exit (1);
    }
  fscanf (in_header, "%s\n", Prototype);
  fprintf (stderr, "Prototype %s\n", Prototype);
  fscanf (in_header, "%d\n", &max_i);
  fprintf (stderr, "NB Items %d\n", max_i);
  tab = realloc (tab, max_i * sizeof (sctk_items_t));
  fscanf (in_header, "%d\n", &max_name);
  fprintf (stderr, "max_name %d\n", max_name);
  for (i = 0; i < max_i; i++)
    {
      tab[i].name = realloc (tab[i].name, max_name + 4);
      memset (tab[i].name, '\0', max_name + 4);
      fscanf (in_header, Prototype,
	      &(tab[i].i), &(tab[i].adress), &(tab[i].size), tab[i].name);
      tab[i].related = build_related (tab[i].name, i);
    }
}

static inline char *
get_func_name (char **addr, int *rank)
{
  int i;
  *rank = -1;
  for (i = 0; i < max_i; i++)
    {
      if ((long) (addr) == (long) (tab[i].adress))
	{
	  *rank = i;
	  return &(tab[i].name[17]);
	}
    }
  return NULL;
}

unsigned long nb_lines = 0;
unsigned long nb_empty_lines = 0;

static inline int
sctk_trace_read (FILE * fd_in, FILE * fd, void *thread)
{
  static sctk_trace_t cur_trace;
  int i;
  int res;
  if (compression)
    {
#ifdef MPC_USE_ZLIB
      uLong source_len;
      uLongf dest_len;
      res = fread (&source_len, sizeof (uLong), 1, fd_in);
      if (0 == res)
	{
/* 	  fprintf(stderr,"No entry\n"); */
	  return res;
	}			/* else { */
/* 	  fprintf(stderr,"1 entry\n"); */

/*       } */
      res = fread (tmp_buffer_compress, 1, source_len, fd_in);
      assert (res == source_len);
      dest_len = sizeof (sctk_trace_t);
      res =
	uncompress ((Bytef *) (&cur_trace), &dest_len,
		    (Bytef *) tmp_buffer_compress, source_len);
      if (res != Z_OK)
	{
	  fprintf (stderr,
		   "res %d Z_OK=%d Z_MEM_ERROR=%d Z_BUF_ERROR=%d Z_DATA_ERROR=%d\n",
		   res, Z_OK, Z_MEM_ERROR, Z_BUF_ERROR, Z_DATA_ERROR);
	}
      assert (dest_len == sizeof (sctk_trace_t));
      res = 1;
#else
      assert (0);
#endif
    }
  else
    {
      res = fread (&cur_trace, sizeof (sctk_trace_t), 1, fd_in);
    }
  if (0 == res)
    {
      return res;
    }
  else
    {
      if (res != 1)
	{
	  fprintf (stderr, "Fail to read tracefile\n");
	  abort ();
	}
    }
/*   fprintf(stderr,"Print entries\n"); */
  for (i = 0; i < NB_ENTRIES; i++)
    {
      if (cur_trace.buf[i].function != NULL)
	{
	  char *func_name;
	  int rank_in_tab;
	  double time_cons = 0;
	  add_thread (cur_trace.buf[i].th);
	  if ((thread == NULL)
	      || ((unsigned long) cur_trace.buf[i].th ==
		  (unsigned long) thread))
	    {
	      func_name =
		get_func_name (cur_trace.buf[i].function, &rank_in_tab);
	      if (start_point
		  && (strcmp (start_point, tab[rank_in_tab].name) == 0))
		{
		  require_print = 1;
		}
	      tab[rank_in_tab].nb_call++;
	      if (tab[rank_in_tab].related)
		{
		  time_cons =
		    (cur_trace.buf[i].date -
		     tab[rank_in_tab].related->last_time);
		  tab[rank_in_tab].related->average_time += time_cons;
		  tab[rank_in_tab].last_time = 0;
		}
	      else
		{
		  tab[rank_in_tab].last_time = cur_trace.buf[i].date;
		}
/* 	      fd = stdout; */
/* 	      fprintf(stderr,"fd %p, require_print %d \n",fd,require_print); */
	      if (fd && require_print)
		{
		  fprintf (fd,
			   "[%05d]\t[%05d]\t%p\t%20.3f\t(%p)%-50s\t%12p\t%12p\t%12p\t%12p\t%f\n",
			   sctk_process_rank,
			   cur_trace.buf[i].vp,
			   cur_trace.buf[i].th,
			   cur_trace.buf[i].date,
			   cur_trace.buf[i].function, func_name,
			   cur_trace.buf[i].arg1,
			   cur_trace.buf[i].arg2,
			   cur_trace.buf[i].arg3, cur_trace.buf[i].arg4,
			   time_cons);
		}
	      if (end_point
		  && (strcmp (end_point, tab[rank_in_tab].name) == 0))
		{
		  require_print = 0;
		}
	    }
	  nb_lines++;
	}
      else
	{
	  nb_empty_lines++;
	}
    }
  return res;
}

static void
read_file_trace ()
{
  while (sctk_trace_read (in_trace, out_trace, NULL));
}

typedef struct
{
  char *name;
  int (*func) (int, int, char **);
  char *desc;
  char *proto;
} trace_option_t;

#define MAX_OPTION 20
trace_option_t option_tab[MAX_OPTION];
#define ADD_OPT(a,b,d,c) do{			\
    option_tab[i].proto=d;			\
    option_tab[i].name=a;			\
    option_tab[i].func=c;			\
    option_tab[i].desc=b;			\
    i++;					\
    assert(i<=MAX_OPTION);			\
  }while(0)


static int
print_help (int i, int argc, char **argv)
{
  int j;
  assert (i < argc);
  fprintf (stderr, "Usage: %s [ ", basename (argv[0]));
  for (j = 0; j < MAX_OPTION; j++)
    {
      if (option_tab[j].name != NULL)
	{
	  fprintf (stderr, "%s ", option_tab[j].proto);
	}
    }
  fprintf (stderr, "] dir\n");
  for (j = 0; j < MAX_OPTION; j++)
    {
      if (option_tab[j].name != NULL)
	{
	  fprintf (stderr, "\t%s %s\n", option_tab[j].name,
		   option_tab[j].desc);
	}
    }
  return 0;
}

static int
select_output_file (int i, int argc, char **argv)
{
  int nbleft;

  nbleft = argc - i;
  if (nbleft < 2)
    {
      fprintf (stderr, "Invalid argument\n");
      print_help (i, argc, argv);
      exit (1);
    }
  fprintf (stderr, "Output file %s\n", argv[i + 1]);
  out_trace = fopen (argv[i + 1], "w");
  if (out_trace == NULL)
    {
      fprintf (stderr, "Unable to create file %s\n", argv[i + 1]);
      exit (1);
    }
  return 2;
}

static int
set_thread (int i, int argc, char **argv)
{
  int nbleft;

  nbleft = argc - i;
  if (nbleft < 2)
    {
      fprintf (stderr, "Invalid argument\n");
      print_help (i, argc, argv);
      exit (1);
    }
  sscanf (argv[i + 1], "%lx", (unsigned long *) (&thread_to_check));
  fprintf (stderr, "Follow thread %s (%lx)\n", argv[i + 1],
	   (unsigned long) thread_to_check);
  return 2;
}

static int
disable_output_file (int i, int argc, char **argv)
{
  assert (i < argc);
  assert (argv != NULL);
  out_trace = NULL;
  return 1;
}

static int
print_thread_list_f (int i, int argc, char **argv)
{
  assert (i < argc);
  assert (argv != NULL);
  print_thread_list = 1;
  return 1;
}

static int
per_thread (int i, int argc, char **argv)
{
  assert (i < argc);
  assert (argv != NULL);
  print_per_thread = 1;
  return 1;
}

static int
view_func (int i, int argc, char **argv)
{
  int nbleft;
  assert (i < argc);

  nbleft = argc - i;
  if (nbleft < 2)
    {
      fprintf (stderr, "Invalid argument\n");
      print_help (i, argc, argv);
      exit (1);
    }
  require_print = 0;
  start_point = realloc (start_point, 4096);
  end_point = realloc (end_point, 4096);
  sprintf (start_point, "START_MPC_TRACE__%s", argv[i + 1]);
  sprintf (end_point, "END___MPC_TRACE__%s", argv[i + 1]);
  fprintf (stderr, "Follow form %s to %s\n", start_point, end_point);

/* int require_print = 0; */
/* char* start_point = "START_MPC_TRACE__free"; */
/* char* end_point =   "END___MPC_TRACE__free"; */

  return 2;
}

static void
init_option ()
{
  int i = 0;
  ADD_OPT ("-h", "Print this message", "[-h]", print_help);
  ADD_OPT ("-o", "Redirect output in file out_file", "[-o out_file]",
	   select_output_file);
  ADD_OPT ("-no_out", "Disable ASCII Generation", "[-no_out]",
	   disable_output_file);
  ADD_OPT ("-thread_list", "Print thread list", "[-thread_list]",
	   print_thread_list_f);
  ADD_OPT ("-thread", "View only thread thread", "[-thread thread]",
	   set_thread);
  ADD_OPT ("-per_thread", "View each thread", "[-per_thread]", per_thread);
  ADD_OPT ("-view_func", "View a specified function func and his sub calls",
	   "[-view_func func]", view_func);
}

static int
threat_args (int i, int argc, char **argv)
{
  int j;
  assert (i < argc);

  for (j = 0; j < MAX_OPTION; j++)
    {
      if (option_tab[j].name != NULL)
	{
	  if (strcmp (argv[i], option_tab[j].name) == 0)
	    {
	      int res;
	      res = option_tab[j].func (i, argc, argv);
	      if (res == 0)
		exit (0);
	      return res;
	    }
	}
    }

  fprintf (stderr, "Unknown arg %s\n", argv[i]);
  print_help (i, argc, argv);
  exit (1);
  return 0;
}

int
main (int argc, char **argv)
{
  char *header_name;
  char header[4096];
  char trace[4096];
  unsigned long str_size;
  int i;
  FILE *orig_output;

  out_trace = stdout;
  init_option ();

  header_name = argv[argc - 1];

  argc--;

  if (strcmp ("-h", header_name) == 0)
    {
      argc++;
    }

  for (i = 1; i < argc;)
    {
      i += threat_args (i, argc, argv);
    }


  orig_output = out_trace;
/*   out_trace = NULL; */

  str_size = sizeof (sctk_trace_t);
  str_size = 2 * (str_size);
  tmp_buffer_compress = malloc (str_size);

  sprintf (header, "%s/trace_header_0", header_name);
  fprintf (stderr, "Line size %f\n",
	   ((double) sizeof (sctk_trace_t)) / ((double) NB_ENTRIES));

  fprintf (stderr, "Header file %s\n", header);
  in_header = fopen (header, "r");
  if (in_header == NULL)
    {
      fprintf (stderr, "Unable to open header file %s\n", header);
      exit (1);
    }

  fscanf (in_header, "%s\n", trace);
  fscanf (in_header, "%d\n", &sctk_process_number);
  fscanf (in_header, "%d\n", &compression);


  fprintf (stderr, "Trace file %s\n", trace);
  fprintf (stderr, "Process number %d\n", sctk_process_number);

  fclose (in_header);
  for (i = 0; i < sctk_process_number; i++)
    {
      sprintf (header, "%s/trace_header_%d", header_name, i);

      in_header = fopen (header, "r");
      if (in_header == NULL)
	{
	  fprintf (stderr, "Unable to open header file %s\n", header);
	  exit (1);
	}

      fscanf (in_header, "%s\n", trace);
      fscanf (in_header, "%d\n", &sctk_process_number);
      fscanf (in_header, "%d\n", &compression);

      fprintf (stderr, "Threat %s compression %d\n", trace, compression);

      in_trace = fopen (trace, "r");
      if (in_trace == NULL)
	{
	  fprintf (stderr, "Unable to open trace file %s\n", trace);
	  exit (1);
	}
      read_file_header ();
      build_thread_stats_init ();
      read_file_trace ();
      fprintf (stderr, "%lu/%lu Lines (%lu chunks)\n", nb_lines,
	       nb_lines + nb_empty_lines,
	       (nb_lines + nb_empty_lines) / NB_ENTRIES);

      out_trace = orig_output;
      fprintf (stderr, "Thread number %lu\n", thread_list_size);
      if (print_per_thread == 0)
	{
	  build_thread_stats (thread_to_check);
	  print_func_stat ();
	}
      else
	{
	  for (i = 0; i < thread_list_size; i++)
	    {
	      build_thread_stats (thread_list[i]);
	      print_func_stat ();
	    }
	}
    }
  fprintf (stderr, "Total thread number %lu (%d)\n", thread_list_size,
	   print_thread_list);
  if (print_thread_list == 1)
    {
      fprintf (stdout, "Thread list\n");
      for (i = 0; i < thread_list_size; i++)
	{
	  fprintf (stdout, "\t%lx\n", (unsigned long) thread_list[i]);
	}
    }
  return 0;
}
