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


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d)


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

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_intel) $(FOBJ_intel) $(SOBJ_intel) $(OBJ_mpcomp_GOMP_include) $(FOBJ_mpcomp_GOMP_include) $(SOBJ_mpcomp_GOMP_include) $(OBJ_mpcomp_GOMP_internal) $(FOBJ_mpcomp_GOMP_internal) $(SOBJ_mpcomp_GOMP_internal) $(OBJ_mpcomp_GOMP_wrapper) $(FOBJ_mpcomp_GOMP_wrapper) $(SOBJ_mpcomp_GOMP_wrapper) $(OBJ_src) $(FOBJ_src) $(SOBJ_src)

$(M_PWD)/include_modules/MPC_OpenMP:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/intel $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_GOMP_include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_GOMP_internal $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/mpcomp_GOMP_wrapper $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_OpenMP/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_OpenMP


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_intel) $(DEP_mpcomp_GOMP_include) $(DEP_mpcomp_GOMP_internal) $(DEP_mpcomp_GOMP_wrapper) $(DEP_src) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:intel/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_internal/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_wrapper/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:src/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.c,$@)" > $@

$(DEP_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:intel/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.c,$@)" > $@

$(DEP_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.c,$@)" > $@

$(DEP_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_internal/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.c,$@)" > $@

$(DEP_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_wrapper/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:src/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,include/%.c,$@) -o $@

$(OBJ_intel):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,intel/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,intel/%.c,$@) -o $@

$(OBJ_mpcomp_GOMP_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_include/%.c,$@) -o $@

$(OBJ_mpcomp_GOMP_internal):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_internal/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_internal/%.c,$@) -o $@

$(OBJ_mpcomp_GOMP_wrapper):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_wrapper/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_wrapper/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,src/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,src/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:intel/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,intel/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_internal/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_internal/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:mpcomp_GOMP_wrapper/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,mpcomp_GOMP_wrapper/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%.d:src/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,include/%.S,$@) -o $@

$(SOBJ_intel):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,intel/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,intel/%.S,$@) -o $@

$(SOBJ_mpcomp_GOMP_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_include/%.S,$@) -o $@

$(SOBJ_mpcomp_GOMP_internal):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_internal/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_internal/%.S,$@) -o $@

$(SOBJ_mpcomp_GOMP_wrapper):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_wrapper/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,mpcomp_GOMP_wrapper/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,src/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%.o,src/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_intel):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:intel/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,intel/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,intel/%.f,$@) -o $@

$(FOBJ_mpcomp_GOMP_include):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_GOMP_include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_include/%.f,$@) -o $@

$(FOBJ_mpcomp_GOMP_internal):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_GOMP_internal/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_internal/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_internal/%.f,$@) -o $@

$(FOBJ_mpcomp_GOMP_wrapper):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:mpcomp_GOMP_wrapper/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_wrapper/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,mpcomp_GOMP_wrapper/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o:src/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,src/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_OpenMP/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
