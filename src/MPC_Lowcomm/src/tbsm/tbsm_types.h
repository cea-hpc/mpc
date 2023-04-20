#ifndef SM_IFACE_H
#define SM_IFACE_H

#include <stddef.h>
#include <stdint.h>

#include <mpc_common_spinlock.h>


typedef struct lcr_tbsm_pkg {
        uint8_t             am_id; /* active message id */
        void               *buf;   /* addr of bcopy buf or send buf */
        size_t              size;
        struct lcr_tbsm_pkg *prev, *next;
} lcr_tbsm_pkg_t;

typedef struct lcr_tbsm_pkg_list {
        lcr_tbsm_pkg_t *list;
        mpc_common_spinlock_t lock; 
        size_t size;
} lcr_tbsm_pkg_list_t;

typedef struct lcr_tbsm_iface_info {
        lcr_tbsm_pkg_list_t *list;
} lcr_tbsm_iface_info_t;

#endif
