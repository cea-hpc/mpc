#include "mpc_conf.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <pwd.h>

#include "utils.h"
#include "loader.h"

static inline void __print_xml_header(FILE *fd)
{
	fprintf(fd, "<?xml version=\"1.0\"?>\n<config>\n");
}

static inline void __print_xml_footer(FILE *fd)
{
	fprintf(fd, "</config>\n");
}

static int __mpc_conf_root_config_check_parenting(void);

/***************
 * OUTPUT KIND *
 ***************/

mpc_conf_output_type_t mpc_conf_output_type_infer_from_file(char *path)
{
	char * ext = _utils_get_extension(path);

	if(!ext)
	{
		return MPC_CONF_FORMAT_NONE;
	}

	_utils_verbose_output(3, "INFERTYPE: %s is %s\n", path, ext);

	if(!strcmp(ext, "conf"))
	{
		return MPC_CONF_FORMAT_CONF;
	}
	else if(!strcmp(ext, "ini"))
	{
		return MPC_CONF_FORMAT_INI;
	}
	else if(!strcmp(ext, "xml"))
	{
		return MPC_CONF_FORMAT_XML;
	}

	return MPC_CONF_FORMAT_NONE;
}

/***************************
* CONFIGURATION TYPE ELEM *
***************************/

static inline char * __stringtolower(char * string)
{
	int l = strlen(string);
	int i;

	for(i = 0 ; i < l ; i++)
	{
		string[i] = tolower(string[i]);
	}

	return string;
}


mpc_conf_config_type_elem_t *mpc_conf_config_type_elem_init(char *name, void *addr, mpc_conf_type_t type, char *doc, mpc_conf_enum_keyval_t * ekv, int ekv_length)
{
	_utils_verbose_output(2, "ELEM: init %s @ %p type %s // %s\n", name, addr, mpc_conf_type_name(type), doc);
	mpc_conf_config_type_elem_t *ret = malloc(sizeof(mpc_conf_config_type_elem_t) );

	assert(ret != NULL);

	ret->addr_is_to_free = 0;
	ret->is_locked       = 0;
	ret->name            = __stringtolower(strdup(name));
	ret->parent          = NULL;
  ret->ekv_length      = ekv_length;
  ret->ekv             = malloc(ekv_length * sizeof(mpc_conf_enum_keyval_t));

  memcpy(ret->ekv, ekv, ekv_length * sizeof(mpc_conf_enum_keyval_t));

	if(strchr(name, '_'))
	{
			_utils_verbose_output(0, "Underscores (_) are not allowed in elements names (see %s)\n", name);
			abort();
	}

	/* Now set to lowercase */
	_utils_lower_string(ret->name);

	/* Ensure that names do match if elem holds a type */
	if(addr && (type == MPC_CONF_TYPE))
	{
		if(strcmp(ret->name, ((mpc_conf_config_type_t*)addr)->name))
		{
			_utils_verbose_output(0, "cannot create a config element holding %s with a name %s they must be identical\n", name, ((mpc_conf_config_type_t*)addr)->name);
			abort();
		}
	}

	ret->addr = addr;

	ret->type = type;
	ret->doc  = strdup(doc);

	return ret;
}

void mpc_conf_config_type_elem_release(mpc_conf_config_type_elem_t **elem)
{
	_utils_verbose_output(3, "ELEM: release %s\n", (*elem)->name);
	free( (*elem)->name);
	free( (*elem)->doc);
  free( (*elem)->ekv);

	if( (*elem)->addr_is_to_free )
	{
		free( (*elem)->addr);
	}

	memset(*elem, 0, sizeof(mpc_conf_config_type_elem_t) );
	free(*elem);
	*elem = NULL;
}


mpc_conf_config_type_t * mpc_conf_config_type_elem_update(mpc_conf_config_type_t * ref, mpc_conf_config_type_t * updater, int force_content)
{
	/* First check current in default to ensure that all entries are known */
    int i;

	if(0 < force_content)
	{
		for(i = 0 ; i < mpc_conf_config_type_count(updater); i++)
		{
			mpc_conf_config_type_elem_t* current_elem = mpc_conf_config_type_nth(updater, i);

			/* Now get the elem to ensure it already exists */
			mpc_conf_config_type_elem_t *default_elem = mpc_conf_config_type_get(ref, current_elem->name);

			if(!default_elem)
			{
				mpc_conf_config_type_print(updater, MPC_CONF_FORMAT_XML);
				_utils_verbose_output(0,"Default definitions do not contain '%s' elements", current_elem->name);
				abort();
			}
		}
	}

	/* Now check default in current to push missing elements from default */
	for(i = 0 ; i < mpc_conf_config_type_count(ref); i++)
    {
        mpc_conf_config_type_elem_t* default_elem = mpc_conf_config_type_nth(ref, i);
        /* Now get the elem to ensure it already exists */
        mpc_conf_config_type_elem_t *existing_elem = mpc_conf_config_type_get(updater, default_elem->name);

        if(existing_elem)
        {

			if(existing_elem->type == MPC_CONF_TYPE)
			{
				mpc_conf_config_type_elem_update(mpc_conf_config_type_elem_get_inner(default_elem),
				                                 mpc_conf_config_type_elem_get_inner(existing_elem),
												 --force_content);
			}
			else
			{
				/* Here we need to update the default elem */
            	mpc_conf_config_type_elem_set_from_elem(default_elem, existing_elem);
			}
		}
    }

	/* Ensure new elemens are also inserted only when force content is <= 0 */
	if(force_content <= 0)
	{
		/* We need a tmp as we cannot pop while iterating */
		mpc_conf_config_type_t *tmp = mpc_conf_config_type_init("tmp", NULL);

		for(i = 0 ; i < mpc_conf_config_type_count(updater); i++)
		{
			mpc_conf_config_type_elem_t* current_elem = mpc_conf_config_type_nth(updater, i);

			/* Now get the elem to ensure it already exists */
			mpc_conf_config_type_elem_t *default_elem = mpc_conf_config_type_get(ref, current_elem->name);

			if(!default_elem)
			{
				/* Here we save the missing element */
				mpc_config_type_append_elem(tmp, current_elem);
			}
		}

		/* Now we pop it from the updater and append it to ref */
		for(i = 0 ; i < mpc_conf_config_type_count(tmp); i++)
		{
			mpc_conf_config_type_elem_t* current_elem = mpc_conf_config_type_nth(tmp, i);
			mpc_conf_config_type_elem_t * updater_elem = mpc_conf_config_type_get(updater, current_elem->name);

			if(updater_elem)
			{
				mpc_config_type_pop_elem(updater, updater_elem);
			}

			mpc_config_type_append_elem(ref, current_elem);
		}

		/* Eventually we empty TMP by popping all */
		while(mpc_conf_config_type_count(tmp) != 0)
		{
			mpc_conf_config_type_elem_t* current_elem = mpc_conf_config_type_nth(tmp, 0);
			mpc_config_type_pop_elem(tmp, current_elem);
		}

		/* And release tmp */
		mpc_conf_config_type_release(&tmp);
	}

    return ref;
}



int mpc_conf_config_type_elem_set_from_elem(mpc_conf_config_type_elem_t *elem, mpc_conf_config_type_elem_t *src)
{
	_utils_verbose_output(3, "ELEM: set %s to %s\n", src->name, elem->name);

	return mpc_conf_config_type_elem_set(elem, src->type, src->addr);
}

