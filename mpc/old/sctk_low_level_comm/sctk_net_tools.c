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
#ifndef __SCTK__NET_TOOLS_H_
#define __SCTK__NET_TOOLS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "sctk_low_level_comm.h"
  void sctk_net_copy_in_buffer (sctk_thread_ptp_message_t * msg,
				       char *buffer)
  {
    size_t i;
    size_t j;
    size_t size;
      sctk_nodebug ("Write from %p", buffer);
    for (i = 0; i < msg->message.nb_items; i++)
      {
	if (msg->message.begins_absolute[i] != NULL)
	  {
	    for (j = 0; j < msg->message.sizes[i]; j++)
	      {
		size = (msg->message.ends_absolute[i][j] -
			msg->message.begins_absolute[i][j] +
			1) * msg->message.elem_sizes[i];
		memcpy (buffer, ((char *) (msg->message.adresses[i])) +
			msg->message.begins_absolute[i][j] *
			msg->message.elem_sizes[i], size);
		buffer += size;
	      }
	  }
	else
	  {
	    if (msg->message.begins[i] == NULL)
	      {
		size = msg->message.sizes[i];
		memcpy (buffer, msg->message.adresses[i], size);
		buffer += size;
	      }
	    else
	      {
		for (j = 0; j < msg->message.sizes[i]; j++)
		  {
		    size =
		      (msg->message.ends[i][j] - msg->message.begins[i][j] +
		       1) * msg->message.elem_sizes[i];
		    memcpy (buffer, (char *) (msg->message.adresses[i]) +
			    msg->message.begins[i][j] *
			    msg->message.elem_sizes[i], size);
		    buffer += size;
		  }
	      }
	  }
      }
  }

//  static void sctk_net_copy_in_buffer (sctk_thread_ptp_message_t * msg,
//      char *buffer)
//  {
//    size_t i;
//    size_t j;
//    size_t size;
//    void* from;
//    sctk_nodebug ("Write from %p", buffer);
//
//    for (i = 0; i < msg->message.nb_items; i++)
//    {
//      if (msg->message.begins_absolute[i] != NULL)
//      {
//        for (j = 0; j < msg->message.sizes[i]; j++)
//        {
//
//          size = (msg->message.ends_absolute[i][j] -
//              msg->message.begins_absolute[i][j] +
//              1) * msg->message.elem_sizes[i];
//
//          from = (((char *) (msg->message.adresses[i])) +
//                  msg->message.begins_absolute[i][j] * msg->message.elem_sizes[i]);
//
//          switch (size) {
//            case (sizeof(int)) :
//              *((int*) buffer) = *((int*) from);
//              break;
//            case (sizeof(long)) :
//              *((long*) buffer) = *((long*) from);
//              break;
//            case (sizeof(char)) :
//              *((char*) buffer) = *((char*) from);
//              break;
//            default :
//              memcpy (buffer, from, size);
//          }
//          buffer += size;
//        }
//      }
//      else
//      {
//        if (msg->message.begins[i] == NULL)
//        {
//          size = msg->message.sizes[i];
//
//          from = msg->message.adresses[i];
//
//          switch (size) {
//            case (sizeof(int)) :
//              *((int*) buffer) = *((int*) from);
//              break;
//            case (sizeof(long)) :
//              *((long*) buffer) = *((long*) from);
//              break;
//            case (sizeof(char)) :
//              *((char*) buffer) = *((char*) from);
//              break;
//            default :
//              memcpy (buffer, from, size);
//          }
//          buffer += size;
//        }
//        else
//        {
//          for (j = 0; j < msg->message.sizes[i]; j++)
//          {
//            size =
//              (msg->message.ends[i][j] - msg->message.begins[i][j] +
//               1) * msg->message.elem_sizes[i];
//
//          from = (char *) (msg->message.adresses[i]) +
//                msg->message.begins[i][j] *
//                msg->message.elem_sizes[i];
//
//          switch (size) {
//            case (sizeof(int)) :
//              *((int*) buffer) = *((int*) from);
//              break;
//            case (sizeof(long)) :
//              *((long*) buffer) = *((long*) from);
//              break;
//            case (sizeof(char)) :
//              *((char*) buffer) = *((char*) from);
//              break;
//            default :
//              memcpy (buffer, from, size);
//          }
//          buffer += size;
//          }
//        }
//      }
//    }
//  }

  void* sctk_net_if_one_msg_in_buffer (sctk_thread_ptp_message_t * msg)
  {
    size_t i;
    size_t j;
    int nb_msg = 0;
    void* ptr = NULL;

    for (i = 0; i < msg->message.nb_items; i++)
    {
      if (msg->message.begins_absolute[i] != NULL)
      {
        for (j = 0; j < msg->message.sizes[i]; j++)
        {
          ptr =((char *) (msg->message.adresses[i])) +
              msg->message.begins_absolute[i][j] *
              msg->message.elem_sizes[i];

          nb_msg++;
          if (nb_msg >= 2)
            return 0;
        }
      }
      else
      {
        if (msg->message.begins[i] == NULL)
        {
          ptr = msg->message.adresses[i];
          nb_msg++;
          if (nb_msg >= 2)
            return 0;
        }
        else
        {
          for (j = 0; j < msg->message.sizes[i]; j++)
          {
            ptr = (char *) (msg->message.adresses[i]) +
                msg->message.begins[i][j] *
                msg->message.elem_sizes[i];

            nb_msg++;
            if (nb_msg >= 2)
              return 0;
          }
        }
      }
    }
    return ptr;
  }



  size_t sctk_net_determine_message_size (sctk_thread_ptp_message_t *
						 msg)
  {
    size_t total_size = 0;
    size_t i;

    for (i = 0; i < msg->message.nb_items; i++)
      {
	if (msg->message.begins_absolute[i] != NULL)
	  {
	    size_t j;
	    for (j = 0; j < msg->message.sizes[i]; j++)
	      {
		total_size +=
		  (msg->message.ends_absolute[i][j] -
		   msg->message.begins_absolute[i][j] +
		   1) * msg->message.elem_sizes[i];
	      }
	  }
	else
	  {
	    if (msg->message.begins[i] == NULL)
	      {
		total_size += msg->message.sizes[i];
	      }
	    else
	      {
		size_t j;
		for (j = 0; j < msg->message.sizes[i]; j++)
		  {
		    total_size +=
		      (msg->message.ends[i][j] - msg->message.begins[i][j] +
		       1) * msg->message.elem_sizes[i];
		  }
	      }
	  }
      }

    return total_size;
  }


