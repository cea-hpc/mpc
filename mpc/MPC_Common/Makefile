# Makefile generated with ../MPC_Tools/mpc_gen_makefile Fri Oct 28 11:19:50 CEST 2011
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#                                                                      #
########################################################################


LOCAL_CFLAGS=$(CFLAGS)

FILES_alloc=$(wildcard sctk_alloc/*.c)
OBJ_alloc=$(FILES_alloc:sctk_alloc/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_alloc=$(FILES_alloc:sctk_alloc/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_alloc=$(wildcard sctk_alloc/*.S)
SOBJ_alloc=$(SFILES_alloc:sctk_alloc/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_alloc=$(SFILES_alloc:sctk_alloc/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_asm=$(wildcard sctk_asm/*.c)
OBJ_asm=$(FILES_asm:sctk_asm/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_asm=$(FILES_asm:sctk_asm/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_asm=$(wildcard sctk_asm/*.S)
SOBJ_asm=$(SFILES_asm:sctk_asm/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_asm=$(SFILES_asm:sctk_asm/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_datastructures=$(wildcard sctk_datastructures/*.c)
OBJ_datastructures=$(FILES_datastructures:sctk_datastructures/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_datastructures=$(FILES_datastructures:sctk_datastructures/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_datastructures=$(wildcard sctk_datastructures/*.S)
SOBJ_datastructures=$(SFILES_datastructures:sctk_datastructures/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_datastructures=$(SFILES_datastructures:sctk_datastructures/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_debug=$(wildcard sctk_debug/*.c)
OBJ_debug=$(FILES_debug:sctk_debug/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_debug=$(FILES_debug:sctk_debug/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_debug=$(wildcard sctk_debug/*.S)
SOBJ_debug=$(SFILES_debug:sctk_debug/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_debug=$(SFILES_debug:sctk_debug/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_helpers=$(wildcard sctk_helpers/*.c)
OBJ_helpers=$(FILES_helpers:sctk_helpers/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_helpers=$(FILES_helpers:sctk_helpers/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_helpers=$(wildcard sctk_helpers/*.S)
SOBJ_helpers=$(SFILES_helpers:sctk_helpers/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_helpers=$(SFILES_helpers:sctk_helpers/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_launch=$(wildcard sctk_launch/*.c)
OBJ_launch=$(FILES_launch:sctk_launch/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_launch=$(FILES_launch:sctk_launch/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_launch=$(wildcard sctk_launch/*.S)
SOBJ_launch=$(SFILES_launch:sctk_launch/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_launch=$(SFILES_launch:sctk_launch/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_optim=$(wildcard sctk_optim/$(SCTK_ARCH_OPTIM)/*.c)
OBJ_optim=$(FILES_optim:sctk_optim/$(SCTK_ARCH_OPTIM)/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_optim=$(FILES_optim:sctk_optim/$(SCTK_ARCH_OPTIM)/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_optim=$(wildcard sctk_optim/$(SCTK_ARCH_OPTIM)/*.S)
SOBJ_optim=$(SFILES_optim:sctk_optim/$(SCTK_ARCH_OPTIM)/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_optim=$(SFILES_optim:sctk_optim/$(SCTK_ARCH_OPTIM)/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_options=$(wildcard sctk_options/*.c)
OBJ_options=$(FILES_options:sctk_options/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_options=$(FILES_options:sctk_options/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_options=$(wildcard sctk_options/*.S)
SOBJ_options=$(SFILES_options:sctk_options/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_options=$(SFILES_options:sctk_options/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FILES_spinlock=$(wildcard sctk_spinlock/*.c)
OBJ_spinlock=$(FILES_spinlock:sctk_spinlock/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
DEP_spinlock=$(FILES_spinlock:sctk_spinlock/%.c=$(M_PWD)/BUILD_MPC_Common/lib/%.d)

SFILES_spinlock=$(wildcard sctk_spinlock/*.S)
SOBJ_spinlock=$(SFILES_spinlock:sctk_spinlock/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.o)
SDEP_spinlock=$(SFILES_spinlock:sctk_spinlock/%.S=$(M_PWD)/BUILD_MPC_Common/lib/%.d)


FFILES_alloc=$(wildcard sctk_alloc/*.f)
FOBJ_alloc=$(FFILES_alloc:sctk_alloc/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_asm=$(wildcard sctk_asm/*.f)
FOBJ_asm=$(FFILES_asm:sctk_asm/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_datastructures=$(wildcard sctk_datastructures/*.f)
FOBJ_datastructures=$(FFILES_datastructures:sctk_datastructures/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_debug=$(wildcard sctk_debug/*.f)
FOBJ_debug=$(FFILES_debug:sctk_debug/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_helpers=$(wildcard sctk_helpers/*.f)
FOBJ_helpers=$(FFILES_helpers:sctk_helpers/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_launch=$(wildcard sctk_launch/*.f)
FOBJ_launch=$(FFILES_launch:sctk_launch/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_optim=$(wildcard sctk_optim/$(SCTK_ARCH_OPTIM)/*.f)
FOBJ_optim=$(FFILES_optim:sctk_optim/$(SCTK_ARCH_OPTIM)/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_options=$(wildcard sctk_options/*.f)
FOBJ_options=$(FFILES_options:sctk_options/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

FFILES_spinlock=$(wildcard sctk_spinlock/*.f)
FOBJ_spinlock=$(FFILES_spinlock:sctk_spinlock/%.f=$(M_PWD)/BUILD_MPC_Common/lib/%_f.o)

all: $(OBJ_alloc) $(FOBJ_alloc) $(SOBJ_alloc) $(OBJ_asm) $(FOBJ_asm) $(SOBJ_asm) $(OBJ_datastructures) $(FOBJ_datastructures) $(SOBJ_datastructures) $(OBJ_debug) $(FOBJ_debug) $(SOBJ_debug) $(OBJ_helpers) $(FOBJ_helpers) $(SOBJ_helpers) $(OBJ_launch) $(FOBJ_launch) $(SOBJ_launch) $(OBJ_optim) $(FOBJ_optim) $(SOBJ_optim) $(OBJ_options) $(FOBJ_options) $(SOBJ_options) $(OBJ_spinlock) $(FOBJ_spinlock) $(SOBJ_spinlock)

$(M_PWD)/include_modules/MPC_Common:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_alloc $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_asm $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_datastructures $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_debug $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_helpers $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_launch $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_optim/$(SCTK_ARCH_OPTIM) $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_options $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Common/sctk_spinlock $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Common


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_alloc) $(DEP_asm) $(DEP_datastructures) $(DEP_debug) $(DEP_helpers) $(DEP_launch) $(DEP_optim) $(DEP_options) $(DEP_spinlock) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_alloc):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_alloc/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_alloc/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_alloc/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_asm):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_asm/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_asm/%.c,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_asm/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_datastructures):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_datastructures/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_datastructures/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_datastructures/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_debug):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_debug/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_debug/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_debug/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_helpers):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_helpers/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_helpers/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_helpers/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_launch):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_launch/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_launch/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_launch/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_optim):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_optim/$(SCTK_ARCH_OPTIM)/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_optim/$(SCTK_ARCH_OPTIM)/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_optim/$(SCTK_ARCH_OPTIM)/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_options):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_options/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_options/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_options/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_spinlock):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_spinlock/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_spinlock/%.c,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_spinlock/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_alloc):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_alloc/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_alloc/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_alloc/%.c,$@)" > $@

$(DEP_asm):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_asm/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_asm/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_asm/%.c,$@)" > $@

$(DEP_datastructures):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_datastructures/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_datastructures/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_datastructures/%.c,$@)" > $@

$(DEP_debug):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_debug/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_debug/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_debug/%.c,$@)" > $@

$(DEP_helpers):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_helpers/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_helpers/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_helpers/%.c,$@)" > $@

$(DEP_launch):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_launch/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_launch/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_launch/%.c,$@)" > $@

$(DEP_optim):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_optim/$(SCTK_ARCH_OPTIM)/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_optim/$(SCTK_ARCH_OPTIM)/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_optim/$(SCTK_ARCH_OPTIM)/%.c,$@)" > $@

$(DEP_options):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_options/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_options/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_options/%.c,$@)" > $@

$(DEP_spinlock):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_spinlock/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_spinlock/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_spinlock/%.c,$@)" > $@

endif

$(OBJ_alloc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_alloc/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_alloc/%.c,$@) -o $@

$(OBJ_asm):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_asm/%.c,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_asm/%.c,$@) -o $@

$(OBJ_datastructures):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_datastructures/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_datastructures/%.c,$@) -o $@

$(OBJ_debug):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_debug/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_debug/%.c,$@) -o $@

$(OBJ_helpers):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_helpers/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_helpers/%.c,$@) -o $@

$(OBJ_launch):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_launch/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_launch/%.c,$@) -o $@

$(OBJ_optim):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_optim/$(SCTK_ARCH_OPTIM)/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_optim/$(SCTK_ARCH_OPTIM)/%.c,$@) -o $@

$(OBJ_options):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_options/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_options/%.c,$@) -o $@

$(OBJ_spinlock):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_spinlock/%.c,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_spinlock/%.c,$@) -o $@

$(SDEP_alloc):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_alloc/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_alloc/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_alloc/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_asm):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_asm/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_asm/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_asm/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_datastructures):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_datastructures/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_datastructures/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_datastructures/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_debug):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_debug/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_debug/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_debug/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_helpers):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_helpers/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_helpers/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_helpers/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_launch):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_launch/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_launch/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_launch/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_optim):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_optim/$(SCTK_ARCH_OPTIM)/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_optim/$(SCTK_ARCH_OPTIM)/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_optim/$(SCTK_ARCH_OPTIM)/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_options):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_options/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_options/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_options/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_spinlock):$(M_PWD)/BUILD_MPC_Common/lib/%.d:sctk_spinlock/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_spinlock/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,sctk_spinlock/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_alloc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_alloc/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_alloc/%.S,$@) -o $@

$(SOBJ_asm):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_asm/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_asm/%.S,$@) -o $@

$(SOBJ_datastructures):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_datastructures/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_datastructures/%.S,$@) -o $@

$(SOBJ_debug):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_debug/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_debug/%.S,$@) -o $@

$(SOBJ_helpers):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_helpers/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_helpers/%.S,$@) -o $@

$(SOBJ_launch):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_launch/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_launch/%.S,$@) -o $@

$(SOBJ_optim):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_optim/$(SCTK_ARCH_OPTIM)/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_optim/$(SCTK_ARCH_OPTIM)/%.S,$@) -o $@

$(SOBJ_options):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_options/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_options/%.S,$@) -o $@

$(SOBJ_spinlock):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_spinlock/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%.o,sctk_spinlock/%.S,$@) -o $@

$(FOBJ_alloc):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_alloc/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_alloc/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_alloc/%.f,$@) -o $@

$(FOBJ_asm):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_asm/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_asm/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_asm/%.f,$@) -o $@

$(FOBJ_datastructures):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_datastructures/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_datastructures/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_datastructures/%.f,$@) -o $@

$(FOBJ_debug):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_debug/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_debug/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_debug/%.f,$@) -o $@

$(FOBJ_helpers):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_helpers/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_helpers/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_helpers/%.f,$@) -o $@

$(FOBJ_launch):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_launch/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_launch/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_launch/%.f,$@) -o $@

$(FOBJ_optim):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_optim/$(SCTK_ARCH_OPTIM)/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_optim/$(SCTK_ARCH_OPTIM)/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_optim/$(SCTK_ARCH_OPTIM)/%.f,$@) -o $@

$(FOBJ_options):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_options/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_options/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_options/%.f,$@) -o $@

$(FOBJ_spinlock):$(M_PWD)/BUILD_MPC_Common/lib/%_f.o:sctk_spinlock/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_spinlock/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Common/lib/%_f.o,sctk_spinlock/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
