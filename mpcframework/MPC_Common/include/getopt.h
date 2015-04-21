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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_GETOPT_H
#define SCTK_GETOPT_H



#ifdef MPC_GETOPT_ENABLED
/* Getopt Support */

#define _GETOPT_H
#include <unistd.h>
#include <getopt.h>

/* Getopt variables */
extern char * sctk_optarg;
extern int sctk_optind, sctk_opterr, sctk_optopt, sctk_optreset, sctk_optpos;

/* Define the option struct */
struct option
{
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

/* Redefine the getopt modifiers */
#define no_argument        0
#define required_argument  1
#define optional_argument  2

/* SCTK getopt implementation */
int sctk_getopt(int, char * const [], const char *);
int sctk_getopt_long(int, char *const *, const char *, const struct option *, int *);
int sctk_getopt_long_only(int, char *const *, const char *, const struct option *, int *);

/* Rewrite getopt variables */
#define optarg sctk_optarg
#define optind sctk_optind
#define opterr sctk_opterr
#define optopt sctk_optopt
#define optreset sctk_optreset
#define optpos sctk_optpos

/* Rewrite getopt functions */
#define getopt_long sctk_getopt_long
#define getopt_long_only sctk_getopt_long_only
#define getopt sctk_getopt
#define __getopt_msg sctk___getopt_msg

/* End of getopt support */
#endif

#endif /* SCTK_GETOPT_H */
