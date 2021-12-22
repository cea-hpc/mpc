/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#include "string.h"

#include "ibtoolkit.h"
#include <execinfo.h>
#include <stdlib.h>


#define HAVE_SHELL_COLORS

static volatile char is_ib_used = 0;

char sctk_network_is_ib_used()
{
	return is_ib_used;
}

void sctk_network_set_ib_used()
{
	is_ib_used = 1;
}

void sctk_network_set_ib_unused()
{
	is_ib_used = 0;
}

#define BT_SIZE 100
void
_mpc_lowcomm_ib_toolkit_print_backtrace ( void )
{
	int j, nptrs;
	void *buffer[BT_SIZE];
	char **strings;

	nptrs = backtrace ( buffer, BT_SIZE );

	strings = backtrace_symbols ( buffer, nptrs );

	if ( strings == NULL )
	{
		perror ( "backtrace_symbols" );
		exit ( EXIT_FAILURE );
	}

	for ( j = 0; j < nptrs; j++ )
		fprintf ( stderr, "%s\n", strings[j] );

	free ( strings );
}
