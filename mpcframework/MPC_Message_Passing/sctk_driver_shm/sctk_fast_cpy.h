#pragma once
//#include <helpers/archdefs.h>
#include <string.h>

/* Memcpy routines from MPICH2-NEMESIS */
#define NT_LEN 2*1024

static inline void amd64_cpy_nt (volatile void *dst, volatile void *src, size_t n)
{
    size_t n32 = (n) >> 5;
    size_t nleft = (n) & (32-1);

    if (n32){
        __asm__ __volatile__ (".align 16  \n"
                              "1:  \n"
                              "mov (%1), %%r8  \n"
                              "mov 8(%1), %%r9  \n"
                              "add $32, %1  \n"
                              "movnti %%r8, (%2)  \n"
                              "movnti %%r9, 8(%2)  \n"
                              "add $32, %2  \n"
                              "mov -16(%1), %%r8  \n"
                              "mov -8(%1), %%r9  \n"
                              "dec %0  \n"
                              "movnti %%r8, -16(%2)  \n"
                              "movnti %%r9, -8(%2)  \n"
                              "jnz 1b  \n"
                              "sfence  \n"
                              "mfence  \n"
                              : "+a" (n32), "+S" (src), "+D" (dst)
                              : : "r8", "r9" /*, "memory" is this needed? */);
    }


    if (nleft){
        memcpy ((void *)dst, (void *)src, nleft);
    }
}

static inline void sse_memcpy_nt(void * dst, void * src, unsigned long size)
{
    unsigned long size_sse;

    if((unsigned long)dst & (15)){
        int unaligned = 16 - ((unsigned long)dst & 15);
        memcpy(dst, src, unaligned);
        dst += unaligned;
        src += unaligned;
        size -= unaligned;
    }

    size_sse = size &(~(unsigned long)127);
    if(size_sse){
        __asm__ __volatile__(
            ".align 16 \n"
            "movq %%rax,%%r8\n"
            "1: \n"
            "leaq 0x40(%%rcx),%%rcx \n"
            "leaq 0x40(%%rdx), %%rdx\n"
            "leaq -0x40(%%r8), %%r8\n"
            "prefetchnta 0x180(%%rdx)\n"
            "movdqu      -0x40(%%rdx), %%xmm0 \n"
            "movdqu      -0x30(%%rdx), %%xmm1 \n"
            "movdqu      -0x20(%%rdx), %%xmm2 \n"
            "movdqu      -0x10(%%rdx), %%xmm3 \n"
            "cmpq         $0x40,%%r8\n"
            "movntdq     %%xmm0, -0x40(%%rcx) \n"
	    "movntdq     %%xmm1, -0x30(%%rcx) \n"
            "movntdq     %%xmm2, -0x20(%%rcx) \n"
            "movntdq     %%xmm3, -0x10(%%rcx) \n"
            "jge         1b\n"
            "sfence \n"
            : "+a" (size_sse), "+c" (dst), "+d" (src) ::"r8");
    }

    memcpy(dst, src, size-size_sse);
}

static inline void ll_memcpy(void *dst, void *src, size_t n)
{
    int i;
    char * d, * s;
    d = dst;
    s = src;
    for(i = 0; i < n; ++i){
        d[i] = s[i];
    }
}

#define asm_memcpy(dst, src, n) ({                                      \
            char *p = (char *)(src);                                    \
            char *q = (char *)(dst);                                    \
            size_t nq = (n) >> 3;                                       \
            __asm__ __volatile__ ("cld ; rep ; movsq ; movl %3,%%ecx ; rep ; movsb" \
                                  : "+c" (nq), "+S" (p), "+D" (q)       \
                                  : "r" ((uint32_t)((n) & 7)) /* : "memory" is this needed? */); \
            (void *)(dst);                                              \
        })

#if defined(_M_X64) || defined(__amd64__)
#define FAST_MEMCPY(a,b,c)(c)>NT_LEN?sse_memcpy_nt(a,b,c):(c)>32?asm_memcpy(a,b,c):ll_memcpy(a,b,c)
#define FAST_MEMCPY_REUSE(a,b,c)(c)>32?asm_memcpy(a,b,c):ll_memcpy(a,b,c)
#else
#warning "DON'T USE FAST MEMCPY"
#define FAST_MEMCPY(a,b,c) memcpy(a,b,c)
#define FAST_MEMCPY_REUSE(a,b,c) memcpy(a,b,c)
#endif

