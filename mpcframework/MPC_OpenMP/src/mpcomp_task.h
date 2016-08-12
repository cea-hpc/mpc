#if (!defined(__SCTK_MPCOMP_TASK_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_H__

#if KMP_ARCH_X86
# define KMP_SIZE_T_MAX (0xFFFFFFFF)
#else
# define KMP_SIZE_T_MAX (0xFFFFFFFFFFFFFFFF)
#endif

// Round up a size to a power of two specified by val
// Used to insert padding between structures co-allocated using a single malloc() call
static size_t
__kmp_round_up_to_val( size_t size, size_t val ) 
{
    if ( size & ( val - 1 ) ) 
    {
        size &= ~ ( val - 1 );
        if ( size <= KMP_SIZE_T_MAX - val ) 
            size += val;    // Round up if there is no overflow.
    }
    return size;
} // __kmp_round_up_to_va

#endif /* __SCTK_MPCOMP_TASK_H__ */
