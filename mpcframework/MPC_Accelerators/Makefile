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
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)


FILES_cuda=$(wildcard sctk_cuda/*.c)
OBJ_cuda=$(FILES_cuda:sctk_cuda/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
DEP_cuda=$(FILES_cuda:sctk_cuda/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)

SFILES_cuda=$(wildcard sctk_cuda/*.S)
SOBJ_cuda=$(SFILES_cuda:sctk_cuda/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
SDEP_cuda=$(SFILES_cuda:sctk_cuda/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)


FILES_openacc=$(wildcard sctk_openacc/*.c)
OBJ_openacc=$(FILES_openacc:sctk_openacc/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
DEP_openacc=$(FILES_openacc:sctk_openacc/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)

SFILES_openacc=$(wildcard sctk_openacc/*.S)
SOBJ_openacc=$(SFILES_openacc:sctk_openacc/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
SDEP_openacc=$(SFILES_openacc:sctk_openacc/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)


FILES_opencl=$(wildcard sctk_opencl/*.c)
OBJ_opencl=$(FILES_opencl:sctk_opencl/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
DEP_opencl=$(FILES_opencl:sctk_opencl/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)

SFILES_opencl=$(wildcard sctk_opencl/*.S)
SOBJ_opencl=$(SFILES_opencl:sctk_opencl/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
SDEP_opencl=$(SFILES_opencl:sctk_opencl/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d)


FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o)

FFILES_cuda=$(wildcard sctk_cuda/*.f)
FOBJ_cuda=$(FFILES_cuda:sctk_cuda/%.f=$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o)

FFILES_openacc=$(wildcard sctk_openacc/*.f)
FOBJ_openacc=$(FFILES_openacc:sctk_openacc/%.f=$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o)

FFILES_opencl=$(wildcard sctk_opencl/*.f)
FOBJ_opencl=$(FFILES_opencl:sctk_opencl/%.f=$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_cuda) $(FOBJ_cuda) $(SOBJ_cuda) $(OBJ_openacc) $(FOBJ_openacc) $(SOBJ_openacc) $(OBJ_opencl) $(FOBJ_opencl) $(SOBJ_opencl) $(OBJ_src) $(FOBJ_src) $(SOBJ_src)

$(M_PWD)/include_modules/MPC_Accelerators:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Accelerators/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Accelerators/sctk_cuda $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Accelerators/sctk_openacc $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Accelerators/sctk_opencl $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Accelerators/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Accelerators


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_cuda) $(DEP_openacc) $(DEP_opencl) $(DEP_src) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_cuda):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_cuda/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_cuda/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_cuda/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_openacc):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_openacc/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_openacc/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_openacc/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_opencl):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_opencl/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_opencl/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_opencl/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:src/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,src/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,include/%.c,$@)" > $@

$(DEP_cuda):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_cuda/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_cuda/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_cuda/%.c,$@)" > $@

$(DEP_openacc):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_openacc/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_openacc/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_openacc/%.c,$@)" > $@

$(DEP_opencl):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_opencl/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_opencl/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_opencl/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:src/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,src/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,include/%.c,$@) -o $@

$(OBJ_cuda):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_cuda/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_cuda/%.c,$@) -o $@

$(OBJ_openacc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_openacc/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_openacc/%.c,$@) -o $@

$(OBJ_opencl):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_opencl/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_opencl/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,src/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,src/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_cuda):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_cuda/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_cuda/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_cuda/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_openacc):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_openacc/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_openacc/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_openacc/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_opencl):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:sctk_opencl/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_opencl/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,sctk_opencl/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Accelerators/lib/%.d:src/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,src/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,include/%.S,$@) -o $@

$(SOBJ_cuda):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_cuda/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_cuda/%.S,$@) -o $@

$(SOBJ_openacc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_openacc/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_openacc/%.S,$@) -o $@

$(SOBJ_opencl):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_opencl/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,sctk_opencl/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,src/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%.o,src/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o:include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_cuda):$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o:sctk_cuda/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,sctk_cuda/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,sctk_cuda/%.f,$@) -o $@

$(FOBJ_openacc):$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o:sctk_openacc/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,sctk_openacc/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,sctk_openacc/%.f,$@) -o $@

$(FOBJ_opencl):$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o:sctk_opencl/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,sctk_opencl/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,sctk_opencl/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o:src/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,src/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Accelerators/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
