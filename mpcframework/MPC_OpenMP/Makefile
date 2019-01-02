# Makefile generated with ../MPC_Tools/mpc_gen_makefile
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

FILES_env=$(wildcard env/*.c)
OBJ_env=$(FILES_env:env/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_env=$(FILES_env:env/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_env=$(wildcard env/*.S)
SOBJ_env=$(SFILES_env:env/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_env=$(SFILES_env:env/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_include=$(wildcard include/*.c)
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_intel=$(wildcard intel/*.c)
OBJ_intel=$(FILES_intel:intel/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_intel=$(FILES_intel:intel/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_intel=$(wildcard intel/*.S)
SOBJ_intel=$(SFILES_intel:intel/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_intel=$(SFILES_intel:intel/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_mpcomp_GOMP_include=$(wildcard mpcomp_GOMP_include/*.c)
OBJ_mpcomp_GOMP_include=$(FILES_mpcomp_GOMP_include:mpcomp_GOMP_include/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_mpcomp_GOMP_include=$(FILES_mpcomp_GOMP_include:mpcomp_GOMP_include/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_mpcomp_GOMP_include=$(wildcard mpcomp_GOMP_include/*.S)
SOBJ_mpcomp_GOMP_include=$(SFILES_mpcomp_GOMP_include:mpcomp_GOMP_include/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_mpcomp_GOMP_include=$(SFILES_mpcomp_GOMP_include:mpcomp_GOMP_include/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_mpcomp_GOMP_internal=$(wildcard mpcomp_GOMP_internal/*.c)
OBJ_mpcomp_GOMP_internal=$(FILES_mpcomp_GOMP_internal:mpcomp_GOMP_internal/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_mpcomp_GOMP_internal=$(FILES_mpcomp_GOMP_internal:mpcomp_GOMP_internal/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_mpcomp_GOMP_internal=$(wildcard mpcomp_GOMP_internal/*.S)
SOBJ_mpcomp_GOMP_internal=$(SFILES_mpcomp_GOMP_internal:mpcomp_GOMP_internal/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_mpcomp_GOMP_internal=$(SFILES_mpcomp_GOMP_internal:mpcomp_GOMP_internal/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_mpcomp_GOMP_wrapper=$(wildcard mpcomp_GOMP_wrapper/*.c)
OBJ_mpcomp_GOMP_wrapper=$(FILES_mpcomp_GOMP_wrapper:mpcomp_GOMP_wrapper/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_mpcomp_GOMP_wrapper=$(FILES_mpcomp_GOMP_wrapper:mpcomp_GOMP_wrapper/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_mpcomp_GOMP_wrapper=$(wildcard mpcomp_GOMP_wrapper/*.S)
SOBJ_mpcomp_GOMP_wrapper=$(SFILES_mpcomp_GOMP_wrapper:mpcomp_GOMP_wrapper/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_mpcomp_GOMP_wrapper=$(SFILES_mpcomp_GOMP_wrapper:mpcomp_GOMP_wrapper/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_mpcomp_KMP=$(wildcard mpcomp_KMP/*.c)
OBJ_mpcomp_KMP=$(FILES_mpcomp_KMP:mpcomp_KMP/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_mpcomp_KMP=$(FILES_mpcomp_KMP:mpcomp_KMP/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_mpcomp_KMP=$(wildcard mpcomp_KMP/*.S)
SOBJ_mpcomp_KMP=$(SFILES_mpcomp_KMP:mpcomp_KMP/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_mpcomp_KMP=$(SFILES_mpcomp_KMP:mpcomp_KMP/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_ompt=$(wildcard ompt/*.c)
OBJ_ompt=$(FILES_ompt:ompt/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_ompt=$(FILES_ompt:ompt/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_ompt=$(wildcard ompt/*.S)
SOBJ_ompt=$(SFILES_ompt:ompt/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_ompt=$(SFILES_ompt:ompt/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FILES_task=$(wildcard task/*.c)
OBJ_task=$(FILES_task:task/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_task=$(FILES_task:task/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_task=$(wildcard task/*.S)
SOBJ_task=$(SFILES_task:task/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_task=$(SFILES_task:task/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


FFILES_env=$(wildcard env/*.f)
FOBJ_env=$(FFILES_env:env/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_intel=$(wildcard intel/*.f)
FOBJ_intel=$(FFILES_intel:intel/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_mpcomp_GOMP_include=$(wildcard mpcomp_GOMP_include/*.f)
FOBJ_mpcomp_GOMP_include=$(FFILES_mpcomp_GOMP_include:mpcomp_GOMP_include/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_mpcomp_GOMP_internal=$(wildcard mpcomp_GOMP_internal/*.f)
FOBJ_mpcomp_GOMP_internal=$(FFILES_mpcomp_GOMP_internal:mpcomp_GOMP_internal/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_mpcomp_GOMP_wrapper=$(wildcard mpcomp_GOMP_wrapper/*.f)
FOBJ_mpcomp_GOMP_wrapper=$(FFILES_mpcomp_GOMP_wrapper:mpcomp_GOMP_wrapper/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_mpcomp_KMP=$(wildcard mpcomp_KMP/*.f)
FOBJ_mpcomp_KMP=$(FFILES_mpcomp_KMP:mpcomp_KMP/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_ompt=$(wildcard ompt/*.f)
FOBJ_ompt=$(FFILES_ompt:ompt/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

FFILES_task=$(wildcard task/*.f)
FOBJ_task=$(FFILES_task:task/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

all: $(OBJ_env) $(FOBJ_env) $(SOBJ_env) $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_intel) $(FOBJ_intel) $(SOBJ_intel) $(OBJ_mpcomp_GOMP_include) $(FOBJ_mpcomp_GOMP_include) $(SOBJ_mpcomp_GOMP_include) $(OBJ_mpcomp_GOMP_internal) $(FOBJ_mpcomp_GOMP_internal) $(SOBJ_mpcomp_GOMP_internal) $(OBJ_mpcomp_GOMP_wrapper) $(FOBJ_mpcomp_GOMP_wrapper) $(SOBJ_mpcomp_GOMP_wrapper) $(OBJ_mpcomp_KMP) $(FOBJ_mpcomp_KMP) $(SOBJ_mpcomp_KMP) $(OBJ_ompt) $(FOBJ_ompt) $(SOBJ_ompt) $(OBJ_src) $(FOBJ_src) $(SOBJ_src) $(OBJ_task) $(FOBJ_task) $(SOBJ_task)

$(M_PWD)/include_modules/MPC_OpenMP:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/env $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/intel $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_GOMP_include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_GOMP_internal $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_GOMP_wrapper $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_KMP $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/ompt $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/src $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/task $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_OpenMP


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_env) $(DEP_include) $(DEP_intel) $(DEP_mpcomp_GOMP_include) $(DEP_mpcomp_GOMP_internal) $(DEP_mpcomp_GOMP_wrapper) $(DEP_mpcomp_KMP) $(DEP_ompt) $(DEP_src) $(DEP_task) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_env):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:env/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,env/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:intel/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_internal/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_wrapper/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_KMP):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_KMP/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_KMP/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_ompt):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:ompt/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,ompt/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_task):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:task/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,task/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_env):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:env/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,env/%.c,$@)" > $@

$(DEP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.c,$@)" > $@

$(DEP_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:intel/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.c,$@)" > $@

$(DEP_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.c,$@)" > $@

$(DEP_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_internal/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.c,$@)" > $@

$(DEP_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_wrapper/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.c,$@)" > $@

$(DEP_mpcomp_KMP):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_KMP/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_KMP/%.c,$@)" > $@

$(DEP_ompt):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:ompt/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,ompt/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.c,$@)" > $@

$(DEP_task):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:task/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,task/%.c,$@)" > $@

endif

$(OBJ_env):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/env/%.c,$@) -o $@

$(OBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/include/%.c,$@) -o $@

$(OBJ_intel):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/intel/%.c,$@) -o $@

$(OBJ_mpcomp_GOMP_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_GOMP_include/%.c,$@) -o $@

$(OBJ_mpcomp_GOMP_internal):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_GOMP_internal/%.c,$@) -o $@

$(OBJ_mpcomp_GOMP_wrapper):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_GOMP_wrapper/%.c,$@) -o $@

$(OBJ_mpcomp_KMP):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_KMP/%.c,$@) -o $@

$(OBJ_ompt):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/ompt/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/src/%.c,$@) -o $@

$(OBJ_task):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/task/%.c,$@) -o $@

$(SDEP_env):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:env/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,env/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:include/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:intel/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_include/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_internal/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_wrapper/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_KMP):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_KMP/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_KMP/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_ompt):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:ompt/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,ompt/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:src/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_task):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:task/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,task/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_env):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/env/%.S,$@) -o $@

$(SOBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/include/%.S,$@) -o $@

$(SOBJ_intel):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/intel/%.S,$@) -o $@

$(SOBJ_mpcomp_GOMP_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_GOMP_include/%.S,$@) -o $@

$(SOBJ_mpcomp_GOMP_internal):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_GOMP_internal/%.S,$@) -o $@

$(SOBJ_mpcomp_GOMP_wrapper):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_GOMP_wrapper/%.S,$@) -o $@

$(SOBJ_mpcomp_KMP):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/mpcomp_KMP/%.S,$@) -o $@

$(SOBJ_ompt):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/ompt/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/src/%.S,$@) -o $@

$(SOBJ_task):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,$(shell pwd)/task/%.S,$@) -o $@

$(FOBJ_env):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:env/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,env/%.f,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:include/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:intel/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,intel/%.f,$@) -o $@

$(FOBJ_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_GOMP_include/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_include/%.f,$@) -o $@

$(FOBJ_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_GOMP_internal/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_internal/%.f,$@) -o $@

$(FOBJ_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_GOMP_wrapper/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_wrapper/%.f,$@) -o $@

$(FOBJ_mpcomp_KMP):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_KMP/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_KMP/%.f,$@) -o $@

$(FOBJ_ompt):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:ompt/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,ompt/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:src/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,src/%.f,$@) -o $@

$(FOBJ_task):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:task/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,task/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
