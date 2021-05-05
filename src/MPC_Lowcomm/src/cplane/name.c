#include "name.h"

#include <string.h>

#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>

struct _mpc_lowcomm_monitor_name_s
{
	char *                              name;
	char *                              port;
	mpc_lowcomm_peer_uid_t              hosting_peer;
	struct _mpc_lowcomm_monitor_name_s *prev;
};

static struct _mpc_lowcomm_monitor_name_s *__names = NULL;
static mpc_common_spinlock_t __names_lock          = SCTK_SPINLOCK_INITIALIZER;

static inline struct _mpc_lowcomm_monitor_name_s *__name_new(char *name, char * port_name, mpc_lowcomm_peer_uid_t hosting_peer)
{
	struct _mpc_lowcomm_monitor_name_s *ret = sctk_malloc(sizeof(struct _mpc_lowcomm_monitor_name_s) );

	assume(ret != NULL);

	ret->name = strdup(name);
	ret->port = strdup(port_name);
	ret->hosting_peer = hosting_peer;
	ret->prev = NULL;

	return ret;
}

static inline struct _mpc_lowcomm_monitor_name_s *__name_get_no_lock(char *name)
{
	struct _mpc_lowcomm_monitor_name_s *ret = NULL;

	struct _mpc_lowcomm_monitor_name_s *cur = __names;

	while(cur)
	{
		if(!strcmp(cur->name, name) )
		{
			ret = cur;
			break;
		}
		cur = cur->prev;
	}

    return ret;
}

static inline struct _mpc_lowcomm_monitor_name_s *__name_get(char *name)
{
	struct _mpc_lowcomm_monitor_name_s *ret = NULL;

	mpc_common_spinlock_lock(&__names_lock);

	ret = __name_get_no_lock(name);

	mpc_common_spinlock_unlock(&__names_lock);

	return ret;
}

static inline void __name_free(struct _mpc_lowcomm_monitor_name_s *n)
{
	free(n->name);
	free(n->port);

	sctk_free(n);
}

int __name_drop(char *name)
{
	struct _mpc_lowcomm_monitor_name_s *to_free = NULL;
	int ret = 1;

	mpc_common_spinlock_lock(&__names_lock);

	if(__names)
	{
		/* If head is the one */
		if(!strcmp(__names->name, name) )
		{
			to_free = __names;
			__names = __names->prev;
			__name_free(to_free);
			ret = 0;
		}
		else
		{
			struct _mpc_lowcomm_monitor_name_s *cur = __names;

			while(cur->prev)
			{
				if(!strcmp(cur->prev->name, name) )
				{
					to_free   = cur->prev;
					cur->prev = cur->prev->prev;
					__name_free(to_free);
					ret = 0;
					break;
				}
			}
		}
	}


	mpc_common_spinlock_unlock(&__names_lock);

	return ret;
}

static inline int __name_push(struct _mpc_lowcomm_monitor_name_s *new)
{
	int ret = 1;

	mpc_common_spinlock_lock(&__names_lock);

	/* Make sure we did not race */
	if(__name_get_no_lock(new->name) == NULL)
	{
		new->prev = __names;
		__names   = new;
		ret       = 0;
	}

	mpc_common_spinlock_unlock(&__names_lock);

	return ret;
}

static inline void __name_release(void)
{
	/* Leave locked to detect post mortem accesses */
	mpc_common_spinlock_lock(&__names_lock);

	struct _mpc_lowcomm_monitor_name_s *cur = __names;

	while(cur)
	{
		struct _mpc_lowcomm_monitor_name_s *to_free = cur;
		cur = cur->prev;
		__name_free(to_free);
	}
}

/********************
* INIT AND RELEASE *
********************/

void _mpc_lowcomm_monitor_name_init(void)
{
	__names = NULL;
}

void _mpc_lowcomm_monitor_name_release(void)
{
	__name_release();
}

/***********
* PUBLISH *
***********/

int _mpc_lowcomm_monitor_name_publish(char *name, char * port_name,  mpc_lowcomm_peer_uid_t hosting_peer)
{
	/* First make sure the name is not already here */
	struct _mpc_lowcomm_monitor_name_s *prev = __name_get(name);

	if(prev)
	{
		return 1;
	}

	prev = __name_new(name, port_name, hosting_peer);

	if(__name_push(prev) )
	{
		/* We were raced on the same name */
		__name_free(prev);
		return 1;
	}

	return 0;
}

int _mpc_lowcomm_monitor_name_unpublish(char *name)
{
	return __name_drop(name);
}

/***********
* RESOLVE *
***********/

char * _mpc_lowcomm_monitor_name_get_port(char *name,  mpc_lowcomm_peer_uid_t *hosting_peer)
{
	struct _mpc_lowcomm_monitor_name_s *n = __name_get(name);

    if(n)
    {
		*hosting_peer = n->hosting_peer;
        return n->port;
    }

    return NULL;
}


/********
* LIST *
********/

char * _mpc_lowcomm_monitor_name_get_csv_list(void)
{
    /* First we need the global string length */

    size_t string_len = 0;

    char * ret = NULL;

	mpc_common_spinlock_lock(&__names_lock);

	struct _mpc_lowcomm_monitor_name_s *cur = __names;

	while(cur)
	{
		string_len += strlen(cur->name) + 2; /* With some margins */
		cur = cur->prev;
	}

    ret = sctk_malloc(string_len + 1);
    ret[0] = '\0';

    assume(ret != NULL);

    cur = __names;

	while(cur)
	{
        strncat(ret, cur->name, string_len);

        if(cur->prev)
        {
            strncat(ret, ",", string_len);
        }

		cur = cur->prev;
	}

	mpc_common_spinlock_unlock(&__names_lock);

    return ret;
}

void _mpc_lowcomm_monitor_name_free_cvs_list(char *list)
{
    sctk_free(list);
}
