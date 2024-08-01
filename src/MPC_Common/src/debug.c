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
#define _GNU_SOURCE
#include <ctype.h>
#include <execinfo.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <mpc_config.h>

#include <mpc_common_debug.h>
#include <mpc_common_flags.h>
#include <mpc_common_helper.h>
#include <mpc_common_rank.h>
#include <mpc_common_spinlock.h>


#ifdef MPC_Thread_db
	#include <sctk_debugger.h>
#endif

#define MPC_MODULE "Common/Debug"

#define SMALL_BUFFER_SIZE ( 4 * 1024 )
#define DEBUG_INFO_SIZE ( 64 )

/**********************************************************************/
/*Threads support                                                     */
/**********************************************************************/

static inline char *__debug_print_info( char *buffer )
{
#ifdef MPC_ENABLE_SHELL_COLORS
		if(mpc_common_debug_is_stderr_tty() && mpc_common_get_flags()->colors){
			snprintf( buffer,
					DEBUG_INFO_SIZE,
					MPC_COLOR_GREEN( [ )
										MPC_COLOR_RED(R%4d)
										MPC_COLOR_BLUE( P%4dN%4d)
										MPC_COLOR_GREEN( ] ),
					mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );
		}
		else{
			snprintf( buffer,
					DEBUG_INFO_SIZE,
					"[R%4dP%4dN%4d]",
					mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );

		}
#else
		snprintf( buffer,
		          DEBUG_INFO_SIZE,
		          "[R%4dP%4dN%4d]",
		          mpc_common_get_task_rank(), mpc_common_get_process_rank(), mpc_common_get_node_rank() );
#endif

	return buffer;
}
/* NOLINTEND(clang-diagnostic-unused-function) */

/**********************************************************************/
/*Abort                                                               */
/**********************************************************************/
void mpc_launch_pmi_abort();

__attribute__((__noreturn__)) void mpc_common_debug_abort( void )
{
	mpc_common_debug_error("######## Program will now abort ########");
	mpc_launch_pmi_abort();
	abort();
}

/**********************************************************************/
/*Messages                                                            */
/**********************************************************************/


const char * mpc_common_debug_get_basename(const char * path)
{
	char * ret = strrchr(path, '/');
	return ret?(ret + 1):path;
}


int MPC_check_compatibility_lib( int major, int minor, int patch, char *pre )
{
	static char errro_msg[4096];

	if ( ( major != MPC_VERSION_MAJOR ) || ( minor != MPC_VERSION_MINOR ) || (patch != MPC_VERSION_PATCH) )
	{
		sprintf( errro_msg,
		         "MPC version used for this file (%d.%d.%d%s) differs from the library used for the link (%d.%d.%d%s)\n",
		         major, minor, patch, pre, MPC_VERSION_MAJOR, MPC_VERSION_MINOR, MPC_VERSION_PATCH, MPC_VERSION_PRE );
		mpc_common_debug_warning( errro_msg );
		return 1;
	}

	return 0;
}

void MPC_printf( const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );
	mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
	                                "%s %s",
	                                __debug_print_info( debug_info ),
	                                fmt );
	mpc_common_io_noalloc_vfprintf( stderr, buff, ap );
	fflush( stderr );
	va_end( ap );
}


void mpc_common_debug_assert_print( FILE *stream, int line, const char *file,
                                 const char *func, const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];

	if ( func == NULL )
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s %s:%d : Assertion %s\n",
		                                __debug_print_info( debug_info ),
												  mpc_common_debug_get_basename(file),
												  line,
		                                fmt);
	else
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
		                                "%s [%s] %s:%d : Assertion %s\n",
		                                __debug_print_info( debug_info ),
												  func,
												  mpc_common_debug_get_basename(file),
												  line,
		                                fmt);

	va_start( ap, fmt );
	mpc_common_io_noalloc_vfprintf( stream, buff, ap );
	va_end( ap );
	mpc_common_debug_abort();
}


