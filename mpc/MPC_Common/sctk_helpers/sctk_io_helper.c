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

#include "sctk_io_helper.h"
#include "sctk_debug.h"
#include <errno.h>
#include <stdio.h>

#ifdef MPC_Threads
volatile int sctk_online_program = -1;
#else
volatile int sctk_online_program = 1;
#endif

/*!
 * Call read in loop to avoid problem with splitted messages.
 * Also support EINTR error if interrupted.
 * @param fd File descriptor to read.
 * @param buf Buffer to read.
 * @param count Size of buffer to read.
*/
ssize_t sctk_safe_read(int fd, void* buf, size_t count) {
	//vars
	int tmp, nb_total_sent_bytes = 0;
	int res = count;

	//loop until read all
	while (nb_total_sent_bytes < count) {
		tmp = read(fd, (char *)buf + nb_total_sent_bytes, count - nb_total_sent_bytes);

		//check errors
		if (tmp == 0) {
			break;
		} else if (tmp < 0) {
			//on interuption continue to re-read
			if (errno == EINTR) {
				continue;
			} else {
				sctk_debug ("READ %p %lu/%lu FAIL\n", buf, count);
				perror("sctk_safe_read");
				res = -1;
				break;
			}
		}

		//update size counter
		nb_total_sent_bytes += tmp;
	};
	
	if(sctk_online_program == 0){
	  exit(0);
	}
	return res;
}

/*!
 * Call write in loop to avoid problem with splitted messages.
 * Also support EINTR error if interrupted.
 * @param fd File descriptor to write.
 * @param buf Buffer to write.
 * @param count Size of buffer to write.
*/
ssize_t sctk_safe_write(int fd, const void* buf, size_t count)
{
	//vars
	int tmp, nb_total_sent_bytes = 0;
	int res = count;

	//loop until read all
	while (nb_total_sent_bytes < count) {
		tmp = write(fd, (char *)buf + nb_total_sent_bytes, count - nb_total_sent_bytes);

		//check errors
		if (tmp < 0) {
			//on interuption continue to re-read
			if (errno == EINTR) {
				continue;
			} else {
				sctk_debug ("WRITE %p %lu/%lu FAIL\n", buf, count);
				perror("sctk_safe_read");
				res = -1;
				break;
			}
		}

		//update size counter
		nb_total_sent_bytes += tmp;
	};
	
	return res;
}

/*!
 * Same than sctk_safe_read but abort on error or non complete read.
*/
ssize_t sctk_safe_checked_read(int fd, void* buf, size_t count)
{
	ssize_t res = sctk_safe_read(fd,buf,count);
	assume(res == count);
	return res;
}

/*!
 * Same than sctk_safe_write but abort on error or non complete write.
*/
ssize_t sctk_safe_checked_write(int fd, void* buf, size_t count)
{
	ssize_t res = sctk_safe_write(fd,buf,count);
	assume(res == count);
	return res;
}
