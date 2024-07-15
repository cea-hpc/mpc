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

#endif