void mpc_common_debug_file( FILE *file, const char *fmt, ... )
{
	va_list ap;
	char debug_info[SMALL_BUFFER_SIZE];
	char buff[SMALL_BUFFER_SIZE];
	va_start( ap, fmt );
	mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
	                                "%s %s\n",
	                                __debug_print_info( debug_info ),
	                                fmt );
	vfprintf( file, buff, ap );
	va_end( ap );
	fflush( file );
}

void mpc_common_debug_abort_log( FILE *stream, int line,
                                    const char *file, const char *func,
                                    const char *fmt, ... )
{
	va_list ap;
	char buff[SMALL_BUFFER_SIZE];

#ifdef MPC_ENABLE_SHELL_COLORS
	if(mpc_common_debug_is_stderr_tty() && mpc_common_get_flags()->colors)
		mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
					"%s:%d (%s):\n"
					"-------------------------------------\n"
					MPC_COLOR_RED_BOLD("%s")"\n"
					"-------------------------------------\n"
					"\n", file, line, func?func:"??",
					fmt );
	else mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
					"%s:%d (%s):\n"
					"-------------------------------------\n"
					"%s\n"
					"-------------------------------------\n"
					"\n", file, line, func?func:"??",
					fmt );
#else
	mpc_common_io_noalloc_snprintf( buff, SMALL_BUFFER_SIZE,
					"%s:%d (%s):\n"
					"-------------------------------------\n"
					"%s\n"
					"-------------------------------------\n"
					"\n", file, line, func?func:"??",
					fmt );
#endif

	va_start( ap, fmt );
	mpc_common_io_noalloc_vfprintf( stream, buff, ap );
	va_end( ap );

	exit(1);
}

/**********************************************************************/
/*Sizes                                                               */
/**********************************************************************/
void mpc_common_debug_check_large_enough( size_t a, size_t b, char *ca, char *cb, char *file,
                         int line )
{
	if ( !( a <= b ) )
	{
		mpc_common_io_noalloc_fprintf( stderr,
		                               "Internal error !(%s <= %s) at line %d in %s\n",
		                               ca, cb, line, file );
		mpc_common_debug_abort();
	}
}
void mpc_common_debug_check_size_equal( size_t a, size_t b, char *ca, char *cb, char *file,
                            int line )
{
	if ( a != b )
	{
		mpc_common_io_noalloc_fprintf( stderr,
		                               "Internal error %s != %s at line %d in %s\n",
		                               ca, cb, line, file );
		mpc_common_debug_abort();
	}
}

int __mpcprintf(char *messagebuffer, char *modulename, char *filename, int line, char *color){
	int r;
	int task_rank = mpc_common_get_task_rank();
	#ifdef MPC_ENABLE_SHELL_COLORS
		if(mpc_common_debug_is_stderr_tty() 
			&& mpc_common_get_flags()->colors)
			(void)fprintf(stderr, 
				MPC_COLOR_GREEN([ )
				MPC_COLOR_RED(R%4d)
				MPC_COLOR_BLUE(P%4dN%4d)
				MPC_COLOR_GREEN(])
				" "
				MPC_COLOR_CYAN(%s)
				" "
				"%s%s%s\n",
				task_rank, 
				mpc_common_get_process_rank(), 
				mpc_common_get_node_rank(),
				modulename,
				color , messagebuffer, MPC_COLOR_DEFAULT_CHAR);
		else
	#else
			(void)fprintf(stderr, 
				"[ R%4d P%4dN%4d ] %s %s%s%s\n",
				task_rank, 
				mpc_common_get_process_rank(), 
				mpc_common_get_node_rank(),
				modulename,
				color , messagebuffer, MPC_COLOR_DEFAULT_CHAR);
	#endif

	return r;
}


/******************************
 * LOG FILTERING CAPABILITIES *
 ******************************/


typedef enum
{
	FILTER_NONE,
	FILTER_MODULE,
	FILTER_FUNCTION,
	FILTER_FILE,
	FILTER_COUNT
}__log_filter_type_t;

