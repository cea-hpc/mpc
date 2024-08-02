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
#include <mpc_common_helper.h>

#include "mpc_common_debug.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sctk_alloc.h>
#include <sctk_alloc.h>



/***********************
 * TRIMMING OF STRINGS *
 ***********************/

char * mpc_common_trim(char * path)
{
	/* From start */
	char * ret = path;

	while( (*ret == ' ' || *ret == '\t') && (*ret != '\0') )
	{
		ret++;
	}

	/* From end */
	unsigned long len = strlen(ret);

	if(len == 0)
	{
		return ret;
	}

	unsigned long  off = len - 1;

	while( ( ret[off] == ' ' || ret[off] == '\t' || ret[off] == '\n' ) && (0 <= off) )
	{
		ret[off] = '\0';
		off--;
	}	

	return ret;
}

int mpc_common_escape_string(const char* str, char *out_str_double_len_of_str, size_t outlen) {
    unsigned long len = strlen(str);

    out_str_double_len_of_str[0] = '\0';  // Initialize the string with a null terminator

    unsigned long i = 0;
    unsigned long j = 0;
    for (i = 0; i < len; i++) {
        char c = str[i];
        switch (c) {
            case '\\':
                out_str_double_len_of_str[j++] = '\\';
                out_str_double_len_of_str[j++] = '\\';
                break;
            case '\b':
                out_str_double_len_of_str[j++] = '\\';
                out_str_double_len_of_str[j++] = 'b';
                break;
            case '\f':
                out_str_double_len_of_str[j++] = '\\';
                out_str_double_len_of_str[j++] = 'f';
                break;
            case '\n':
                out_str_double_len_of_str[j++] = '\\';
                out_str_double_len_of_str[j++] = 'n';
                break;
            case '\r':
                out_str_double_len_of_str[j++] = '\\';
                out_str_double_len_of_str[j++] = 'r';
                break;
            case '\t':
                out_str_double_len_of_str[j++] = '\\';
                out_str_double_len_of_str[j++] = 't';
                break;
			default:
				break;
        }
        // If the character is printable ASCII, just append it
        if (c >= ' ' && c <= '~') {
            out_str_double_len_of_str[j++] = c;
        }

		if(j == (outlen - 1))
		{
			out_str_double_len_of_str[j] = '\0';
			return 1;
		}
    }

    // Null-terminate the string and return it
    out_str_double_len_of_str[j] = '\0';

	return 0;
}

/********************************** GLOBALS *********************************/

static inline int _mpc_io_all_digits( char *str )
{
	int len = strlen( str );
	int i;

	for ( i = 0 ; i < len ; i++ )
	{
		if ( !isdigit( str[i] ) )
		{
			return 0;
		}
	}

	return 1;
}

long mpc_common_parse_long( char *input )
{
	if ( !_mpc_io_all_digits( input ) )
	{
		mpc_common_debug_fatal( "Could not parse value '%s' expected integer", input );
	}

	long ret = 0;
	char *endptr = NULL;
	errno = 0;
	ret = strtol( input, &endptr, 10 );

	if ( ( errno == ERANGE
	       && ( ret == LONG_MAX || ret == LONG_MIN ) )
	     || ( errno != 0 && ret == 0 ) )
	{
		perror( "strtol" );
		mpc_common_debug_fatal( "Could not parse value '%s' expected integer", input );
	}

	if ( endptr == input )
	{
		mpc_common_debug_fatal( "Could not parse value '%s' expected integer", input );
	}

	return ret;
}

ssize_t mpc_common_io_safe_read( int fd, void *buf, size_t count )
{
	/* vars */
	size_t nb_total_received_bytes = 0;
	int res = count;

	if ( count == 0 )
	{
		return 1;
	}

	/* loop until read all */
	while ( nb_total_received_bytes < count )
	{
		int tmp = read( fd, ( char * )buf + nb_total_received_bytes, count - nb_total_received_bytes );

		/* check errors */
		if ( tmp == 0 )
		{
			if(nb_total_received_bytes != count)
			{
				/* Early EOF */
				res = -1;
			}
			else
			{
				res = nb_total_received_bytes;
			}

			break;
		}
		else if ( tmp < 0 )
		{
			/* on interuption continue to re-read */
			if ( errno == EINTR )
			{
				continue;
			}
			else if ( errno == EBADF ) /* possibly closed socket */
			{
				mpc_common_nodebug( "Socket %d not valid anymore !", fd );
				res = -1;
				break;
			}
			else
			{
				mpc_common_debug( "READ %p %lu/%lu FAIL\n", buf, count );
				//perror( "mpc_common_io_safe_read" );
				res = -1;
				break;
			}
		}

		/* update size counter */
		nb_total_received_bytes += tmp;
	};

	return res;
}

