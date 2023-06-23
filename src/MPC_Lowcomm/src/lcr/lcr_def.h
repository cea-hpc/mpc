#ifndef LCR_DEF_H
#define LCR_DEF_H

#include <stdint.h>
#include <stddef.h>

//FIXME: to be changed
#define LCR_AM_ID_MAX 64
#define LCR_COMPONENT_NAME_MAX 16
#define LCR_DEVICE_NAME_MAX 64
#define LCR_DEVICE_MAX 64

typedef struct _mpc_lowcomm_endpoint_s _mpc_lowcomm_endpoint_t;
typedef struct sctk_rail_info_s        sctk_rail_info_t;
typedef struct sctk_rail_pin_ctx_list  lcr_memp_t;
typedef struct lcr_tag_context         lcr_tag_context_t;
typedef struct lcr_completion          lcr_completion_t;
typedef struct lcr_rail_attr           lcr_rail_attr_t;
typedef struct lcr_device              lcr_device_t;

typedef struct lcr_component          *lcr_component_h;

typedef union {
	struct {
		uint64_t comm:16;
		uint64_t tag:24;
		uint64_t src:24;
	} t_tag;
	uint64_t t;
} lcr_tag_t;

typedef int (*lcr_am_callback_t)(void *arg, void *data, size_t length,
				 unsigned flags);

typedef size_t (*lcr_pack_callback_t)(void *dest, void *data);

typedef size_t (*lcr_unpack_callback_t)(void *arg, const void *data, size_t length);

typedef void (*lcr_completion_callback_t)(lcr_completion_t *self);

typedef int (*lcr_tag_completion_callback_t)(lcr_tag_context_t *ctx, unsigned flags);

#endif