const char *  __filter_type_name(__log_filter_type_t t)
{
	const char * names[] = {
		"None",
		"Module",
		"Function",
		"File"
	};

	if(t > FILTER_COUNT)
	{
		mpc_common_debug_fatal("Overflow in filter");
	}

	return names[t];
}


#define FILTER_MAX_SIZE 512

struct __log_filter_rule_s
{
	__log_filter_type_t type;
	char match[FILTER_MAX_SIZE];
};


#define MAX_LOG_FILTER_RULES 32

static struct __log_filter_rule_s __log_filter_rules[MAX_LOG_FILTER_RULES];
static unsigned int __log_filter_rules_count = 0;


static inline void __parse_log_filter_rules(void)
{
	char * filter_str = getenv("MPC_LOG");

	if(!filter_str)
	{
		/* No filter to apply __log_filter_rules remains null*/
		return;
	}

	/* Prepare the regular expression to parse the filters */
	regex_t re_file;
	regex_t re_module;
	regex_t re_function;

	if (regcomp(&re_file, "file:(.*)", REG_ICASE | REG_EXTENDED)) {
		mpc_common_debug_error("Failed to compile re_file regular expression");
		return;
	}

	if (regcomp(&re_module, "mod:(.*)", REG_ICASE | REG_EXTENDED)) {
		mpc_common_debug_error("Failed to compile re_module regular expression");
		return;
	}

	if (regcomp(&re_function, "func:(.*)", REG_ICASE | REG_EXTENDED)) {
		mpc_common_debug_error("Failed to compile re_function regular expression");
		return;
	}

	/* Tokenize on ',' */

	char *token = NULL;
	char *save_ptr = NULL;

    // Get the first token
    token = strtok_r(filter_str, ",", &save_ptr);

    // Walk through other tokens
    while (token != NULL) {

			char buff[FILTER_MAX_SIZE];
			(void)snprintf(buff,FILTER_MAX_SIZE, "%s", token);
			char * trimed_token = mpc_common_trim(buff);


			regmatch_t match[2];

			__log_filter_type_t filter_type = FILTER_NONE;
			char * smatch = NULL;

			if(regexec(&re_file, trimed_token, 2, match, 0) == 0)
			{
				*(trimed_token + match[1].rm_eo) = '\0';
				smatch = trimed_token + match[1].rm_so;
				filter_type = FILTER_FILE;
			}else if (regexec(&re_module, trimed_token, 2, match, 0) == 0)
			{
				*(trimed_token + match[1].rm_eo) = '\0';
				smatch = trimed_token + match[1].rm_so;
				filter_type = FILTER_MODULE;
			}
			else if (regexec(&re_function, trimed_token, 2, match, 0) == 0)
			{
				*(trimed_token + match[1].rm_eo) = '\0';
				smatch = trimed_token + match[1].rm_so;
				filter_type = FILTER_FUNCTION;
			}
			else
			{
				/* No match it is a module */
				smatch = trimed_token;
				filter_type = FILTER_MODULE;
			}

			if(smatch)
			{
				if( __log_filter_rules_count == MAX_LOG_FILTER_RULES)
				{
					mpc_common_debug_fatal("It is not possible to have more than %d rules in MPC_LOG environment variable", MAX_LOG_FILTER_RULES);
				}

				mpc_common_debug_info("Filter on %s (from %s) type %s", smatch, token, __filter_type_name(filter_type));

				(void)snprintf(__log_filter_rules[__log_filter_rules_count].match, FILTER_MAX_SIZE, "%s", smatch);
				__log_filter_rules[__log_filter_rules_count].type = filter_type;

				__log_filter_rules_count++;

			}

		
			/* Move to next token */
        token = strtok_r(NULL, "-", &save_ptr);

    }
}