ssize_t mpc_common_io_safe_write( int fd, const void *buf, size_t count )
{
	/* vars */
	size_t nb_total_sent_bytes = 0;
	int res = count;

	/* loop until read all */
	while ( nb_total_sent_bytes < count )
	{
		int tmp = write( fd, ( char * )buf + nb_total_sent_bytes, count - nb_total_sent_bytes );

		/* check errors */
		if ( tmp < 0 )
		{
			/* on interuption continue to re-read */
			if ( errno == EINTR )
			{
				continue;
			}
			else if ( errno == EBADF ) /* possibly closed socket */
			{
				mpc_common_nodebug( "Socket %d not valid anymore !", fd );
				res = -1;
				break;
			}
			else
			{
				mpc_common_debug( "WRITE %p %lu/%lu FAIL\n", buf, count );
				perror( "mpc_common_io_safe_write" );
				res = -1;
				break;
			}
		}

		/* update size counter */
		nb_total_sent_bytes += tmp;
	};

	return res;
}

ssize_t mpc_common_iovec_safe_write(int fd,
				    struct iovec *iov,
				    size_t iovcnt,
				    size_t length)
{
	/* vars */
	ssize_t sent = 0, cur_sent;

	/* loop until read all */
	do {
		struct msghdr msg = {
			.msg_iov = iov,
			.msg_iovlen = iovcnt
		};
		cur_sent = sendmsg(fd, &msg, MSG_NOSIGNAL);

		if (cur_sent < 0) {
			/* on interuption continue to re-read */
			if (errno == EINTR) {
				continue;
			} else if (errno == EBADF) /* possibly closed socket */ {
				mpc_common_nodebug( "Socket %d not valid anymore !", fd );
				sent = -1;
				break;
			} else {
				mpc_common_debug( "WRITE %p %lu/%lu FAIL\n", iov, sent, length);
				perror( "mpc_common_io_safe_write" );
				sent = -1;
				break;
			}
		}
		sent += cur_sent;
	} while ((size_t)sent < length);

	return sent;
}

/**********************************************************************/
/*No alloc IO                                                         */
/**********************************************************************/

int mpc_common_io_noalloc_vsnprintf ( char *s, size_t n, const char *format, va_list ap )
{
	int res;
#ifdef HAVE_VSNPRINTF
	res = vsnprintf ( s, n, format, ap );
#else
	res = vsprintf ( s, format, ap );
#endif
	return res;
}

int mpc_common_io_noalloc_snprintf ( char *restrict s, size_t n, const char *restrict format, ... )
{
	va_list ap;
	int res;
	va_start ( ap, format );
	res = mpc_common_io_noalloc_vsnprintf ( s, n, format, ap );
	va_end ( ap );
	return res;
}

size_t mpc_common_io_noalloc_fwrite ( const void *ptr, size_t size, size_t nmemb,
                                      FILE *stream )
{
	int fd;
	size_t tmp;

	if ( stream == stderr )
	{
		fd = 2;
	}
	else if ( stream == stdout )
	{
		fd = 1;
	}
	else
	{
		fd = fileno( stream );

		if ( fd < 0 )
		{
			mpc_common_debug_error ( "Unknown file descriptor" );
			abort ();
		}
	}

	tmp = write ( fd, ptr, size * nmemb );
	return tmp;
}

int mpc_common_io_noalloc_fprintf ( FILE *stream, const char *format, ... )
{
        int ret;
	va_list ap;
	char tmp[MPC_COMMON_MAX_NOALLOC_IO_SIZE];

	va_start ( ap, format );
	mpc_common_io_noalloc_vsnprintf ( tmp, MPC_COMMON_MAX_NOALLOC_IO_SIZE, format, ap );
	va_end ( ap );
	ret = mpc_common_io_noalloc_fwrite ( tmp, 1, strlen ( tmp ), stream );

        return ret;
}

int mpc_common_io_noalloc_vfprintf ( FILE *stream, const char *format, va_list ap )
{
        int ret;
	char tmp[MPC_COMMON_MAX_NOALLOC_IO_SIZE];

	mpc_common_io_noalloc_vsnprintf ( tmp, MPC_COMMON_MAX_NOALLOC_IO_SIZE, format, ap );
	ret = mpc_common_io_noalloc_fwrite ( tmp, 1, strlen ( tmp ), stream );

        return ret;
}