int mpc_conf_config_type_elem_set(mpc_conf_config_type_elem_t *elem, mpc_conf_type_t type, void *ptr)
{

	if( (elem->type == MPC_CONF_INT) && (type == MPC_CONF_LONG_INT) )
	{
		/* Handle LONG to int conversion if it fits */
		long source_value = *((long*)ptr);
		int int_storage = 0;


		if(source_value <= INT_MAX)
		{
			int_storage = source_value;
			_utils_verbose_output(2, "ELEM: set %d to %s (LONG -> INT conversion)\n", int_storage, elem->name);

			ptr = &int_storage;
		}
		else
		{
			_utils_verbose_output(0, "ELEM: cannot set %ld to %s as it is larger than what MPC_CONF_INT can store\n", source_value, elem->name);
			abort();
		}
	}
	else if( (elem->type == MPC_CONF_LONG_INT) && (type == MPC_CONF_INT) )
	{
		/* Promote int to long when needed */
		int long_storage = *((int*)ptr);
		_utils_verbose_output(2, "ELEM: set %ld to %s (INT -> LONG conversion)\n", long_storage, elem->name);
		ptr = &long_storage;
	}
	else if(elem->type != type)
	{
		_utils_verbose_output(0, "%s mismatching types between %s and %s\n", elem->name,
																			 mpc_conf_type_name(elem->type),
																			 mpc_conf_type_name(type) );
					abort();
		return 1;
	}

	if(elem->is_locked)
	{
		_utils_verbose_output(0, "cannot write to %s which is LOCKED\n", elem->name);
		return 1;
	}

	_utils_verbose_output(3, "ELEM: setting @'%p' to %s\n", ptr, elem->name);

	mpc_conf_type_set_value(elem->type, &elem->addr, ptr);

	return 0;
}


int mpc_conf_config_type_elem_set_doc(mpc_conf_config_type_elem_t *elem, char * doc)
{
	if(!elem)
	{
		return 1;
	}

	free(elem->doc);

	elem->doc = strdup(doc);

	return 0;
}

int mpc_config_type_pop_elem(mpc_conf_config_type_t *type, mpc_conf_config_type_elem_t * elem)
{
	unsigned int i;

	for( i = 0 ; i < type->elem_count; i++)
	{
		if(type->elems[i] == elem)
		{
			/* Found */
			unsigned int j;
			for(j = i ; j < type->elem_count -1; j++)
			{
				type->elems[j] = type->elems[j + 1];
			}

			type->elems[type->elem_count] = NULL;
			type->elem_count--;

			return 0;
		}
	}

	return -1;
}


int mpc_config_type_match_order(mpc_conf_config_type_t *type, mpc_conf_config_type_t *ref)
{
	if( type->elem_count < ref->elem_count  )
	{
		_utils_verbose_output(1, "REORDER: reordered type must be at least as large as ref type (type %d & ref %d)\n", type->elem_count, ref->elem_count);
		return -1;
	}


	/* Move all known types to match the order of ref */
	unsigned int i, j;

	for( i = 0 ; i < ref->elem_count; i++)
	{
		/* Now find it in type */
		for( j = 0 ; j < type->elem_count; j++)
		{
			if(!strcmp(type->elems[j]->name, ref->elems[i]->name))
			{
				if( i != j)
				{
					/* Was found at != offset we swap to ref's offset */
					mpc_conf_config_type_elem_t * tmp = type->elems[i];
					type->elems[i] = type->elems[j];
					type->elems[j] = tmp;
				}
			}
		}
	}


	return 0;
}


int mpc_conf_config_type_elem_set_from_string(mpc_conf_config_type_elem_t *elem, mpc_conf_type_t type, char *string)
{
	_utils_verbose_output(3, "ELEM: setting string '%s' to %s\n", string, elem->name);

	if(elem->is_locked)
	{
		_utils_verbose_output(0, "cannot write to %s which is LOCKED\n", elem->name);
		return 1;
	}

	if(type != elem->type)
	{
		/* Allow LONG_INT to INT */
		if((elem->type == MPC_CONF_LONG_INT) || (elem->type == MPC_CONF_INT) )
		{
			type = elem->type;
		}
		else
		{
			_utils_verbose_output(3, "ELEM: mismatching types %s != %s  when setting '%s' to %s\n", mpc_conf_type_name(type) , mpc_conf_type_name(elem->type), elem->name, string);
			return -1;
		}

	}

	return mpc_conf_type_set_value_from_string(elem->type, &elem->addr, string, elem->ekv, elem->ekv_length);
}

void mpc_conf_config_type_elem_set_to_free(mpc_conf_config_type_elem_t *elem, int to_free)
{
	assert(elem != NULL);

	elem->addr_is_to_free = to_free;
}

void mpc_conf_config_type_elem_set_locked(mpc_conf_config_type_elem_t *elem, int locked)
{
	assert(elem != NULL);

	elem->is_locked = locked;
}

#define COMMENT_OFFSET    40

static inline int __mpc_conf_config_type_elem_print_xml(FILE *fd, mpc_conf_config_type_elem_t *elem, int indent)
{
	char  value_buff[256];
	char  ___indentbuff[32];
	char *indentstr = _utils_gen_indent(indent, ___indentbuff, 32);

	int can_do_color = _utils_conf_can_do_color(fd) ? 1 : 0;

	mpc_conf_type_print_value(elem->type, value_buff, 256, elem->addr, can_do_color, elem->ekv, elem->ekv_length);

	char __spacebuff[128];

	char outbuf[1024];
	/* Consider len without escape sequences */
	int len = snprintf(outbuf, 1024, "%s<%s>%s</%s>", indentstr, elem->name, value_buff, elem->name);

	if(can_do_color)
	{
		snprintf(outbuf, 1024, "%s"BOLD (GREEN("<%s>") ) "%s"BOLD (GREEN("</%s>") ), indentstr, elem->name, value_buff, elem->name);
	}

  char enumbuf[1024] = "";
  if(elem->type == MPC_CONF_ENUM) {
    if(elem->ekv_length > 0) {
      snprintf(enumbuf, 1024, " {%s", elem->ekv[0].key);
    }
    for(int i = 1; i < elem->ekv_length; i++) {
      snprintf(&(enumbuf[strlen(enumbuf)]), 1024 - strlen(enumbuf), ", %s", elem->ekv[i].key);
    }
    snprintf(&(enumbuf[strlen(enumbuf)]), 1024 - strlen(enumbuf), "}");
  }

	char docbuf[1024];
	if(can_do_color)
	{
		snprintf(docbuf, 1024, CYAN("<!-- [%s%s]%s %s -->"), mpc_conf_type_name(elem->type), enumbuf, elem->is_locked?"[LOCKED]":"", elem->doc);
	}
	else
	{
		snprintf(docbuf, 1024, "<!-- [%s%s] %s -->", mpc_conf_type_name(elem->type), enumbuf, elem->doc);
	}

	if(len < COMMENT_OFFSET)
	{
		char *spaces = _utils_gen_spaces(COMMENT_OFFSET - len, __spacebuff, 128);
		fprintf(fd, "%s%s%s\n", outbuf, spaces, docbuf);
	}
	else
	{
		fprintf(fd, "%s%s\n", indentstr, docbuf);
		fprintf(fd, "%s\n", outbuf);
	}

	return 0;
}


