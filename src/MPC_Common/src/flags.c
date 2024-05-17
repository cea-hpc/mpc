#include <mpc_common_flags.h>
#include <mpc_common_spinlock.h>

#include "mpc_common_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct mpc_common_flags ___mpc_flags = { 0 };

OPA_int_t __mpc_p_disguise_flag;

struct mpc_common_init_entry
{
        char name[64];
        int priority;
        void (*callback)();
};

static inline void __entry_init(struct mpc_common_init_entry * entry, char * callback_name, void (*callback)(), int priority )
{
        snprintf(entry->name, 64, "%s", callback_name);
        entry->priority = priority;
        entry->callback = callback;
}



static int __entry_sort(const void *pa, const void *pb)
{
        struct mpc_common_init_entry *a = (struct mpc_common_init_entry *)pa;
        struct mpc_common_init_entry *b = (struct mpc_common_init_entry *)pb;

        return a->priority - b->priority;
}


#define MAX_INIT_ENTRY 32

struct mpc_common_init_list
{
        char name[32];
        struct mpc_common_init_entry entries[MAX_INIT_ENTRY];
        unsigned int entry_count;
};

static inline  void __list_trigger(struct mpc_common_init_list * list)
{
        unsigned int i;

        for( i = 0 ; i < list->entry_count; i++)
        {
                if( ___mpc_flags.debug_callbacks )
                {
                        mpc_common_debug_log("INIT : Triggering %s - %s (%d)", list->name, list->entries[i].name, list->entries[i].priority);
                }

                if(list->entries[i].callback)
                {
                        (list->entries[i].callback)();
                }
        }
}

static inline void __list_add_callback(struct mpc_common_init_list * list, char * callback_name, void (*callback)(), int priority )
{
        if(list->entry_count == MAX_INIT_ENTRY)
        {
                mpc_common_debug_fatal("Maximum number of entries reached when adding callback '%s' to '%s'", callback_name, list->name );
        }


        struct mpc_common_init_entry *entry = &list->entries[ list->entry_count ];
        list->entry_count++;

        __entry_init(entry, callback_name, callback, priority);

        //Sort callbacks
        qsort(list->entries, list->entry_count, sizeof(struct mpc_common_init_entry), __entry_sort);
}


#define MAX_INIT_LIST 16

struct mpc_common_init
{
        mpc_common_spinlock_t lock;
        struct mpc_common_init_list lists[MAX_INIT_LIST];
        unsigned int list_count;
};

static struct mpc_common_init __mpc_init = { 0 };


static inline struct mpc_common_init_list * _init_get_list(char * level_name)
{
        unsigned int i;

        struct mpc_common_init_list *ret = NULL;

        mpc_common_spinlock_lock(&__mpc_init.lock);

        for( i = 0 ; i < __mpc_init.list_count; i++)
        {
                if(!strcmp(__mpc_init.lists[i].name, level_name))
                {
                        ret = &__mpc_init.lists[i];
                        break;
                }
        }

        mpc_common_spinlock_unlock(&__mpc_init.lock);

        return ret;
}

void mpc_common_init_trigger(char * level_name)
{
        struct mpc_common_init_list * list = _init_get_list(level_name);

        if( list )
        {
                __list_trigger(list);
        }
}

void mpc_common_init_list_register(char * list_name)
{
        struct mpc_common_init_list * list = _init_get_list(list_name);

        if( list )
        {
                return;
        }

        mpc_common_spinlock_lock(&__mpc_init.lock);

        if(__mpc_init.list_count == MAX_INIT_LIST)
        {
                mpc_common_debug_fatal("Cannot create more than %d Initialization lists", MAX_INIT_LIST);
        }

        list = &__mpc_init.lists[ __mpc_init.list_count ];
        snprintf(list->name, 32, "%s", list_name);
        __mpc_init.list_count++;

        mpc_common_spinlock_unlock(&__mpc_init.lock);
}

void mpc_common_init_callback_register(char * list_name, char * callback_name, void (*callback)(), int priority )
{
        mpc_common_init_list_register(list_name);


        struct mpc_common_init_list * list = _init_get_list(list_name);

        if(!list)
        {
                mpc_common_debug_fatal("Cannot register callback '%s' to non-existent list '%s'", callback_name, list_name);
        }

        __list_add_callback(list, callback_name, callback, priority);
}

void mpc_common_init_print()
{
        unsigned int i, j;

        for( i = 0 ; i < __mpc_init.list_count ; i++ )
        {
                struct mpc_common_init_list * list = &__mpc_init.lists[i];
                mpc_common_debug_log("INIT List %s --------", list->name);

                for( j = 0; j < list->entry_count ; j++ )
                {
                        struct mpc_common_init_entry * entry = &list->entries[j];
                        mpc_common_debug_log("\t(%d/%d) Callback %s (prio %d) (ptr %p)", j+1,
                                                                                      list->entry_count,
                                                                                      entry->name,
                                                                                      entry->priority,
                                                                                      entry->callback);
                }

                mpc_common_debug_log("-------------------");
        }
}