int mpc_common_io_noalloc_printf ( const char *format, ... )
{
        int ret;

	va_list ap;
	va_start ( ap, format );
	ret = mpc_common_io_noalloc_vfprintf ( stdout, format, ap );
	va_end ( ap );

        return ret;
}

/*****************
 * BLOCK SIGNALS *
 *****************/


void mpc_common_helper_ignore_sigpipe()
{
	struct sigaction act ;

	if ( sigaction( SIGPIPE, ( struct sigaction * )NULL, &act ) == -1 )
	{
		return;
	}

	if ( act.sa_handler == SIG_DFL )
	{
		act.sa_handler = SIG_IGN ;

		if ( sigaction( SIGPIPE, &act, ( struct sigaction * )NULL ) == -1 )
		{
			return;
		}
	}

	return ;
}


/****************
 * MEMORY STATS *
 ****************/


#if defined(__linux__)

size_t mpc_common_helper_memory_in_use( void )
{
	size_t mem_used = 0;
	FILE *f = fopen( "/proc/self/stat", "r" );

	if ( f )
	{
		// See proc(1), category 'stat'
		int z_pid;
		char z_comm[4096];
		char z_state;
		int z_ppid;
		int z_pgrp;
		int z_session;
		int z_tty_nr;
		int z_tpgid;
		unsigned long z_flags;
		unsigned long z_minflt;
		unsigned long z_cminflt;
		unsigned long z_majflt;
		unsigned long z_cmajflt;
		unsigned long z_utime;
		unsigned long z_stime;
		long z_cutime;
		long z_cstime;
		long z_priority;
		long z_nice;
		long z_zero;
		long z_itrealvalue;
		long z_starttime;
		unsigned long z_vsize;
		long z_rss;
		assume( fscanf( f, "%d %s %c %d %d %d %d %d",
		                &z_pid, z_comm, &z_state, &z_ppid, &z_pgrp, &z_session, &z_tty_nr, &z_tpgid ) == 8 );
		assume( fscanf( f, "%lu %lu %lu %lu %lu %lu %lu",
		                &z_flags, &z_minflt, &z_cminflt, &z_majflt, &z_cmajflt, &z_utime, &z_stime ) == 7 );
		assume( fscanf( f, "%ld %ld %ld %ld %ld %ld %ld %lu %ld",
		                &z_cutime, &z_cstime, &z_priority, &z_nice, &z_zero, &z_itrealvalue, &z_starttime, &z_vsize, &z_rss ) == 9 );
		int pz =  getpagesize();
		mem_used = ( size_t )( z_rss * pz );
		fclose( f );
	}

	return mem_used ;
}
#else

size_t mpc_common_helper_memory_in_use( void )
{
	return 0;
}

#endif


/**********************
 * CMD LINE ARGUMENTS *
 **********************/

char * mpc_common_helper_command_line_pretty(int json, char * buff, int len)
{
	char ** cmd = mpc_common_helper_command_line();

	if(!cmd)
	{
		return NULL;
	}

	if(json)
	{
		buff[0] = '[';
		buff[1] = '\0';
	}
	else
	{
		buff[0] = '\0';
	}

	int cnt = 0;

	int buffer_limit = 0;

	while(cmd[cnt])
	{
		char tmp[512];

		char * sep = " ";

		if(json)
		{
			sep = (cnt>0)?(", "):("");
		}

		buffer_limit += strlen(buff);
		if(buffer_limit >= len) break; // if cmd lind arguments len exceeds the buffer len then stop writing. This is not bad since this truncated line will be shown on the helper.

		snprintf(tmp, 512, "%s%s", sep, cmd[cnt]);
		strncat(buff, tmp, len - strlen(buff) - 1);
		cnt++;
	}

	if(json)
	{
		strncat(buff, "]", len);
	}

	return buff;
}

