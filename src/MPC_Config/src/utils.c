#include "utils.h"
#include "mpc_conf.h"

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <stdlib.h>

/**********************
* OUTPUT INDENTATION *
**********************/

static inline char *__gen_char(int count, char *buf, char pad, int len)
{
	int i = 0;

	while( (i < count) && (i < (len - 1) ) )
	{
		buf[i] = pad;
		i++;
	}

	buf[i] = '\0';

	return buf;
}

char *_utils_gen_indent(int count, char *buf, int len)
{
	int indent_count = MPC_CONF_DEFAULT_INDENT;

	mpc_conf_self_config_t *conf = mpc_conf_self_config_get();

	if(conf)
	{
		indent_count = conf->indent_count;
	}

	return __gen_char(count * indent_count, buf, ' ', len);
}

char *_utils_gen_spaces(int count, char *buf, int len)
{
	return __gen_char(count, buf, ' ', len);
}

/******************
* STRING HELPERS *
******************/

void _utils_lower_string(char *string)
{
	int i;
	int len = strlen(string);

	for(i = 0; i < len; i++)
	{
		string[i] = tolower(string[i]);
	}
}

void _utils_upper_string(char *string)
{
	int i;
	int len = strlen(string);

	for(i = 0; i < len; i++)
	{
		string[i] = toupper(string[i]);
	}
}



char * _utils_get_extension(char * path)
{
	char *point = strrchr(path, '.');

	if(!point)
	{
		return NULL;
	}

	char * ret = point + 1;

	if(!strlen(ret))
	{
		return NULL;
	}

	return ret;
}

char * _utils_trim(char * path)
{
	/* From start */
	char * ret = path;

	while( (*ret == ' ' || *ret == '\t') && (*ret != '\0') )
	{
		ret++;
	}

	/* From end */
	int len = strlen(ret);

	if(len == 0)
	{
		return ret;
	}

	int off = len - 1;

	while( ( ret[off] == ' ' || ret[off] == '\t' || ret[off] == '\n' ) && (0 <= off) )
	{
		ret[off] = '\0';
		off--;
	}	

	return ret;
}


char * _utils_split_next_newline(char * string)
{
	char * nl = strchr(string, '\n');

	if(nl)
	{
		*nl = '\0';
	}

	return nl;
}

int _utils_strcasestarts_with(char * string, char * start)
{
	int lens = strlen(string);
	int lenc = strlen(start);

	int i = 0;

	while( (i < lens) && (i < lenc) )
	{
		if(tolower(string[i]) != tolower(start[i]))
		{
			return 0;
		}

		i++;
	}

	return 1;
}

int _utils_string_check(char * string, int (*filter)(int c))
{
	int i;
	int len = strlen(string);

	for( i = 0 ; i < len; i++)
	{
		if(!(filter)(string[i]))
		{
			return 0;
		}
	}

	return 1;
}

/******************
* CONFIG HELPERS *
******************/

int _utils_conf_can_do_color(FILE *fd)
{
	mpc_conf_self_config_t *conf = mpc_conf_self_config_get();

	int color_in_conf = 0;

	if(conf)
	{
		color_in_conf = conf->color_enabled;
	}

	static FILE *__prev_fd     = NULL;
	static int   prev_was_atty = 0;

	if(__prev_fd != fd)
	{
		__prev_fd     = fd;
		prev_was_atty = isatty(fileno(fd) );
	}

	return prev_was_atty && color_in_conf;
}

int _utils_verbose_output(int verbosity, char *format, ...)
{
	mpc_conf_self_config_t *conf = mpc_conf_self_config_get();

	if(!conf)
	{
		return 0;
	}

	if(conf->verbose < verbosity)
	{
		return 0;
	}


    if(_utils_conf_can_do_color(stderr))
    {
        fprintf(stderr, RED("CONF[%d]: "), verbosity);
    }
    else
    {
        fprintf(stderr, "CONF[%d]: ", verbosity);
    }


	va_list ap;

	va_start(ap, format);

	int ret = vfprintf(stderr, format, ap);

	va_end(ap);

	return ret;
}

/****************
 * FILE HELPERS *
 ****************/


static inline int __get_ftype(char * path)
{
    struct stat st;
    if(stat(path, &st) != 0)
    {
		_utils_verbose_output(3, "stat error for %s\n", path);

        //perror("stat");
        return -1;
    }

    return st.st_mode & S_IFMT;
}


int _utils_is_file_or_dir(char * path)
{
	int ftype = __get_ftype( path);

	if(ftype < 0 )
	{
		return 0;
	}

    if((ftype != S_IFDIR) && (ftype != S_IFREG))
    {
		printf("KK\n");
        /* We only accept regular files and directories */
        return 0;
    }

	return 1;
}

int _utils_is_dir(char * path)
{
	int ftype = __get_ftype( path);

	if(ftype < 0 )
	{
		return 0;
	}

	if(ftype == S_IFDIR)
	{
		return 1;
	}

	return 0;
}

int _utils_is_file(char * path)
{
	int ftype = __get_ftype( path);

	if(ftype < 0 )
	{
		return 0;
	}

	if(ftype == S_IFREG)
	{
		return 1;
	}

	return 0;	
}

ssize_t _util_file_size(char * path)
{
    struct stat st;
    if(stat(path, &st) != 0)
    {
		_utils_verbose_output(3, "stat error for %s\n", path);

        //perror("stat");
        return -1;
    }

	return st.st_size;	
}


#define MAX_READ_SIZE (100 * 1024 * 1024)

char * _utils_read_whole_file(char * path, size_t * file_size)
{
	if(!_utils_is_file(path))
	{
		return NULL;
	}

	if(file_size)
	{
		*file_size = 0;
	}

	ssize_t size = _util_file_size(path);

	if(size < 0)
	{
		return NULL;
	}

	if( MAX_READ_SIZE <= size )
	{
		_utils_verbose_output(0, "cannot read %s larger than %d MBytes\n", path, (MAX_READ_SIZE)/(1024*1024));
		return NULL;
	}

	FILE * file = fopen(path, "r");

	if(!file)
	{
		perror("fopen");
		return NULL;
	}


	char * ret = malloc(( size + 1 ) * sizeof(char));

	if(!ret)
	{
		perror("malloc");
		return NULL;
	}


	size_t fret = fread(ret, sizeof(char), size, file);

	if(fret != (size_t)size)
	{
		_utils_verbose_output(0, "fread was truncated for %s (expected %ld bytes)\n", path, size);
		free(ret);
		return NULL;

	}

	if(file_size)
	{
		*file_size = size;
	}

	ret[size] = '\0';

	fclose(file);

	return ret;
}