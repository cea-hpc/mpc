#include "mpc_conf_types.h"

#include "mpc_conf.h"
#include "utils.h"

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

const char *mpc_conf_type_name(mpc_conf_type_t type)
{
	assert(type <MPC_CONF_TYPE_NONE);

	static const char *conf_type_names[MPC_CONF_TYPE_NONE] =
	{
		"INT",
		"LONG_INT",
		"DOUBLE",
		"STRING",
		"FUNCTION",
		"CONFIG",
		"BOOL"
	};


	return conf_type_names[type];
}

int mpc_conf_type_is_string(mpc_conf_type_t type)
{
	switch(type)
	{
		case MPC_CONF_INT:
		case MPC_CONF_LONG_INT:
		case MPC_CONF_DOUBLE:
		case MPC_CONF_TYPE:
		case MPC_CONF_TYPE_NONE:
		case MPC_CONF_BOOL:
			return 0;

		case MPC_CONF_FUNC:
		case MPC_CONF_STRING:
			return 1;
	}

	return 0;
}

int mpc_conf_type_print_value(mpc_conf_type_t type, char *buf, int len, void *ptr, int add_color)
{
	buf[0] = '\0';

	switch(type)
	{
		case MPC_CONF_INT:
			if(add_color)
			{
				return snprintf(buf, len, RED("%d"), *( (int *)ptr) );
			}
			else
			{
				return snprintf(buf, len, "%d", *( (int *)ptr) );
			}

		case MPC_CONF_LONG_INT:
			if(add_color)
			{
				return snprintf(buf, len, GREEN("%ld"), *( (long int *)ptr) );
			}
			else
			{
				return snprintf(buf, len, "%ld", *( (long int *)ptr) );
			}

		case MPC_CONF_DOUBLE:
			if(add_color)
			{
				return snprintf(buf, len, BLUE("%f"), *( (double *)ptr) );
			}
			else
			{
				return snprintf(buf, len, "%f", *( (double *)ptr) );
			}

		case MPC_CONF_FUNC:
		case MPC_CONF_STRING:
		{

			if(add_color)
			{
				return snprintf(buf, len, MAGENTA("%s"), (char*)ptr);
			}
			else
			{
				return snprintf(buf, len, "%s", (char*)ptr);
			}
		}
		case MPC_CONF_BOOL:
			if(add_color)
			{
				return snprintf(buf, len, BLUE("%s"), *( (int *)ptr) ? "true" : "false");
			}
			else
			{
				return snprintf(buf, len, "%s", *( (int *)ptr) ? "true" : "false");
			}

		case MPC_CONF_TYPE:
		{
			/* This is a placeholder this should be walked with mpc_conf_config_type_elem_printXX */
			mpc_conf_config_type_t *type = (mpc_conf_config_type_t *)ptr;
			if(add_color)
			{
				return snprintf(buf, len, BOLD(RED("<%s/>") ), type->name);
			}
			else
			{
				return snprintf(buf, len, "<%s/>", type->name);
			}
		}

		default:
			return snprintf(buf, len, "UNKNOWN ??");
	}
}

ssize_t mpc_conf_type_size(mpc_conf_type_t type)
{
	switch(type)
	{
		case MPC_CONF_INT:
		case MPC_CONF_BOOL:
			return sizeof(int);

		case MPC_CONF_LONG_INT:
			return sizeof(long);

		case MPC_CONF_DOUBLE:
			return sizeof(double);

		case MPC_CONF_FUNC:
		case MPC_CONF_STRING:
			return sizeof(char)*MPC_CONF_STRING_SIZE;
		case MPC_CONF_TYPE:
			return sizeof(void *);
		case MPC_CONF_TYPE_NONE:
			return -1;
	}

	return -1;
}

int mpc_conf_type_set_value(mpc_conf_type_t type, void **dest, void *from)
{
	switch(type)
	{
		case MPC_CONF_INT:
		case MPC_CONF_BOOL:
		case MPC_CONF_LONG_INT:
		case MPC_CONF_DOUBLE:
		case MPC_CONF_FUNC:
		case MPC_CONF_STRING:
			memcpy(*dest, from, mpc_conf_type_size(type) );
			break;
		case MPC_CONF_TYPE:
			mpc_conf_config_type_release( (mpc_conf_config_type_t **)dest);
			*dest = from;
			break;
		case MPC_CONF_TYPE_NONE:
			return 1;
	}

	return 0;
}