char ** mpc_common_helper_command_line(void)
{

	FILE * cmdline = fopen("/proc/self/cmdline", "r");

	if(!cmdline)
	{
		return NULL;
	}

	char* buff = sctk_malloc(1024 * 1024);

	assume(buff != NULL);

	ssize_t size = fread(buff, sizeof(char), 1024*1024, cmdline);

	if(size < 0)
	{
		sctk_free(buff);
		return NULL;
	}

	/* Now count the number of \0 */
	int cnt = 0;
	ssize_t i;
	for( i = 0 ; i < size ; i++)
	{
		if(buff[i] == '\0')
		{
			cnt++;
		}
	}

	char ** ret = sctk_malloc( (cnt + 1) * sizeof(char *));
	assume(ret != NULL);

	for(i = 0 ; i < cnt; i++)
	{
		ret[i] = NULL;
	}

	cnt = 0;

	for(i = 0; i < size; i++)
	{


		if(buff[i] == '\0')
		{
			cnt++;
		}
		else if(ret[cnt] == NULL)
		{
			ret[cnt] = &buff[i];
		}

	}

	ret[cnt] = NULL;

	return ret;
}

void mpc_common_helper_command_line_free(char **cmd)
{
	if(!cmd)
	{
		return;
	}

	sctk_free(cmd[0]);
	sctk_free(cmd);
}



/**************************************
 * GETADDRINFO WITH DEVICE PREFERENCE *
 **************************************/


int mpc_common_address_in_range(in_addr_t network_addr, in_addr_t network_mask, in_addr_t candidate)
{
    if ((network_addr & network_mask) == (candidate & network_mask))
    {
        return 1;
    }

    return 0;
}

int mpc_common_getaddr_interface(struct sockaddr_in *addr, char * ifname, int ifname_len)
{
    struct ifaddrs* ifaddr;

    if( getifaddrs(&ifaddr) < 0 )
    {
        return -1;
    }

    int ret = -1;

    struct ifaddrs* ifa = NULL;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr)
        {
            if (AF_INET == ifa->ifa_addr->sa_family)
            {
                struct sockaddr_in* inaddr = (struct sockaddr_in*)ifa->ifa_addr;
                struct sockaddr_in* netmaskaddr = (struct sockaddr_in*)ifa->ifa_netmask;

#if 0
                char ip[INET6_ADDRSTRLEN];
                inet_ntop( AF_INET, &inaddr->sin_addr, ip, sizeof( ip ) );
			    printf( "IFACE IP : %s\n", ip );
                inet_ntop( AF_INET, &netmaskaddr->sin_addr, ip, sizeof( ip ) );
			    printf( "IFACE MASK : %s\n", ip );
#endif

                if(mpc_common_address_in_range(inaddr->sin_addr.s_addr,
                                               netmaskaddr->sin_addr.s_addr,
                                               addr->sin_addr.s_addr))
                {
                    if (ifa->ifa_name)
                    {
                        //printf("TESTING %s\n", ifa->ifa_name);
                        snprintf(ifname, ifname_len, "%s", ifa->ifa_name);
                        ret = 0;
                        break;
                    }
                }

            }
        }
    }

    freeifaddrs(ifaddr);

    return ret;
}


int mpc_common_getsocket_interface(int socket, char * ifname, int ifname_len)
{
    struct sockaddr_in addr;

    socklen_t addr_len = sizeof (addr);
    if( getsockname(socket, (struct sockaddr*)&addr, &addr_len) < 0)
    {
        return -1;
    }

    return mpc_common_getaddr_interface(&addr, ifname, ifname_len);
}

static inline int __count_addrinfo(struct addrinfo * info)
{
    int ret = 0;

    while(info)
    {
        ret++;
        info = info->ai_next;
    }

    return ret;
}

struct addrinfo * __linearize_addrinfo(struct addrinfo * info)
{
    int res_count = __count_addrinfo(info);
    struct addrinfo * reorder_buff = sctk_malloc(sizeof( struct addrinfo ) * (res_count + 1) );

    if(!reorder_buff)
    {
        perror("malloc");
        return NULL;
    }

    int cnt = 0;

	struct addrinfo * tmp = info;

    while(tmp)
    {
        memcpy(reorder_buff + cnt, tmp, sizeof(struct addrinfo));
        tmp = tmp->ai_next;
        cnt++;
    }

	/* Last cell is the original pointer */
	reorder_buff[res_count].ai_next = info;

    return reorder_buff;
}

void __addr_info_swap(struct addrinfo * from, struct addrinfo * to)
{
    struct addrinfo tmp;
    memcpy(&tmp, to, sizeof(struct addrinfo));
    memcpy(to, from, sizeof(struct addrinfo));
    memcpy(from, &tmp, sizeof(struct addrinfo));
}