char* strstr_case_insensitive(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;

    for (; *haystack; ++haystack) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            const char* h = haystack + 1;
            const char* n = needle   + 1;
            while (*n && *h && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
                ++h;
                ++n;
            }
            if (!*n) return (char*)haystack;
        }
    }

    return NULL;
}

static inline int __call_is_filtered_from_env(const char * modulename, const char *funcname, const char * filename)
{

	/* No filter nothing is filtered */
	if(__log_filter_rules_count == 0)
	{
		return 0;
	}

	unsigned int i = 0;

	for( i = 0 ; i < __log_filter_rules_count ; i++)
	{
	 	struct __log_filter_rule_s * tmp = &__log_filter_rules[i];

		/* Allow print at first rule matching */
		switch (tmp->type) {
			case FILTER_FILE:
				if(strstr_case_insensitive(filename, tmp->match))
				{
					return 0;
				}
			break;
			case FILTER_MODULE:
				if(strstr_case_insensitive(modulename, tmp->match))
				{
					return 0;
				}
			break;
			case FILTER_FUNCTION:
				if(strstr_case_insensitive(funcname, tmp->match))
				{
					return 0;
				}
			break;
			case FILTER_COUNT:
			case FILTER_NONE:
				mpc_common_debug_fatal("There should be no lo rule of type FILTER_NONE");
			break;
		}


	}


	/* If no rule did match we are filtered */

	return 1;	
}


int mpc_common_debug_print(char *filename, int line, const char *funcname, char *color, mpc_common_debug_verbosity_level_t verbosity_level, char *modulename, char *string, ...){
	va_list ap;
	int r = 0;

	/* Check loglevel */
	if( mpc_common_get_flags()->verbosity < (int)verbosity_level)
	{
		return 0;
	}

	if(__call_is_filtered_from_env(modulename, funcname, filename))
	{
		/* All filters did fail no need to print*/
		return 0;
	}

	va_start (ap, string); 

	char messagebuffer[SMALL_BUFFER_SIZE];

	if(modulename[0] == 'M' && modulename[1] == 'P' && modulename[2] == 'C')
	{
		if(!strcmp("MPC_MODULE", modulename))
		{
			/* No module was set */
			modulename = "";
		}
	}

	/* Strip quotes */
	char __no_quote_module_name[128];
	char * no_quote_module_name = __no_quote_module_name;
	(void)snprintf(no_quote_module_name, 128, "%s", modulename);

	unsigned long modlen = strlen(no_quote_module_name);

	if(modlen)
	{
		if(no_quote_module_name[modlen - 1] == '"' || no_quote_module_name[modlen - 1] == '\'')
		{
			no_quote_module_name[modlen - 1] = '\0';
		}

		if(no_quote_module_name[0] == '"' || no_quote_module_name[0] == '\'')
		{
			no_quote_module_name++;
		}
	}

	r = vsnprintf(messagebuffer,(long)SMALL_BUFFER_SIZE, string, ap);


	if(strchr(messagebuffer, '\n'))
	{
		char *save_ptr = NULL;
 		char * sline = strtok_r(messagebuffer, "\n", &save_ptr);

    	while (sline != NULL) {
			if(strlen(sline))
			{
				__mpcprintf(sline, no_quote_module_name, filename, line, color);
			}
			sline = strtok_r(NULL, "\n", &save_ptr);
		}

	}
	else
	{
		__mpcprintf(messagebuffer, no_quote_module_name, filename, line, color);
	}

	va_end (ap);
	return r;
}

/**************
 * TTY STATUS *
 **************/

static int __is_stderr_tty = 0;


int mpc_common_debug_is_stderr_tty(){
	return __is_stderr_tty;
}

/***************************************
 * MPC COMMON DEBUG INITALIZATION CODE *
 ***************************************/


void mpc_common_debug_init(){
	/* Set TTY support for stderr from launcher state */
	if(getenv("IS_STDERR_TTY") )
	{
		__is_stderr_tty = 1;
	}
	else
	{ 
		__is_stderr_tty = 0;
	}


	__parse_log_filter_rules();

}
