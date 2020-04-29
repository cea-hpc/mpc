AM_CPPFLAGS=-I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/ $(OPENPA_CFLAGS)

lib_LTLIBRARIES = libmpcarch.la

libmpcarch_la_SOURCES = ./pause.c ./timer.c

libmpcarch_la_LIBADD = $(OPENPA_LIBS)

# Optimized Context Switches

if MPC_ARCH_X86_64_CONTEXT_SWITCH
        libmpcarch_la_SOURCES += ./context/x86_64/getcontext.S ./context/x86_64/makecontext.c
        libmpcarch_la_SOURCES += ./context/x86_64/setcontext.S ./context/x86_64/startcontext.S ./context/x86_64/swapcontext.S
endif

# Microstask Support

if MPC_ARCH_X86_64_MICROTASK
        libmpcarch_la_SOURCES += ./microtask/x86_64/z_Linux_asm.S
endif

if MPC_ARCH_GENERIC_MICROTASK
        libmpcarch_la_SOURCES += ./microtask/generic/microtask.c
endif