void __mpc_conf_config_type_elem_get_path_to(mpc_conf_config_type_elem_t *cur, mpc_conf_config_type_elem_t *last, char * separator, char * path, int len)
{
	if(cur->parent)
	{
		__mpc_conf_config_type_elem_get_path_to(cur->parent, last, separator, path, len);
	}

	strncat(path, cur->name, len);

	if(cur != last)
	{
		strncat(path, separator, len);
	}
}


int mpc_conf_config_type_elem_get_path_to(mpc_conf_config_type_elem_t *elem, char * path, int len, char * separator)
{
	path[0] = '\0';
	__mpc_conf_config_type_elem_get_path_to(elem, elem, separator, path, len);
	return 0;
}



static inline int __mpc_conf_config_type_elem_print_gen(FILE *fd, mpc_conf_config_type_elem_t *elem, char * separator, int to_upper)
{
	char  value_buff[256];
	int can_do_color = _utils_conf_can_do_color(fd) ? 1 : 0;
	mpc_conf_type_print_value(elem->type, value_buff, 256, elem->addr, can_do_color, elem->ekv, elem->ekv_length);

	char path[512];
	mpc_conf_config_type_elem_get_path_to(elem, path, 512, separator);

	if(to_upper)
	{
		int i;
		int len = strlen(path);

		for( i = 0 ; i < len ; i++)
		{
			path[i] = toupper(path[i]);
		}
	}

	if(can_do_color)
	{
		fprintf(fd,CYAN("# %s")"\n", elem->doc);
	}
	else
	{
		fprintf(fd,"#%s\n", elem->doc);
	}

	fprintf(fd,"%s=%s\n", path, value_buff);

	return 0;
}

int mpc_conf_config_type_elem_print_fd(mpc_conf_config_type_elem_t *elem, FILE * fd, mpc_conf_output_type_t output_type)
{
	if(elem->type == MPC_CONF_TYPE)
	{
		return mpc_conf_config_type_print_fd(mpc_conf_config_type_elem_get_inner(elem), fd, output_type);
	}

	switch(output_type)
	{
		case MPC_CONF_FORMAT_XML:
			return __mpc_conf_config_type_elem_print_xml(stdout, elem, 0);
		break;
		case MPC_CONF_FORMAT_CONF:
			return __mpc_conf_config_type_elem_print_gen(fd, elem, "_", 1);
		case MPC_CONF_FORMAT_INI:
			return __mpc_conf_config_type_elem_print_gen(fd, elem, ".", 0);
		default:
			return 1;
	}

	return 1;
}



int mpc_conf_config_type_elem_print(mpc_conf_config_type_elem_t *elem, mpc_conf_output_type_t output_type)
{
	return mpc_conf_config_type_elem_print_fd(elem, stdout, output_type);
}

mpc_conf_config_type_t *mpc_conf_config_type_elem_get_inner(mpc_conf_config_type_elem_t *elem)
{
	assert(elem);
	assert(elem->type ==MPC_CONF_TYPE);

	if(elem->type !=MPC_CONF_TYPE)
	{
		return NULL;
	}

	return (mpc_conf_config_type_t *)elem->addr;
}

/**********************
* CONFIGURATION TYPE *
**********************/

mpc_conf_config_type_t *mpc_conf_config_type_init(char *name, ...)
{
	mpc_conf_config_type_t *ret = malloc(sizeof(mpc_conf_config_type_t) );

	assert(ret != NULL);

	ret->elem_count = 0;
	ret->name       = strdup(name);

	/* Now set to lowercase */
	_utils_lower_string(ret->name);

	va_list ap;

	va_start(ap, name);

	while(1)
	{
		char *pname = va_arg(ap, char *);

		if(pname == NULL)
		{
			break;
		}

		if(ret->elem_count == (MPC_CONF_CONFIG_TYPE_MAX_ELEM - 1) )
		{
			_utils_verbose_output(0, "'%s' cannot store more than %d elements\n", name, MPC_CONF_CONFIG_TYPE_MAX_ELEM);
			break;
		}

		void *          pptr = va_arg(ap, void *);
		mpc_conf_type_t type = va_arg(ap, mpc_conf_type_t);
		char *          pdoc = va_arg(ap, char *);

    mpc_conf_enum_keyval_t * pekv = NULL;
    int pekv_length = 0;
    if(type == MPC_CONF_ENUM)
    {
      pekv_length = va_arg(ap, int);
      pekv = va_arg(ap, mpc_conf_enum_keyval_t*);
    }

		if(mpc_conf_config_type_get(ret, pname) )
		{
			_utils_verbose_output(0, "'%s' is already present in '%s'\n", pname, ret->name);
			break;
		}

		mpc_conf_config_type_elem_t *elem = mpc_conf_config_type_elem_init(pname, pptr, type, pdoc, pekv, pekv_length);
		assert(elem != NULL);

		ret->elems[ret->elem_count] = elem;
		ret->elem_count++;
	}

	va_end(ap);

	/* This recalculates all the parent pointers */
	__mpc_conf_root_config_check_parenting();

	return ret;
}

int mpc_config_type_append_elem(mpc_conf_config_type_t *type, mpc_conf_config_type_elem_t * elem)
{

	if(mpc_conf_config_type_get(type, elem->name) )
	{
		_utils_verbose_output(0, "'%s' is already present in '%s'\n", elem->name, type->name);
		return -1;
	}

	assert(elem != NULL);

	type->elems[type->elem_count] = elem;
	type->elem_count++;

	return 0;
}


mpc_conf_config_type_elem_t *mpc_conf_config_type_append(mpc_conf_config_type_t *type, char *ename, void *eptr, mpc_conf_type_t etype, char *edoc, mpc_conf_enum_keyval_t * eekv, int eekv_length)
{
	assert(type != NULL);

	if(mpc_conf_config_type_get(type, ename) )
	{
		_utils_verbose_output(0, "'%s' is already present in '%s'\n", ename, type->name);
		return NULL;
	}

	if(type->elem_count == (MPC_CONF_CONFIG_TYPE_MAX_ELEM - 1) )
	{
		_utils_verbose_output(0, "'%s' cannot store more than %d elements when adding %s\n", type->name,
		        MPC_CONF_CONFIG_TYPE_MAX_ELEM,
		        ename);
		return NULL;
	}

	mpc_conf_config_type_elem_t *elem = mpc_conf_config_type_elem_init(ename, eptr, etype, edoc, eekv, eekv_length);
	assert(elem != NULL);

	type->elems[type->elem_count] = elem;
	type->elem_count++;

	/* This recalculates all the parent pointers */
	__mpc_conf_root_config_check_parenting();

	return elem;
}

mpc_conf_config_type_elem_t *mpc_conf_config_type_get(mpc_conf_config_type_t *type, char *name)
{
	assert(type != NULL);

	unsigned int i;

	for(i = 0; i < type->elem_count; i++)
	{
		if(!strcasecmp(type->elems[i]->name, name) )
		{
			return type->elems[i];
		}
	}

	return NULL;
}