#if (defined(__GNUC__) || defined(__ICC)) && defined(__x86_64__)

  static inline void
amd64_cpy_nt ( volatile void *dst, const volatile void *src, const size_t n ) {
	size_t n32 = ( n ) >> 5;
	size_t nleft = ( n ) & ( 32 - 1 );

	if ( n32 ) {
		__asm__ __volatile__ ( ".align 16  \n"
		                       "1:  \n"
		                       "mov (%1), %%r8  \n"
		                       "mov 8(%1), %%r9  \n"
		                       "add $32, %1  \n"
		                       "movnti %%r8, (%2)  \n"
		                       "movnti %%r9, 8(%2)  \n"
		                       "add $32, %2  \n"
		                       "mov -16(%1), %%r8  \n"
		                       "mov -8(%1), %%r9  \n"
		                       "dec %0  \n"
		                       "movnti %%r8, -16(%2)  \n"
		                       "movnti %%r9, -8(%2)  \n"
		                       "jnz 1b  \n"
		                       "sfence  \n"
	                       "mfence  \n":"+a" ( n32 ), "+S" ( src ),
		                       "+D" ( dst ) ::"r8",
		                       "r9" );
	}


	if ( nleft ) {
		memcpy ( ( void * ) dst, ( void * ) src, nleft );
	}
}
#else
static  inline void
amd64_cpy_nt ( volatile void *dst, volatile void *src, size_t n ) {
	memcpy ( ( void * ) dst, ( void * ) src, n );
}
#endif

