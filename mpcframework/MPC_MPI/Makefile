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
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)


FILES_mpc=$(wildcard mpc/*.c)
OBJ_mpc=$(FILES_mpc:mpc/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
DEP_mpc=$(FILES_mpc:mpc/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)

SFILES_mpc=$(wildcard mpc/*.S)
SOBJ_mpc=$(SFILES_mpc:mpc/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
SDEP_mpc=$(SFILES_mpc:mpc/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)


FILES_mpi=$(wildcard mpi/*.c)
OBJ_mpi=$(FILES_mpi:mpi/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
DEP_mpi=$(FILES_mpi:mpi/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)

SFILES_mpi=$(wildcard mpi/*.S)
SOBJ_mpi=$(SFILES_mpi:mpi/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
SDEP_mpi=$(SFILES_mpi:mpi/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_MPI/lib/%.d)


FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o)

FFILES_mpc=$(wildcard mpc/*.f)
FOBJ_mpc=$(FFILES_mpc:mpc/%.f=$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o)

FFILES_mpi=$(wildcard mpi/*.f)
FOBJ_mpi=$(FFILES_mpi:mpi/%.f=$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_mpc) $(FOBJ_mpc) $(SOBJ_mpc) $(OBJ_mpi) $(FOBJ_mpi) $(SOBJ_mpi) $(OBJ_src) $(FOBJ_src) $(SOBJ_src)

$(M_PWD)/include_modules/MPC_MPI:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_MPI/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_MPI/mpc $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_MPI/mpi $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_MPI/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_MPI


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_mpc) $(DEP_mpi) $(DEP_src) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpc):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:mpc/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,mpc/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpi):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:mpi/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,mpi/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,include/%.c,$@)" > $@

$(DEP_mpc):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:mpc/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,mpc/%.c,$@)" > $@

$(DEP_mpi):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:mpi/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,mpi/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/include/%.c,$@) -o $@

$(OBJ_mpc):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/mpc/%.c,$@) -o $@

$(OBJ_mpi):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/mpi/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/src/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:include/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpc):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:mpc/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,mpc/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpi):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:mpi/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,mpi/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_MPI/lib/%.d:src/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/include/%.S,$@) -o $@

$(SOBJ_mpc):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/mpc/%.S,$@) -o $@

$(SOBJ_mpi):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/mpi/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%.o,$(shell pwd)/src/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o:include/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_mpc):$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o:mpc/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,mpc/%.f,$@) -o $@

$(FOBJ_mpi):$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o:mpi/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,mpi/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_MPI/lib/%_f.o:src/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_MPI/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
