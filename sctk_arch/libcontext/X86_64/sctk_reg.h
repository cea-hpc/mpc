/*
.global mpc__setjmp
.type mpc__setjmp,@function;
.align 4;

*/

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

#include <sctk_offset.h>
