#ifndef MPC_CONTEXT_REGISTERS_X86_64
#define MPC_CONTEXT_REGISTERS_X86_64

/* X86_64 Support */

/*
.global mpc__setjmp
.type mpc__setjmp,@function;
.align 4;

*/

/************************
 * REGISTER DEFINITIONS *
 ************************/

#  define cfi_startproc                 .cfi_startproc
#  define cfi_endproc                   .cfi_endproc
#  define cfi_def_cfa(reg, off)         .cfi_def_cfa reg, off
#  define cfi_def_cfa_register(reg)     .cfi_def_cfa_register reg
#  define cfi_def_cfa_offset(off)       .cfi_def_cfa_offset off
#  define cfi_adjust_cfa_offset(off)    .cfi_adjust_cfa_offset off
#  define cfi_offset(reg, off)          .cfi_offset reg, off
#  define cfi_rel_offset(reg, off)      .cfi_rel_offset reg, off
#  define cfi_register(r1, r2)          .cfi_register r1, r2
#  define cfi_return_column(reg)        .cfi_return_column reg
#  define cfi_restore(reg)              .cfi_restore reg
#  define cfi_same_value(reg)           .cfi_same_value reg
#  define cfi_undefined(reg)            .cfi_undefined reg
#  define cfi_remember_state            .cfi_remember_state
#  define cfi_restore_state             .cfi_restore_state
#  define cfi_window_save               .cfi_window_save
#  define cfi_personality(enc, exp)     .cfi_personality enc, exp
#  define cfi_lsda(enc, exp)            .cfi_lsda enc, exp

#define SCTK_REG_RAX 0
#define SCTK_REG_RBX 1
#define SCTK_REG_RCX 2
#define SCTK_REG_RDX 3
#define SCTK_REG_RSI 4
#define SCTK_REG_RDI 5
#define SCTK_REG_RBP 6
#define SCTK_REG_R8 7
#define SCTK_REG_R9 8
#define SCTK_REG_R10 9
#define SCTK_REG_R11 10
#define SCTK_REG_R12 11
#define SCTK_REG_R13 12
#define SCTK_REG_R14 13
#define SCTK_REG_R15 14
#define SCTK_REG_RSP 15
#define SCTK_REG_RIP 16
#define SCTK_REG_EFL 17

/********************
 * REGISTER OFFSETS *
 ********************/

#define SCTK_oRAX 0x28
#define SCTK_oRBX 0x30
#define SCTK_oRCX 0x38
#define SCTK_oRDX 0x40
#define SCTK_oRSI 0x48
#define SCTK_oRDI 0x50
#define SCTK_oRBP 0x58
#define SCTK_oR8 0x60
#define SCTK_oR9 0x68
#define SCTK_oR10 0x70
#define SCTK_oR11 0x78
#define SCTK_oR12 0x80
#define SCTK_oR13 0x88
#define SCTK_oR14 0x90
#define SCTK_oR15 0x98
#define SCTK_oRSP 0xa0
#define SCTK_oRIP 0xa8
#define SCTK_oEFL 0xb0
#define SCTK_oFPREGS 0xe0
#define SCTK_oFPREGSMEM 0x1a8
#define SCTK_oMXCSR 0x1c0

#endif /* MPC_CONTEXT_REGISTERS_X86_64 */