int mpc_common_getaddrinfo(const char *node, const char *service,
                           const struct addrinfo *hints,
                           struct addrinfo **res,
                           const char *preferred_device)
{
    struct addrinfo *local_res = NULL;

	int ret = getaddrinfo( node, service, hints, &local_res );

    if(ret < 0)
    {
        return ret;
    }

    /* We now want to reorder according to prefered interface name
       but to do so linearize the list */

    int res_count = __count_addrinfo(local_res);
    struct addrinfo * reorder_buff = __linearize_addrinfo(local_res);

    if(!strlen(preferred_device))
    {
        *res = reorder_buff;
		goto MPC_COMMON_AIDDR_DONE;
    }

    /* First check if there is an exact match */

    int did_match_exactly = 0;

    int i;

    for( i = 0 ; i < res_count ; i++)
    {
        char iface_name[128];
        mpc_common_getaddr_interface((struct sockaddr_in *)reorder_buff[i].ai_addr, iface_name, 128);
        if(!strcmp(iface_name, preferred_device))
        {
            /* This device should be first */
            if(i!=0)
            {
                __addr_info_swap(&reorder_buff[i], &reorder_buff[0]);
                did_match_exactly = 1;
            }
        }
    }

    /* Did we match exactly if not try to match partially */
    if(!did_match_exactly)
    {
        int cnt = 0;
        for( i = 0 ; i < res_count ; i++)
        {
            char iface_name[128];
            mpc_common_getaddr_interface((struct sockaddr_in *)reorder_buff[i].ai_addr,
                                         iface_name,
                                         128);
            if(strstr(iface_name, preferred_device))
            {
                /* This device should be among the firsts */
                if(i!=cnt)
                {
                    __addr_info_swap(&reorder_buff[i], &reorder_buff[cnt]);
                    cnt++;
                }
            }
        }
    }

MPC_COMMON_AIDDR_DONE:

    /* Last step is to rebuild a new list */
    for( i = 0 ; i < res_count - 1 ; i++)
    {
        reorder_buff[i].ai_next = &reorder_buff[i + 1];
    }

    reorder_buff[res_count - 1].ai_next = NULL;

    *res = reorder_buff;

    return 0;
}

static inline void __print_ipv4(struct sockaddr_in * inaddr, char * ip, int iplen)
{
    inet_ntop( AF_INET, &inaddr->sin_addr, ip, iplen );
}


int mpc_common_resolve_local_ip_for_iface(char * ip, int iplen, char *preferred_device)
{

    if(!ip || !iplen)
    {
        return -1;
    }

    assume(INET6_ADDRSTRLEN <= iplen);

    ip[0] = '\0';

    struct ifaddrs* ifaddr;

    if( getifaddrs(&ifaddr) < 0 )
    {
        return -1;
    }

    struct ifaddrs* ifa = NULL;

    /* First pass Exact Match */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_name)
        {
            if (AF_INET == ifa->ifa_addr->sa_family)
            {
    		struct sockaddr_in* inaddr = (struct sockaddr_in*)ifa->ifa_addr;
              	/* Exact Match */
		if(!strcmp(ifa->ifa_name, preferred_device) )
		{
			__print_ipv4(inaddr, ip, iplen);
			goto LOCADDRDONE;
		}
            }
        }
    }

    /* Second Pass Partial Match */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_name)
        {
            if (AF_INET == ifa->ifa_addr->sa_family)
            {
    		struct sockaddr_in* inaddr = (struct sockaddr_in*)ifa->ifa_addr;
              	/* Exact Match */
		if(strstr(ifa->ifa_name, preferred_device) )
		{
			__print_ipv4(inaddr, ip, iplen);
			goto LOCADDRDONE;
		}
            }
        }
    }


    /* Last Pass First not localhost */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_name)
        {
            if(!strcmp(ifa->ifa_name, "lo"))
	    {
		    continue;
	    }

            if (AF_INET == ifa->ifa_addr->sa_family)
            {
    		struct sockaddr_in* inaddr = (struct sockaddr_in*)ifa->ifa_addr;
		__print_ipv4(inaddr, ip, iplen);

		/* We really dont want to answer localhost ! */
		if(!strstr(ip, "127.0."))
		{
			goto LOCADDRDONE;
		}
            }
        }
    }

    if(strlen(ip) == 0 )
    {
	return -1;
    }


LOCADDRDONE:
    freeifaddrs(ifaddr);

    return 0;
}

void mpc_common_freeaddrinfo(struct addrinfo *res)
{
    int res_count = __count_addrinfo(res);

	/* Last cell holds the addrinfo addr */
    freeaddrinfo(res[res_count].ai_next);
	sctk_free(res);
}