int mpc_conf_config_type_delete(mpc_conf_config_type_t *type, char *name)
{
	assert(type != NULL);

	unsigned int i, j;

	for(i = 0; i < type->elem_count; i++)
	{
		if(!strcasecmp(type->elems[i]->name, name) )
		{
			if(type->elems[i]->type ==MPC_CONF_TYPE)
			{
				mpc_conf_config_type_release( (mpc_conf_config_type_t **)&type->elems[i]->addr);
			}

			mpc_conf_config_type_elem_release(&type->elems[i]);

			/* Shift left to fill the empty slot */
			for(j = i; j < type->elem_count - 1; j++)
			{
				type->elems[j] = type->elems[j + 1];
			}


			type->elem_count--;
			/* All done */
			return 0;
		}
	}

	return 1;
}

void mpc_conf_config_type_clear(mpc_conf_config_type_t *type)
{
	unsigned int i;

	for(i = 0; i < type->elem_count; i++)
	{
		if(type->elems[i]->type ==MPC_CONF_TYPE)
		{
			mpc_conf_config_type_release( (mpc_conf_config_type_t **)&type->elems[i]->addr);
		}

		mpc_conf_config_type_elem_release(&type->elems[i]);
	}

	type->elem_count = 0;
}

void mpc_conf_config_type_release(mpc_conf_config_type_t **ptype)
{
	mpc_conf_config_type_t *type = *ptype;

	if(!type)
	{
		return;
	}

	_utils_verbose_output(3, "TYPE: release %s\n", type->name);

	free(type->name);

	mpc_conf_config_type_clear(type);

	memset(type, 0, sizeof(mpc_conf_config_type_t) );

	free(type);

	*ptype = NULL;
}

static inline int __mpc_conf_config_type_print_xml(mpc_conf_config_type_t *type, FILE *fd, int indent)
{
	int can_do_color = _utils_conf_can_do_color(fd) ? 1 : 0;

	char  ___indentbuff[32];
	char *indentstr = _utils_gen_indent(indent, ___indentbuff, 32);

	char  ___indentbuff1[32];
	char *indentstr_inner = _utils_gen_indent(indent + 1, ___indentbuff1, 32);

	if(can_do_color)
	{
		fprintf(fd, "%s"BOLD (RED("<%s>") ) "\n", indentstr, type->name);
	}
	else
	{
		fprintf(fd, "%s<%s>\n", indentstr, type->name);
	}

	unsigned int i;
	for(i = 0; i < type->elem_count; i++)
	{
		if(type->elems[i]->type ==MPC_CONF_TYPE)
		{
			if(can_do_color)
			{
				fprintf(fd, "%s"BOLD (CYAN("<!-- %s -->") ) "\n", indentstr_inner, type->elems[i]->doc);
			}
			else
			{
				fprintf(fd, "%s<!-- %s -->\n", indentstr_inner, type->elems[i]->doc);
			}
			__mpc_conf_config_type_print_xml(mpc_conf_config_type_elem_get_inner(type->elems[i]), fd, indent + 1);
		}
		else
		{
			__mpc_conf_config_type_elem_print_xml(fd, type->elems[i], indent + 1);
		}
	}

	if(can_do_color)
	{
		fprintf(fd, "%s"BOLD (RED("</%s>") ) "\n", indentstr, type->name);
	}
	else
	{
		fprintf(fd, "%s</%s>\n", indentstr, type->name);
	}

	return 0;
}

static inline int __mpc_conf_config_type_print_conf(mpc_conf_config_type_t *type, FILE *fd, mpc_conf_output_type_t output_type)
{
	int can_do_color = _utils_conf_can_do_color(fd) ? 1 : 0;

	int ret = 0;
	unsigned int i;
	for(i = 0; i < type->elem_count; i++)
	{
		if(type->elems[i]->type ==MPC_CONF_TYPE)
		{
			if(can_do_color)
			{
				fprintf(fd,"\n"BOLD (RED("# %s"))"\n\n",type->elems[i]->doc);
			}
			else
			{
				fprintf(fd,"\n#%s\n\n",type->elems[i]->doc);
			}

			ret |= __mpc_conf_config_type_print_conf(mpc_conf_config_type_elem_get_inner(type->elems[i]), fd, output_type);
		}
		else
		{
			ret |= mpc_conf_config_type_elem_print_fd(type->elems[i], fd, output_type);
		}
	}

	return ret;
}


int mpc_conf_config_type_print_fd(mpc_conf_config_type_t *type, FILE * fd, mpc_conf_output_type_t output_type)
{
	switch(output_type)
	{
		case MPC_CONF_FORMAT_XML:
			return __mpc_conf_config_type_print_xml(type, fd, 0);
		break;
		case MPC_CONF_FORMAT_CONF:
		case MPC_CONF_FORMAT_INI:
			return __mpc_conf_config_type_print_conf(type, fd, output_type);
		break;
		default:
			return 1;
	}

	return 1;


}

int mpc_conf_config_type_print(mpc_conf_config_type_t *type, mpc_conf_output_type_t output_type)
{
	return mpc_conf_config_type_print_fd(type, stdout, output_type);
}

int mpc_conf_config_type_count(mpc_conf_config_type_t *type)
{
	return type->elem_count;
}

mpc_conf_config_type_elem_t *mpc_conf_config_type_nth(mpc_conf_config_type_t *type, unsigned int id)
{
	if(type->elem_count <= id)
	{
		return NULL;
	}

	return type->elems[id];
}

mpc_conf_config_type_t *  mpc_conf_type_elem_get_root(mpc_conf_config_type_elem_t * elem)
{
	mpc_conf_config_type_elem_t * cur = elem;

	while(cur->parent)
	{
		cur = cur->parent;
	}

	if(cur->type == MPC_CONF_TYPE)
	{
		return mpc_conf_config_type_elem_get_inner(cur);
	}
	else
	{
		/* Should never happen */
		return NULL;
	}
}

int mpc_conf_type_elem_get_as_int(mpc_conf_config_type_elem_t * elem)
{
	if(elem->type != MPC_CONF_INT)
	{
		_utils_verbose_output(0, "trying to get an int from a non int type %s\n", elem->name);
		abort();
	}

	return *((int*)elem->addr);
}


char * mpc_conf_type_elem_get_as_string(mpc_conf_config_type_elem_t * elem)
{
	if(elem->type != MPC_CONF_STRING)
	{
		_utils_verbose_output(0, "trying to get a string from a non string type %s\n", elem->name);
		abort();
	}

	return ((char*)elem->addr);
}

/*********************************
 * Environment Manager Definition *
 **********************************/

#define MPC_CONF_MAX_CONFIGS    128

typedef struct mpc_conf_env_key_s
{
	char *                               name;
	char *                               value;
	struct mpc_conf_env_processed_key_s *next;
}mpc_conf_env_key_t;


mpc_conf_env_key_t *mpc_conf_env_key_get(mpc_conf_env_key_t *key, char *name)
{
	mpc_conf_env_key_t *cur = key;

	while(cur)
	{
		if(!strcasecmp(cur->name, name) )
		{
			return key;
		}

		cur = (mpc_conf_env_key_t *)cur->next;
	}

	return NULL;
}