static inline int __parse_long_int(char *buff, long *val)
{
	char *begin = buff;
	char *end   = NULL;

	errno = 0;
	*val  = strtol(begin, &end, 10);

	if( (errno == ERANGE &&
	     (*val == LONG_MAX || *val == LONG_MIN) ) ||
	    (errno != 0 && *val == 0) )
	{
		perror("strtol");
		return 1;
	}

	if(end == begin)
	{
		return 1;
	}

	return 0;
}

static inline int __parse_double(char *buff, double *val)
{
	char *begin = buff;
	char *end   = NULL;

	errno = 0;

	*val = strtold(begin, &end);

	if( (errno == ERANGE) || (errno != 0 && *val == 0.0) )
	{
		perror("strtold");
		return 1;
	}

	if(end == begin)
	{
		return 1;
	}

	return 0;
}

int __mpc_conf_type_parse_from_string(mpc_conf_type_t type, void *dest, char *from)
{
	switch(type)
	{
		case MPC_CONF_BOOL:
			/* Bool as string */
			if(!_utils_string_check(from, isdigit))
			{
				if(!strcasecmp(from, "true"))
				{
					*((int*)dest) = 1;
					return 0;
				}
				else if(!strcasecmp(from, "false"))
				{
					*((int*)dest) = 0;
					return 0;
				}
				else
				{
					_utils_verbose_output(0,"could not parse bool expected {true,false} or {0,1} got '%s'\n", from);
					return 1;
				}
			}
		/* FALLTHRU */
		/* Could also be an int so we fallthrough */
		case MPC_CONF_INT:
		{
			long tmp = 0;
			int  ret = __parse_long_int(from, &tmp);
			if(ret)
			{
				return ret;
			}

			if(INT_MAX <= tmp)
			{
				_utils_verbose_output(0,"value %ld too large for an integer\n", tmp);
				return 1;
			}

			*( (int *)dest) = (int)tmp;

			return 0;
		}

		case MPC_CONF_LONG_INT:
		{
			return __parse_long_int(from, (long *)dest);
		}

		case MPC_CONF_DOUBLE:
		{
			return __parse_double(from, (double *)dest);
		}

		case MPC_CONF_FUNC:
		case MPC_CONF_STRING:
			snprintf(dest, MPC_CONF_STRING_SIZE, "%s", (char *)from);
			return 0;
		case MPC_CONF_TYPE:
		case MPC_CONF_TYPE_NONE:
			return 1;
	}

	return 1;
}

int mpc_conf_type_set_value_from_string(mpc_conf_type_t type, void **dest, char *from)
{
	/* String can only be assigned to actual values */
	if(type ==MPC_CONF_TYPE)
	{
		_utils_verbose_output(0,"cannot assign string value '%s' toMPC_CONF_TYPE\n", from);
		return 1;
	}


	/* If type has a size we need to parse directly */
	ssize_t tsize = mpc_conf_type_size(type);

	if(0 <= tsize)
	{
		/* Now we need to parse */
		if(__mpc_conf_type_parse_from_string(type, *dest, from) != 0)
		{
			_utils_verbose_output(0,"Failed parsing %s from string '%s'\n", mpc_conf_type_name(type), from);
			return 1;
		}
	
		return 0;
	}
	else
	{
		/* If we are here no parsing needed :
		 *      - Pointer type
		 */
		return mpc_conf_type_set_value(type, dest, (void *)from);
	}

	return 1;
}

mpc_conf_type_t mpc_conf_type_infer_from_string(char *string)
{
	/* Only numbers provide largest int */
	if(_utils_string_check(string, isdigit))
	{
		return MPC_CONF_LONG_INT;
	}

	/* Could it be a bool ? */
	if(!strcasecmp(string, "true") || !strcasecmp(string, "false"))
	{
		return MPC_CONF_BOOL;
	}

	/* Can I parse it as a double ? */
	double test;
	if( __mpc_conf_type_parse_from_string(MPC_CONF_DOUBLE, &test, string) == 0 )
	{
		return MPC_CONF_DOUBLE;
	}

	/* Otherwise it seems to be a string */
	return MPC_CONF_STRING;
}
