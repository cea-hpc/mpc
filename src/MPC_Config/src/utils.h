#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

/*********
 * COLORS *
 **********/

#define MPC_CONF_COLOR    1

#ifdef MPC_CONF_COLOR

#define BOLD(a)       "\e[1m"a "\e[0m"

#define RED(a)        "\e[31m"a "\e[0m"
#define GREEN(a)      "\e[32m"a "\e[0m"
#define BLUE(a)       "\e[34m"a "\e[0m"
#define MAGENTA(a)    "\e[35m"a "\e[0m"
#define CYAN(a)       "\e[36m"a "\e[0m"


#else

#define BOLD(a)       a

#define RED(a)        a
#define GREEN(a)      a
#define BLUE(a)       a
#define MAGENTA(a)    a
#define CYAN(a)       a

#endif

/**********************
 * OUTPUT INDENTATION *
 **********************/

#define MPC_CONF_DEFAULT_INDENT    4

char *_utils_gen_indent(int count, char *buf, int len);
char *_utils_gen_spaces(int count, char *buf, int len);

/******************
 * STRING HELPERS *
 ******************/

void _utils_lower_string(char *string);

void _utils_upper_string(char *string);

char * _utils_get_extension(char * path);

char * _utils_trim(char * path);

char * _utils_split_next_newline(char * string);

int _utils_strcasestarts_with(char * string, char * start);

int _utils_string_check(char * string, int (*filter)(int c));

/******************
 * CONFIG HELPERS *
 ******************/

int _utils_conf_can_do_color(FILE *fd);

int _utils_verbose_output(int verbosity, char * format, ... );

/****************
 * FILE HELPERS *
 ****************/

int _utils_is_file_or_dir(char * path);

int _utils_is_dir(char * path);

int _utils_is_file(char * path);

ssize_t _util_file_size(char * path);

char * _utils_read_whole_file(char * path, size_t * file_size);

#endif /* UTILS_H */