int mpc_conf_env_key_push(mpc_conf_env_key_t **head, char *name, char *value)
{
	mpc_conf_env_key_t *new = malloc(sizeof(mpc_conf_env_key_t) );

	if(!new)
	{
		perror("malloc");
		return 1;
	}

	new->name  = strdup(name);
	new->value = strdup(value);

	new->next = (struct mpc_conf_env_processed_key_s *)*head;
	*head     = new;

	return 0;
}

int mpc_conf_env_key_free(mpc_conf_env_key_t **key, int recursive)
{
	if(!(*key) )
	{
		/* All done */
		return 0;
	}

	free( (*key)->name);
	(*key)->name = NULL;

	free( (*key)->value);
	(*key)->value = NULL;

	if( (*key)->next)
	{
		mpc_conf_env_key_free( (mpc_conf_env_key_t **)&(*key)->next, recursive);
	}

	free(*key);
	*key = NULL;

	return 0;
}

mpc_conf_env_key_t *__mpc_conf_env_key_del(mpc_conf_env_key_t *key, char *name)
{
	if(!strcasecmp(key->name, name) )
	{
		mpc_conf_env_key_t *ret = (mpc_conf_env_key_t *)key->next;
		mpc_conf_env_key_free(&key, 0);
		return ret;
	}

	if(key->next)
	{
		mpc_conf_env_key_t *ret = __mpc_conf_env_key_del( (mpc_conf_env_key_t *)key->next, name);

		if(ret != NULL)
		{
			key->next = (struct mpc_conf_env_processed_key_s *)ret;
		}
	}

	return NULL;
}

int mpc_conf_env_key_del(mpc_conf_env_key_t **key, char *name)
{
	mpc_conf_env_key_t *ret = __mpc_conf_env_key_del(*key, name);

	if(ret != NULL)
	{
		*key = (mpc_conf_env_key_t *)ret;
	}

	return 0;
}

typedef struct mpc_conf_env_manager_s
{
	int                 init_done;
	mpc_conf_env_key_t *processed_keys;
	mpc_conf_env_key_t *loaded_keys;
}mpc_conf_env_manager_t;


#define ENV_STATIC_SIZE    (10 * 1024 * 1024)

static inline int __read_environ(mpc_conf_env_manager_t *envm)
{
	size_t env_len = ENV_STATIC_SIZE;

	char *envdat = malloc(env_len);

	if(envdat == NULL)
	{
		return 1;
	}

	FILE *fprocenv = fopen("/proc/self/environ", "r");

	if(!fprocenv)
	{
		perror("fopen");
		return 1;
	}

	/* Read env content */
	env_len = fread(envdat, sizeof(char), env_len, fprocenv);

	if(env_len == 0)
	{
		free(envdat);
		return 1;
	}

	if(env_len == ENV_STATIC_SIZE)
	{
		_utils_verbose_output(0, "environment was too large (large than %ld bytes)\n", (long int)ENV_STATIC_SIZE);
		return 1;
	}

	/* Now we read null separated env variables with first =
	 * separating KEY from VALUE */

	char *cur_entry = envdat;

	while(cur_entry < (envdat + env_len) )
	{
		int entry_len = strlen(cur_entry);

		char *equal = strchr(cur_entry, '=');

		if(equal)
		{
			*equal = '\0';
			char *key   = cur_entry;
			char *value = equal + 1;

			if(mpc_conf_env_key_push(&envm->loaded_keys, key, value) != 0)
			{
				_utils_verbose_output(0, "failed to read environment variable %s = %s\n", key, value);
			}
		}

		cur_entry = (cur_entry + entry_len) + 1;
	}

	free(envdat);
	fclose(fprocenv);

	return 0;
}

int mpc_conf_env_manager_for_each(mpc_conf_env_manager_t *envm,
                                  int (*callback)(mpc_conf_env_manager_t *envm, mpc_conf_env_key_t *key,
                                                  void *ptr),
                                  void *ptr)
{
	if(!callback)
	{
		return 1;
	}

	mpc_conf_env_key_t *cur = envm->loaded_keys;

	while(cur)
	{
		if( (callback)(envm, cur, ptr) )
		{
			return 1;
		}

		cur = (mpc_conf_env_key_t *)cur->next;
	}

	return 0;
}

int mpc_conf_env_manager_check_init(mpc_conf_env_manager_t *envm)
{
	envm->init_done++;


	if(envm->init_done > 1)
	{
		return 0;
	}

	envm->loaded_keys    = NULL;
	envm->processed_keys = NULL;

	if(__read_environ(envm) )
	{
		return 1;
	}

	return 0;
}

int mpc_conf_env_manager_check_release(mpc_conf_env_manager_t *envm)
{
	envm->init_done--;

	if(envm->init_done)
	{
		/* Not the last one */
		return 0;
	}

	mpc_conf_env_key_free(&envm->processed_keys, 1);
	mpc_conf_env_key_free(&envm->loaded_keys, 1);

	return 0;
}

/*******************
* MPC_CONF CONFIG *
*******************/

int mpc_conf_self_config_check_init(mpc_conf_self_config_t *config)
{
	config->init_done++;

	if(config->init_done > 1)
	{
		return 0;
	}

	/* Load defaults */

	config->color_enabled = MPC_CONF_COLOR;
	config->indent_count  = MPC_CONF_DEFAULT_INDENT;
	config->verbose       = 0;

	/* As envirnoment might be loaded later on
	   we make an exception to allow config to be debuged early */
	char *env_conf_verb = getenv("CONF_SETTINGS_VERBOSE");
	if(env_conf_verb)
	{
		config->verbose = atoi(env_conf_verb);
	}

	config->is_valid      = 1;

	mpc_conf_root_config_init("conf");

	mpc_conf_config_type_t *self_conf = mpc_conf_config_type_init("settings",
	                                                              PARAM("color",
	                                                                    &config->color_enabled,
	                                                                   MPC_CONF_BOOL,
	                                                                    "Enable color inside mpc_conf's output"),
	                                                              PARAM("indent",
	                                                                    &config->indent_count,
	                                                                   MPC_CONF_INT,
	                                                                    "Number of spaces used to indent mpc_conf's output"),
	                                                              PARAM("verbose",
	                                                                    &config->verbose,
	                                                                   MPC_CONF_INT,
	                                                                    "mpc conf's verbosity level (0-3)"),
	                                                              NULL);

	mpc_conf_root_config_append("conf", self_conf, "mpc conf Configuration");

	mpc_conf_config_type_elem_t *conf_type = mpc_conf_root_config_get("conf");
	if(conf_type && conf_type->type == MPC_CONF_TYPE)
	{
			mpc_conf_config_type_elem_t *ve = mpc_conf_config_type_append(mpc_conf_config_type_elem_get_inner(conf_type),
																		  "valid",
																		  &config->is_valid,
																		  MPC_CONF_BOOL,
																		  "Utility variable to check for configuration validity", NULL, 0);
			mpc_conf_config_type_elem_set_locked(ve, 1);
	}
	else
	{
		return 1;
	}

	mpc_conf_config_type_t *paths = mpc_conf_config_type_init("paths", NULL );
	mpc_conf_root_config_append("conf", paths, "mpc conf Search Paths");

	return 0;
}

