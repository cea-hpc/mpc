# Makefile generated with ../MPC_Tools/mpc_gen_makefile Thu Feb 16 15:55:08 CET 2012
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

FILES_doc=$(wildcard doc/*.c)
OBJ_doc=$(FILES_doc:doc/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_doc=$(FILES_doc:doc/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_doc=$(wildcard doc/*.S)
SOBJ_doc=$(SFILES_doc:doc/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_doc=$(SFILES_doc:doc/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FILES_generator=$(wildcard generator/*.c)
OBJ_generator=$(FILES_generator:generator/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_generator=$(FILES_generator:generator/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_generator=$(wildcard generator/*.S)
SOBJ_generator=$(SFILES_generator:generator/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_generator=$(SFILES_generator:generator/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Config/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Config/lib/%.d)


FFILES_doc=$(wildcard doc/*.f)
FOBJ_doc=$(FFILES_doc:doc/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

FFILES_generator=$(wildcard generator/*.f)
FOBJ_generator=$(FFILES_generator:generator/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Config/lib/%_f.o)

all: $(OBJ_doc) $(FOBJ_doc) $(SOBJ_doc) $(OBJ_generator) $(FOBJ_generator) $(SOBJ_generator) $(OBJ_src) $(FOBJ_src) $(SOBJ_src)

$(M_PWD)/include_modules/MPC_Config:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/doc $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/generator $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Config/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Config


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_doc) $(DEP_generator) $(DEP_src) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_doc):$(M_PWD)/BUILD_MPC_Config/lib/%.d:doc/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,doc/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,doc/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_generator):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generator/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generator/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generator/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Config/lib/%.d:src/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_doc):$(M_PWD)/BUILD_MPC_Config/lib/%.d:doc/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,doc/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,doc/%.c,$@)" > $@

$(DEP_generator):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generator/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generator/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generator/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Config/lib/%.d:src/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_doc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,doc/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,doc/%.c,$@) -o $@

$(OBJ_generator):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,generator/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,generator/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,src/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,src/%.c,$@) -o $@

$(SDEP_doc):$(M_PWD)/BUILD_MPC_Config/lib/%.d:doc/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,doc/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,doc/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_generator):$(M_PWD)/BUILD_MPC_Config/lib/%.d:generator/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generator/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,generator/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Config/lib/%.d:src/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_doc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,doc/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,doc/%.S,$@) -o $@

$(SOBJ_generator):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,generator/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,generator/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,src/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%.o,src/%.S,$@) -o $@

$(FOBJ_doc):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:doc/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,doc/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,doc/%.f,$@) -o $@

$(FOBJ_generator):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:generator/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,generator/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,generator/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Config/lib/%_f.o:src/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,src/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Config/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
