#ifndef __SCTK_SHM_RAW_QUEUES_ARCHDEFS_H__
#define __SCTK_SHM_RAW_QUEUES_ARCHDEFS_H__

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif /* PAGE_SIZE */

/**
 * Get offset of a member
 */
#define sctk_shm_ll_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * Casts a member of a structure out to the containing structure
 * @param ptr        the pointer to the member.
 * @param type       the type of the container struct this is embedded in.
 * @param member     the name of the member within the struct.
 *
 */

#define sctk_shm_container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - sctk_shm_ll_offsetof(type,member) );})

/* Some distribution haven't container_of */
#ifndef container_of
#define container_of sctk_shm_container_of
#endif

static inline void * 
align(void *ptr, unsigned long boundary)
{
    return (void*)(((unsigned long)ptr + boundary - 1)&(~(boundary - 1)));
}

static inline void *
cacheline_align(void * ptr)
{
	return align(ptr, CACHELINE_SIZE);
}

static inline void *
page_align(void *ptr)
{
	return align(ptr, PAGE_SIZE);
}

static inline void 
cpu_relax(void)
{
    __asm__ __volatile__ ("rep;nop":::"memory");
}

#endif /* __SCTK_SHM_RAW_QUEUES_ARCHDEFS_H__ */
