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
/* #   - VALAT Sébastien sebastien.valat@cea.fr                           # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

/********************************* INCLUDES *********************************/
#include "mpc_common_io_helper.h"
#include "sctk_debug.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/********************************** GLOBALS *********************************/

static inline int _mpc_io_all_digits(char * str)
{
	int len = strlen(str);

	int i;
	for( i = 0 ; i < len ; i++ )
	{
		if(!isdigit(str[i]))
		{
			return 0;
		}
	}

	return 1;
}

long mpc_common_parse_long(char * input)
{
	if(!_mpc_io_all_digits(input))
	{
		sctk_fatal("Could not parse value '%s' expected integer", input);
	}

	long ret = 0;
	char * endptr = NULL;

	errno = 0;
	ret = strtol(input, &endptr, 10);

	if ((errno == ERANGE
	     && (ret == LONG_MAX || ret == LONG_MIN))
	|| (errno != 0 && ret == 0)) {
		perror("strtol");
		sctk_fatal("Could not parse value '%s' expected integer", input);
	}

	if (endptr == input) {
		sctk_fatal("Could not parse value '%s' expected integer", input);
	}

	return ret;
}

ssize_t mpc_common_io_safe_read(int fd, void* buf, size_t count)
{
	/* vars */
	int tmp = 0;
	size_t nb_total_received_bytes = 0;
	int res = count;

	if( count == 0 )
	{
		return 1;
	}

	/* loop until read all */
	while (nb_total_received_bytes < count) {
		tmp = read(fd, (char *)buf + nb_total_received_bytes, count - nb_total_received_bytes);

		/* check errors */
		if (tmp == 0) {
			res = nb_total_received_bytes;
			break;
		} else if (tmp < 0) {
			/* on interuption continue to re-read */
			if (errno == EINTR) {
				continue;
			}
			else if(errno == EBADF) /* possibly closed socket */
			{
				sctk_nodebug("Socket %d not valid anymore !", fd);
				res = -1;
				break;
			}
			else {
				sctk_debug ("READ %p %lu/%lu FAIL\n", buf, count);
				perror("mpc_common_io_safe_read");
				res = -1;
				break;
			}
		}

		/* update size counter */
		nb_total_received_bytes += tmp;
	};


	return res;
}

ssize_t mpc_common_io_safe_write(int fd, const void* buf, size_t count)
{
	/* vars */
	int tmp;
	size_t nb_total_sent_bytes = 0;
	int res = count;

	/* loop until read all */
	while (nb_total_sent_bytes < count) {
		tmp = write(fd, (char *)buf + nb_total_sent_bytes, count - nb_total_sent_bytes);

		/* check errors */
		if (tmp < 0) {
			/* on interuption continue to re-read */
			if (errno == EINTR) {
				continue;
			}
			else if(errno == EBADF) /* possibly closed socket */
			{
				sctk_nodebug("Socket %d not valid anymore !", fd);
				res = -1;
				break;
			} else {
				sctk_debug ("WRITE %p %lu/%lu FAIL\n", buf, count);
				perror("mpc_common_io_safe_write");
				res = -1;
				break;
			}
		}

		/* update size counter */
		nb_total_sent_bytes += tmp;
	};

	return res;
}
