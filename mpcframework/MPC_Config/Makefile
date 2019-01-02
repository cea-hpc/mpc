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

FILES_editor=$(wildcard editor/*.c)
OBJ_editor=$(FILES_editor:editor/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_editor=$(FILES_editor:editor/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_editor=$(wildcard editor/*.S)
SOBJ_editor=$(SFILES_editor:editor/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_editor=$(SFILES_editor:editor/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FILES_extra=$(wildcard extra/*.c)
OBJ_extra=$(FILES_extra:extra/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_extra=$(FILES_extra:extra/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_extra=$(wildcard extra/*.S)
SOBJ_extra=$(SFILES_extra:extra/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_extra=$(SFILES_extra:extra/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FILES_generated=$(wildcard generated/*.c)
OBJ_generated=$(FILES_generated:generated/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_generated=$(FILES_generated:generated/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_generated=$(wildcard generated/*.S)
SOBJ_generated=$(SFILES_generated:generated/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_generated=$(SFILES_generated:generated/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FILES_generators=$(wildcard generators/*.c)
OBJ_generators=$(FILES_generators:generators/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_generators=$(FILES_generators:generators/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_generators=$(wildcard generators/*.S)
SOBJ_generators=$(SFILES_generators:generators/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_generators=$(SFILES_generators:generators/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FFILES_editor=$(wildcard editor/*.f)
FOBJ_editor=$(FFILES_editor:editor/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

FFILES_extra=$(wildcard extra/*.f)
FOBJ_extra=$(FFILES_extra:extra/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

FFILES_generated=$(wildcard generated/*.f)
FOBJ_generated=$(FFILES_generated:generated/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

FFILES_generators=$(wildcard generators/*.f)
FOBJ_generators=$(FFILES_generators:generators/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

all: $(OBJ_editor) $(FOBJ_editor) $(SOBJ_editor) $(OBJ_extra) $(FOBJ_extra) $(SOBJ_extra) $(OBJ_generated) $(FOBJ_generated) $(SOBJ_generated) $(OBJ_generators) $(FOBJ_generators) $(SOBJ_generators) $(OBJ_src) $(FOBJ_src) $(SOBJ_src) build_bin build_doc

$(M_PWD)/include_modules/MPC_Config:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/editor $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/extra $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/generated $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/generators $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Config


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_editor) $(DEP_extra) $(DEP_generated) $(DEP_generators) $(DEP_src) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_editor):$(M_PWD)/BUILD_MPC_Config/lib/%.d:editor/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,editor/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_extra):$(M_PWD)/BUILD_MPC_Config/lib/%.d:extra/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,extra/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_generated):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generated/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generated/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_generators):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generators/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generators/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Config/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_editor):$(M_PWD)/BUILD_MPC_Config/lib/%.d:editor/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,editor/%.c,$@)" > $@

$(DEP_extra):$(M_PWD)/BUILD_MPC_Config/lib/%.d:extra/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,extra/%.c,$@)" > $@

$(DEP_generated):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generated/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generated/%.c,$@)" > $@

$(DEP_generators):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generators/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generators/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Config/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_editor):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/editor/%.c,$@) -o $@

$(OBJ_extra):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/extra/%.c,$@) -o $@

$(OBJ_generated):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/generated/%.c,$@) -o $@

$(OBJ_generators):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/generators/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/src/%.c,$@) -o $@

$(SDEP_editor):$(M_PWD)/BUILD_MPC_Config/lib/%.d:editor/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,editor/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_extra):$(M_PWD)/BUILD_MPC_Config/lib/%.d:extra/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,extra/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_generated):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generated/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generated/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_generators):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generators/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generators/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Config/lib/%.d:src/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_editor):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/editor/%.S,$@) -o $@

$(SOBJ_extra):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/extra/%.S,$@) -o $@

$(SOBJ_generated):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/generated/%.S,$@) -o $@

$(SOBJ_generators):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/generators/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,$(shell pwd)/src/%.S,$@) -o $@

$(FOBJ_editor):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:editor/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,editor/%.f,$@) -o $@

$(FOBJ_extra):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:extra/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,extra/%.f,$@) -o $@

$(FOBJ_generated):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:generated/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,generated/%.f,$@) -o $@

$(FOBJ_generators):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:generators/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,generators/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:src/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
build_bin:
	@mkdir -p $(M_PWD)/BUILD_MPC_Config/bin
	@echo "MAK bin"
	@$(MAKE) BUILD_DIR=$(M_PWD)/BUILD_MPC_Config/bin -C bin all
build_doc:
	@mkdir -p $(M_PWD)/BUILD_MPC_Config/doc
	@echo "MAK doc"
	@$(MAKE) BUILD_DIR=$(M_PWD)/BUILD_MPC_Config/doc -C doc all
