#ifndef __MPCOMP_OMPT_GENERAL_H__
#define __MPCOMP_OMPT_GENERAL_H__

#if OMPT_SUPPORT

#include "ompt.h"

int mpcomp_ompt_is_enabled(void);
ompt_callback_t mpcomp_ompt_get_callback(int);
void mpcomp_ompt_post_init(void);

#endif /* OMPT_SUPPORT */

#endif /* __MPCOMP_OMPT_GENERAL_H__ */
