#ifndef MPC_COMMON_COMPILER_DEF_H
#define MPC_COMMON_COMPILER_DEF_H

//FIXME: redefine unicity
#define MPC_PP_UNIQUE_ID __LINE__

#define __MPC_TOKENPASTE_HELPER(x, y) x ## y
#define MPC_TOKENPASTE(x, y) __MPC_TOKENPASTE_HELPER(x, y)

#define MPC_APPEND_UNIQUE_ID(x) MPC_TOKENPASTE(x, MPC_PP_UNIQUE_ID)

#define MPC_CTOR __attribute__((constructor))

#define MPC_STATIC_INIT \
	static void MPC_CTOR MPC_APPEND_UNIQUE_ID(mpc_init_ctor)()

#define mpc_container_of(ptr, type, member) \
	({ \
	 const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	 (type *)( (char *)__mptr - offsetof(type,member) ); \
	 })

#define mpc_buffer_offset(_buf, _offset) \
        ((uint64_t) ((uintptr_t)_buf + _offset))

void rand_seed_init(void);
uint64_t lcp_rand_uint64(void);

static inline uint64_t lcp_get_process_uid(uint64_t pid, int32_t tid) {
        pid &= 0xFFFFFFFF00000000ull; /* first reset least significant bits to 0 */
        return pid | tid;
}

#endif