int mpc_conf_self_config_check_release(mpc_conf_self_config_t *config)
{
	if(config->init_done == 0)
	{
		return 1;
	}

	config->init_done--;

	if(config->init_done)
	{
		return 0;
	}

	return mpc_conf_root_config_release("conf");
}

/***************
* ROOT CONFIG *
***************/

typedef struct mpc_conf_root_config_s
{
	mpc_conf_config_type_elem_t *elems[MPC_CONF_MAX_CONFIGS];
	mpc_conf_env_manager_t       env;
	mpc_conf_self_config_t       config;
}mpc_conf_root_config_t;

static mpc_conf_root_config_t __root_config = { { 0 }, { 0 }, { 0 } };

mpc_conf_self_config_t *mpc_conf_self_config_get(void)
{
	if(!__root_config.config.init_done)
	{
		return NULL;
	}

	return &__root_config.config;
}

static inline mpc_conf_config_type_elem_t **__get_global_config_ref_by_name(char *name)
{
	int i = 0;

	while(i < MPC_CONF_MAX_CONFIGS)
	{
		if(__root_config.elems[i])
		{
			if(!strcasecmp(__root_config.elems[i]->name, name) )
			{
				return &__root_config.elems[i];
			}
		}
		i++;
	}

	return NULL;
}

static inline mpc_conf_config_type_elem_t *__get_global_config_by_name(char *name)
{
	mpc_conf_config_type_elem_t **ref = __get_global_config_ref_by_name(name);

	if(ref)
	{
		return *ref;
	}

	return NULL;
}

static inline int __append_global_config(char *name, mpc_conf_config_type_elem_t *elem)
{
	int i = 0;

	while( (i < MPC_CONF_MAX_CONFIGS) && (__root_config.elems[i] != NULL) )
	{
		i++;
	}

	if(i == MPC_CONF_MAX_CONFIGS)
	{
		_utils_verbose_output(0, "for %s cannot register more than %d configs\n", name, MPC_CONF_MAX_CONFIGS);
		return 1;
	}

	__root_config.elems[i] = elem;

	return 0;
}

mpc_conf_config_type_elem_t * mpc_conf_root_elem(char * conf_name)
{
	mpc_conf_config_type_elem_t *conf_root_elem = __get_global_config_by_name(conf_name);

	if(!conf_root_elem)
	{
		return NULL;
	}

	return conf_root_elem;
}

mpc_conf_config_type_t *mpc_conf_root_config(char *conf_name)
{
	mpc_conf_config_type_elem_t * conf_root_elem = mpc_conf_root_elem(conf_name);

	if(!conf_root_elem)
	{
		return NULL;
	}

	return mpc_conf_config_type_elem_get_inner(conf_root_elem);
}

int mpc_conf_root_config_count(void)
{
	int cnt = 0;

	int i;

	for(i = 0; i < MPC_CONF_MAX_CONFIGS; i++)
	{
		if(__root_config.elems[i])
		{
			cnt++;
		}
	}

	return cnt;
}

mpc_conf_config_type_t *mpc_conf_root_config_nth(int id)
{
	int cnt = 0;

	int i;

	for(i = 0; i < MPC_CONF_MAX_CONFIGS; i++)
	{
		if(__root_config.elems[i])
		{
			if(cnt == id)
			{
				return mpc_conf_config_type_elem_get_inner(__root_config.elems[i]);
			}

			cnt++;
		}
	}

	return NULL;
}

int mpc_conf_root_config_append(char *conf_name, mpc_conf_config_type_t *conf, char *doc)
{
	_utils_verbose_output(1, "CONF: appending %s @ %p //%s\n", conf_name, conf, doc);
	mpc_conf_config_type_t *root_node = mpc_conf_root_config(conf_name);

	if(!root_node)
	{
		_utils_verbose_output(0, "no such root config %s\n", conf_name);
		return 1;
	}

	mpc_conf_config_type_append(root_node, conf->name, conf,MPC_CONF_TYPE, doc, NULL, 0);

	return 0;
}

int __root_conf_alter_from_env(mpc_conf_env_manager_t *envm, mpc_conf_env_key_t *key, void *ptr)
{
	char *conf_name = (char *)ptr;
	int   i;

	int len_key  = strlen(key->name);
	int len_name = strlen(conf_name);

	/* Make sure the key starts with the config prefix */

	for(i = 0; (i < len_key) && (i < len_name); i++)
	{
		if(tolower(conf_name[i]) != tolower(key->name[i]) )
		{
			return 0;
		}
	}

	/* Make sure the value was not already processed */
	if(mpc_conf_env_key_get(envm->processed_keys, key->name) )
	{
		_utils_verbose_output(2, "ENV: %s already processed\n", key->name);
		return 1;
	}

	/* Now push in the value from string */
	mpc_conf_config_type_elem_t *elem = mpc_conf_root_config_get_sep(key->name, "_");

	if(elem)
	{
		_utils_verbose_output(1, "(env) set %s = %s\n", key->name, key->value);

		if(mpc_conf_config_type_elem_set_from_string(elem, elem->type, key->value) != 0)
		{
			_utils_verbose_output(0, "could not load configuration from env [%s = %s]\n", key->name, key->value);
			return 1;
		}

		mpc_conf_env_key_push(&envm->processed_keys, key->name, key->value);
	}
	else
	{
		_utils_verbose_output(1, "ENV: could not find element to load environment variable %s\n", key->name);
	}

	return 0;
}

int mpc_conf_root_config_load_env(char *conf_name)
{
	_utils_verbose_output(1, "ENV: loading environment variables for %s\n", conf_name);
	return mpc_conf_env_manager_for_each(&__root_config.env, __root_conf_alter_from_env, (void *)conf_name);
}

int mpc_conf_root_config_load_env_all(void)
{
	int i;

	for(i = 0; i < MPC_CONF_MAX_CONFIGS; i++)
	{
		if(__root_config.elems[i])
		{
			mpc_conf_root_config_load_env(__root_config.elems[i]->name);
		}
	}

	return 0;
}

int mpc_conf_root_config_load_files(char *conf_name)
{
	_utils_verbose_output(1, "FILES: loading configuration files for %s\n", conf_name);
	return mpc_conf_config_load(conf_name);
}

int mpc_conf_root_config_load_files_all(void)
{
	int i;

	for(i = 0; i < MPC_CONF_MAX_CONFIGS; i++)
	{
		if(__root_config.elems[i])
		{
			mpc_conf_root_config_load_files(__root_config.elems[i]->name);
		}
	}

	return 0;
}

int mpc_conf_root_config_init(char *conf_name)
{
	mpc_conf_env_manager_check_init(&__root_config.env);
	mpc_conf_self_config_check_init(&__root_config.config);

	/* Ensure not present yet */
	mpc_conf_config_type_elem_t *existing_elem = __get_global_config_by_name(conf_name);

	if(existing_elem)
	{
		_utils_verbose_output(0, "a root config %s is already present\n", conf_name);
		return 1;
	}

	mpc_conf_config_type_t *     root_node   = mpc_conf_config_type_init(conf_name, NULL);
	mpc_conf_config_type_elem_t *config_elem = mpc_conf_config_type_elem_init(conf_name, root_node,MPC_CONF_TYPE, "Config ROOT Node", NULL, 0);

	if(__append_global_config(conf_name, config_elem) )
	{
		return 1;
	}

	return 0;
}

