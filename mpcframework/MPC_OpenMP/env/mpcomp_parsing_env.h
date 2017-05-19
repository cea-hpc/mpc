#ifndef __MPCOMP_PARSING_ENV_H__
#define __MPCOMP_PARSING_ENV_H__

typedef struct mpcomp_places_info_s
{
    unsigned int id;
    hwloc_bitmap_t interval;
    unsigned int relative_depth;
    struct mpcomp_places_info_s *prev, *next;
} mpcomp_places_info_t; 

void mpcomp_display_places( mpcomp_places_info_t* list );
mpcomp_places_info_t* mpcomp_places_env_variable_parsing(void);

#endif /*  __MPCOMP_PARSING_ENV_H__ */