static inline size_t
copy_frag ( char *msg,
                    size_t size,
                    char *buffer,
                    size_t curr_copy,
                    size_t max_copy,
                    size_t tot_size,
                    int* go_on ) {
	size_t size_frag_msg;

	sctk_nodebug ( "size of the msg: %d (cur_cop %d) (tot_size %d)", size, curr_copy, tot_size );

	/* if this msg's part has to be copyied */
	if ( ( tot_size + size > curr_copy ) | ( curr_copy == 0 ) ) {

		/* it remains data to copy from this msg */
		if ( curr_copy > tot_size ) {

			/* segment size which remains to copy */
			size_frag_msg = ( size - ( curr_copy - tot_size ) );
			sctk_nodebug
			( "Copy the remain (%d) of the msg, (size : %d) (curr_copy : %d) (tot_size : %d) (max_copy %d)",
			  size_frag_msg, size, curr_copy, tot_size, max_copy );

			/*  if the frag size is greater than the max size */
			if ( size_frag_msg > max_copy ) {
				sctk_nodebug ( "Msg is greater than the max msg size again" );

				/* copy the max size */
				amd64_cpy_nt ( buffer, ( char* ) msg + curr_copy, max_copy );

				/* return the size of all copied data */
				*go_on = 0;
				return curr_copy + max_copy;
			} else {
				/* copy the whole remaining message */
				sctk_nodebug ( "Copy the whole remaining msg (size_frag_msg %d) (deb %hhx) (first %hhx)", size_frag_msg, msg, ( ( char* ) msg ) + ( size - size_frag_msg ) ) ;
				amd64_cpy_nt ( buffer, ( char * ) msg + ( size - size_frag_msg ), size_frag_msg );

				*go_on = 1;
				return curr_copy + size_frag_msg;
			}

		}
		/* there is no more space for the new message. Slot filled at max */
		else if ( ( tot_size - curr_copy ) == max_copy ) {
			sctk_nodebug ( "No more space for the msg" );
			*go_on = 0;
			return tot_size;
		}
		/* if the new msg to copy is greater thant the max size */
		else if ( size + ( tot_size - curr_copy ) > max_copy ) {
			/* msg needs to be splitted in several parts */
			size_frag_msg = max_copy - ( tot_size - curr_copy );

			sctk_nodebug ( "Msg is greater than the max size (size %d) (max %d) (tot_size %d) (curr_copy %d) (size_frag %d) (deb %hhx) (next %hhx)", size, max_copy, tot_size, curr_copy, size_frag_msg, msg, ( ( ( char* ) msg ) + size_frag_msg ) );

			/* copy the max size that we can copy */
			amd64_cpy_nt ( buffer + ( tot_size - curr_copy ), msg, size_frag_msg );

			/* return the new size */
			*go_on = 0;
			return tot_size + size_frag_msg;
		} else {
			if ( curr_copy != 0 )
				sctk_nodebug ( "Copy the whole msg (size %d) (curr %d) (tot %d) (max %d)", size, curr_copy, tot_size, max_copy );

			/* copy the whole msg */
			amd64_cpy_nt ( buffer + ( tot_size - curr_copy ), msg, size );
			*go_on = 1;
			return tot_size + size;
		}
	} else {
		sctk_nodebug ( "Already copied. Skipping the msg (tot %d) (size %d)", tot_size, size );
		*go_on = 1;
		return tot_size + size;
	}
}


int sctk_net_copy_frag_msg (
    const sctk_thread_ptp_message_t * msg,
    char *buffer,
    const size_t curr_copy,
    const size_t max_copy ) {
	size_t i;
	size_t j;
	size_t size;
	size_t tot_size = 0;
	int go_on;

	sctk_nodebug ( "Write from %p", buffer );
	sctk_nodebug ( "Curr_copy (%u), max_copy (%u)", curr_copy, max_copy );

	for ( i = 0; i < msg->message.nb_items; i++ ) {
		if ( msg->message.begins_absolute[i] != NULL ) {
			for ( j = 0; j < msg->message.sizes[i]; j++ ) {
				size = ( msg->message.ends_absolute[i][j] -
				         msg->message.begins_absolute[i][j] +
				         1 ) * msg->message.elem_sizes[i];

				sctk_nodebug ( "size 0: %d", size );

				tot_size = copy_frag (
				            ( ( char * ) ( msg->message.adresses[i] ) ) +
				            msg->message.begins_absolute[i][j] *
				            msg->message.elem_sizes[i],
				            size,
				            buffer,
				            curr_copy,
				            max_copy,
				            tot_size,
				            &go_on );

				sctk_nodebug ( "Tot size : %ld", tot_size );

				if ( go_on == 0 ) {
					sctk_nodebug ( "Tot size : %ld", tot_size );
					return tot_size;
				}
			}
		} else {
			if ( msg->message.begins[i] == NULL ) {
				sctk_nodebug ( "size 1: %d", msg->message.sizes[i] );

				tot_size = copy_frag (
				            msg->message.adresses[i],
				            msg->message.sizes[i],
				            buffer,
				            curr_copy,
				            max_copy,
				            tot_size,
				            &go_on );

				sctk_nodebug ( "Tot size : %ld", tot_size );

				sctk_nodebug ( "Curr copy : %d", curr_copy );

				if ( go_on == 0 ) {
					sctk_nodebug ( "Tot size : %ld", tot_size );
					return tot_size;
				}
			} else {
				for ( j = 0; j < msg->message.sizes[i]; j++ ) {
					size =
					 ( msg->message.ends[i][j] - msg->message.begins[i][j] +
					   1 ) * msg->message.elem_sizes[i];

					sctk_nodebug ( "size 2: %d", size );

					tot_size = copy_frag (
					            ( char * ) ( msg->message.adresses[i] ) +
					            msg->message.begins[i][j] *
					            msg->message.elem_sizes[i],
					            size,
					            buffer,
					            curr_copy,
					            max_copy,
					            tot_size,
					            &go_on );

					sctk_nodebug ( "Tot size : %ld", tot_size );

					if ( go_on == 0 ) {
						sctk_nodebug ( "Tot size : %ld", tot_size );
						return tot_size;
					}
				}
			}
		}
	}

	sctk_nodebug ( "End of function (curr_copy %d) (max %d) (tot_size %d)", curr_copy, max_copy, tot_size );
	return tot_size;
}

#ifdef __cplusplus
}
#endif
#endif