static inline void __mpc_conf_root_config_release_elem(mpc_conf_config_type_elem_t **elem)
{
	mpc_conf_env_manager_check_release(&__root_config.env);
	mpc_conf_self_config_check_release(&__root_config.config);

	assert(elem != NULL);
	mpc_conf_config_type_release( (mpc_conf_config_type_t **)&(*elem)->addr);
	mpc_conf_config_type_elem_release(elem);
}

int mpc_conf_root_config_release(char *conf_name)
{
	mpc_conf_config_type_elem_t **existing_elem = __get_global_config_ref_by_name(conf_name);

	if(!existing_elem)
	{
		return 1;
	}

	__mpc_conf_root_config_release_elem(existing_elem);

	return 0;
}

int mpc_conf_root_config_release_all(void)
{
	int i;

	for(i = 0; i < MPC_CONF_MAX_CONFIGS; i++)
	{
		if(__root_config.elems[i])
		{
			__mpc_conf_root_config_release_elem(&__root_config.elems[i]);
		}
	}

	return 0;
}

mpc_conf_config_type_elem_t *mpc_conf_root_config_get_sep(char *name, char *separator)
{
	_utils_verbose_output(3, "CONF: get '%s' (sep '%s')\n", name, separator);
	char *to_search = strdup(name);
	char *token     = NULL;
	char *saveptr1  = NULL;

	/* Get root node */
	token = strtok_r(to_search, separator, &saveptr1);

	mpc_conf_config_type_t *type = mpc_conf_root_config(token);

	/* First ensure first token is main storage key */
	if(!type)
	{
		_utils_verbose_output(0, "config root node '%s' not found in %s for %s\n", token, __FUNCTION__, name);
		free(to_search);
		return NULL;
	}

	mpc_conf_config_type_elem_t *elem = NULL;

	/* Move to next token */
	token = strtok_r(NULL, separator, &saveptr1);

	if(!token)
	{
		/* In this case we refer to the root elem
		 * as there was no following tokens */
		free(to_search);
		return __get_global_config_by_name(name);
	}

	while(token)
	{
		/* Make sure type can be taken from elem
		 * and that we are not looking in a non
		 * storage element */
		if(!type)
		{
			if(!elem)
			{
				break;
			}

			if(elem->type ==MPC_CONF_TYPE)
			{
				type = mpc_conf_config_type_elem_get_inner(elem);
			}
			else
			{
				_utils_verbose_output(0, "cannot enter '%s' in '%s' type is '%s' notMPC_CONF_TYPE\n",
				        token,
				        name,
				        mpc_conf_type_name(elem->type) );
				elem = NULL;
				break;
			}
		}


		elem = mpc_conf_config_type_get(type, token);

		if(!elem)
		{
			/* Not found */
			break;
		}
		else
		{
			/* Mark type as expanded */
			type = NULL;
		}

		token = strtok_r(NULL, separator, &saveptr1);
	}

	free(to_search);

	return elem;
}

mpc_conf_config_type_elem_t *mpc_conf_root_config_get(char *name)
{
	return mpc_conf_root_config_get_sep(name, ".");
}

static inline mpc_conf_config_type_t *__create_container(char *name, char *separator)
{
	char *to_search = strdup(name);
	char *token     = NULL;
	char *saveptr1  = NULL;

	/* Get root node */
	token = strtok_r(to_search, separator, &saveptr1);

	mpc_conf_config_type_t *type = mpc_conf_root_config(token);

	/* First ensure first token is main storage key */
	if(!type)
	{
		_utils_verbose_output(1, "config root node was '%s' not found in %s\n", token, __FUNCTION__);
		free(to_search);
		return NULL;
	}

	/* Move to next token */
	token = strtok_r(NULL, separator, &saveptr1);

	if(!token)
	{
		/* In this case we refer to the root elem
		 * as there was no following tokens */
		return type;
	}

	while(token)
	{
		mpc_conf_config_type_elem_t *elem = mpc_conf_config_type_get(type, token);

		if(!elem)
		{
			/* We need to create */
			mpc_conf_config_type_t *new_type = mpc_conf_config_type_init(token, NULL);
			mpc_conf_config_type_append(type, token, new_type,MPC_CONF_TYPE, "Dynamically inserted", NULL, 0);
			type = new_type;
		}
		else
		{
			if(elem->type !=MPC_CONF_TYPE)
			{
				_utils_verbose_output(0, "cannot insert in %s from %s type is %s notMPC_CONF_TYPE\n", elem->name,
				        name,
				        mpc_conf_type_name(elem->type) );
			}

			type = mpc_conf_config_type_elem_get_inner(elem);
		}

		token = strtok_r(NULL, separator, &saveptr1);
	}

	free(to_search);

	return type;
}

static inline int __get_prefix_and_suffix(char *name, char *separator, char **prefix, char **suffix)
{
	/* We need to go down the hierarchy
	 * ensuring that each entry with subkeys
	 * is part of aMPC_CONF_CONFIG entry
	 * and that the last entry if present
	 * matches the expected type that we wand to set */

	char *last_point = strrchr(name, separator[0]);

	if(!last_point)
	{
		_utils_verbose_output(0, "when setting elements there must be at least two entries in the path\n");
		_utils_verbose_output(0, "for example XX.XX\n");
		return -1;
	}

	*last_point = '\0';
	*prefix     = name;
	*suffix     = last_point + 1;

	if(!strlen(*suffix) )
	{
		_utils_verbose_output(0, "cannot set value in %s with no name\n", name);
		return -1;
	}

	return 0;
}

mpc_conf_config_type_elem_t *mpc_conf_root_config_set_sep(char *name, char *separator, mpc_conf_type_t etype, void *eptr, int allow_create)
{
	_utils_verbose_output(2, "CONF: set %p to '%s' (sep '%s') type %s can create %d\n", eptr, name, separator, mpc_conf_type_name(etype), allow_create);

	char *where_to_set = strdup(name);

	char *container  = NULL;
	char *value_name = NULL;

	if(__get_prefix_and_suffix(where_to_set, separator, &container, &value_name) )
	{
		free(where_to_set);
		return NULL;
	}

	/* First check if container exists */
	mpc_conf_config_type_elem_t *pcontelem = mpc_conf_root_config_get_sep(container, separator);
	mpc_conf_config_type_t *     pcont     = NULL;

	if(!pcontelem)
	{
		if(!allow_create)
		{
			free(where_to_set);
			return NULL;
		}

		/* CREATE CONT */
		pcont = __create_container(container, separator);
	}
	else
	{
		pcont = mpc_conf_config_type_elem_get_inner(pcontelem);
	}

	if(!pcont)
	{
		_utils_verbose_output(0, "could not insert in container %s\n", container);
		free(where_to_set);
		return NULL;
	}


	/* At this point container exits and check if elem exits */
	mpc_conf_config_type_elem_t *elem = mpc_conf_config_type_get(pcont, value_name);

	if(elem)
	{
		/* Override the value */
		mpc_conf_config_type_elem_set(elem, etype, eptr);
	}
	else
	{
		if(!allow_create)
		{
			free(where_to_set);
			return NULL;
		}

		elem = mpc_conf_config_type_append(pcont, value_name, eptr, etype, "Dynamically inserted", NULL, 0);
	}

	free(where_to_set);

	/* This recalculates all the parent pointers */
	__mpc_conf_root_config_check_parenting();

	return elem;
}

