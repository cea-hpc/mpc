# Makefile generated with ../MPC_Tools/mpc_gen_makefile Mon May 14 10:11:28 CEST 2012
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

FILES_cmake=$(wildcard cmake/*.c)
OBJ_cmake=$(FILES_cmake:cmake/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
DEP_cmake=$(FILES_cmake:cmake/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)

SFILES_cmake=$(wildcard cmake/*.S)
SOBJ_cmake=$(SFILES_cmake:cmake/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
SDEP_cmake=$(SFILES_cmake:cmake/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)


FILES_include=$(wildcard include/*.c)
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)


FILES_tests=$(wildcard tests/*.c)
OBJ_tests=$(FILES_tests:tests/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
DEP_tests=$(FILES_tests:tests/%.c=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)

SFILES_tests=$(wildcard tests/*.S)
SOBJ_tests=$(SFILES_tests:tests/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.o)
SDEP_tests=$(SFILES_tests:tests/%.S=$(M_PWD)/BUILD_MPC_Allocator/lib/%.d)


FFILES_cmake=$(wildcard cmake/*.f)
FOBJ_cmake=$(FFILES_cmake:cmake/%.f=$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o)

FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o)

FFILES_tests=$(wildcard tests/*.f)
FOBJ_tests=$(FFILES_tests:tests/%.f=$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o)

all: $(OBJ_cmake) $(FOBJ_cmake) $(SOBJ_cmake) $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_src) $(FOBJ_src) $(SOBJ_src) $(OBJ_tests) $(FOBJ_tests) $(SOBJ_tests)

$(M_PWD)/include_modules/MPC_Allocator:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Allocator/cmake $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Allocator/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Allocator/src $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Allocator/tests $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Allocator


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_cmake) $(DEP_include) $(DEP_src) $(DEP_tests) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_cmake):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:cmake/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,cmake/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,cmake/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_include):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:src/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,src/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_tests):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:tests/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,tests/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,tests/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_cmake):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:cmake/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,cmake/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,cmake/%.c,$@)" > $@

$(DEP_include):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,include/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:src/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,src/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,src/%.c,$@)" > $@

$(DEP_tests):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:tests/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,tests/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,tests/%.c,$@)" > $@

endif

$(OBJ_cmake):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,cmake/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,cmake/%.c,$@) -o $@

$(OBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,include/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,src/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,src/%.c,$@) -o $@

$(OBJ_tests):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,tests/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,tests/%.c,$@) -o $@

$(SDEP_cmake):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:cmake/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,cmake/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,cmake/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_include):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:src/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,src/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_tests):$(M_PWD)/BUILD_MPC_Allocator/lib/%.d:tests/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,tests/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,tests/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_cmake):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,cmake/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,cmake/%.S,$@) -o $@

$(SOBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,include/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,src/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,src/%.S,$@) -o $@

$(SOBJ_tests):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,tests/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%.o,tests/%.S,$@) -o $@

$(FOBJ_cmake):$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o:cmake/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,cmake/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,cmake/%.f,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o:include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o:src/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,src/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,src/%.f,$@) -o $@

$(FOBJ_tests):$(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o:tests/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,tests/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Allocator/lib/%_f.o,tests/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