mpc_conf_config_type_elem_t *mpc_conf_root_config_set(char *name, mpc_conf_type_t etype, void *eptr, int allow_create)
{
	return mpc_conf_root_config_set_sep(name, ".", etype, eptr, allow_create);
}

int mpc_conf_root_config_delete_sep(char *name, char *separator)
{
	_utils_verbose_output(2, "CONF: delete '%s' (sep '%s')\n", name, separator);

	char *where_to_set = strdup(name);

	char *container  = NULL;
	char *value_name = NULL;

	if(__get_prefix_and_suffix(where_to_set, separator, &container, &value_name) )
	{
		free(where_to_set);
		return -1;
	}

	/* First check if container exists */
	mpc_conf_config_type_elem_t *pcontelem = mpc_conf_root_config_get_sep(container, separator);

	/* No such elem */
	if(!pcontelem)
	{
		free(where_to_set);
		return 1;
	}

	/* Not a container */
	if(pcontelem->type !=MPC_CONF_TYPE)
	{
		free(where_to_set);
		return -1;
	}

	mpc_conf_config_type_t *pcont = mpc_conf_config_type_elem_get_inner(pcontelem);

	int ret = mpc_conf_config_type_delete(pcont, value_name);

	free(where_to_set);

	if(!ret)
	{
		_utils_verbose_output(3, "CONF: DID delete '%s' (sep '%s')\n", name, separator);
	}

	return ret;
}

int mpc_conf_root_config_delete(char *name)
{
	return mpc_conf_root_config_delete_sep(name, ".");
}

static int __mpc_conf_root_config_print_xml(FILE* fd)
{
	int indent = 0;

	if(fd != stdout)
	{
		__print_xml_header(fd);
		indent++;
	}

	int i;
	int conf_count = mpc_conf_root_config_count();

	int ret = 0;

	for(i = 0; i < conf_count; i++)
	{
		mpc_conf_config_type_t *conf = mpc_conf_root_config_nth(i);
		ret |= __mpc_conf_config_type_print_xml(conf, fd, indent);
	}


	if(fd != stdout)
	{
		__print_xml_footer(fd);
	}

	return ret;
}

static int __mpc_conf_root_config_print_gen(FILE* fd, mpc_conf_output_type_t output_type)
{
	int i;
	int conf_count = mpc_conf_root_config_count();

	int ret = 0;

	for(i = 0; i < conf_count; i++)
	{
		mpc_conf_config_type_t *conf = mpc_conf_root_config_nth(i);
		ret |= mpc_conf_config_type_print_fd(conf, fd, output_type);
	}

	return ret;
}

int mpc_conf_root_config_print_fd(FILE *fd, mpc_conf_output_type_t output_type)
{
	switch(output_type)
	{
		case MPC_CONF_FORMAT_XML:
			return __mpc_conf_root_config_print_xml(fd);
		break;
		case MPC_CONF_FORMAT_CONF:
		case MPC_CONF_FORMAT_INI:
			return __mpc_conf_root_config_print_gen(fd, output_type);
		break;
		default:
			return 1;
	}
}

static int ___mpc_conf_root_config_check_parenting(mpc_conf_config_type_elem_t *elem, mpc_conf_config_type_elem_t * parent)
{
	int ret = 0;

	if(elem->type == MPC_CONF_TYPE)
	{
		mpc_conf_config_type_t * type = mpc_conf_config_type_elem_get_inner(elem);

		unsigned int i;
		for(i = 0; i < type->elem_count; i++)
		{
			ret |= ___mpc_conf_root_config_check_parenting(type->elems[i], elem);
		}
	}

	elem->parent = parent;

	return ret;
}

static int __mpc_conf_root_config_check_parenting(void)
{
	int i;
	int conf_count = mpc_conf_root_config_count();

	int ret = 0;

	for(i = 0; i < conf_count; i++)
	{
		mpc_conf_config_type_elem_t *elem = __root_config.elems[i];
		ret |= ___mpc_conf_root_config_check_parenting(elem, NULL);
	}

	return ret;
}


int mpc_conf_root_config_print(mpc_conf_output_type_t output_type)
{
	return mpc_conf_root_config_print_fd(stdout, output_type);
}

int mpc_conf_root_config_save(char *output_file, mpc_conf_output_type_t output_type)
{
	FILE *out = fopen(output_file, "w");

	if(!out)
	{
		perror("fopen");
		return 1;
	}

	mpc_conf_root_config_print_fd(out, output_type);

	fclose(out);

	return 0;
}

int mpc_conf_root_config_set_lock(char * path, int is_locked)
{
	mpc_conf_config_type_elem_t *elem = mpc_conf_root_config_get(path);

	if(!elem)
	{
		return 1;
	}

	mpc_conf_config_type_elem_set_locked(elem, is_locked);

	return 0;
}

int mpc_conf_root_config_search_path(char *conf_name, char * system, char * user, char * rights)
{
	mpc_conf_config_type_t * existing_conf = mpc_conf_config_loader_search_path(conf_name);

	if(existing_conf)
	{
		_utils_verbose_output(0, "search path already present for for %s\n", conf_name);
		return 1;
	}


	mpc_conf_config_type_elem_t *search_paths = mpc_conf_root_config_get("conf.paths");

	if(search_paths == NULL)
	{
		_utils_verbose_output(0, "could not get mpc_conf config\n");
		return 1;
	}

	mpc_conf_config_type_t *search_paths_type = mpc_conf_config_type_elem_get_inner(search_paths);

	if(!search_paths_type)
	{
		_utils_verbose_output(0, "could not get mpc_conf config type\n");
		return 1;
	}

	mpc_conf_config_type_t * new_conf = mpc_conf_config_loader_paths(conf_name,
																	 system,
																	 user,
																	 rights);

	if(new_conf == NULL)
	{
		_utils_verbose_output(0, "failed to set config search paths for %s\n", conf_name);
		return 1;
	}

	char doc[512];
	snprintf(doc, 512, "Search paths for %s", conf_name);

	mpc_conf_config_type_append(search_paths_type, conf_name, new_conf,MPC_CONF_TYPE, doc, NULL, 0);



	return 0;
}

/****************
 * CONF HELPERS *
 ****************/

int mpc_conf_resolvefunc(char * func_name, void (**func_ptr)())
{
	void * ptr = dlsym(NULL, func_name);

	*func_ptr = ptr;

	return (ptr != NULL);
}

char *mpc_conf_user_prefix(char *conf_name, char *buff, int size)
{
	struct passwd *pw    = getpwuid(getuid() );
	const char *   homed = pw->pw_dir;

	snprintf(buff, size, "%s/.config/%s", homed, conf_name);

	if(!_utils_is_dir(buff) )
	{
		char conf_dir[512];
		snprintf(conf_dir, 512, "%s/.config/", homed);
		/* Config dir is present ?*/
		if(!_utils_is_dir(conf_dir) )
		{
			if(mkdir(conf_dir, 0700) < 0)
			{
				perror("mkdir");
				return NULL;
			}
		}

		if(mkdir(buff, 0700) < 0)
		{
			perror("mkdir");
			return NULL;
		}
	}

	return buff;
}
