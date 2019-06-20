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

FILES_comm_layer=$(wildcard comm_layer/*.c)
OBJ_comm_layer=$(FILES_comm_layer:comm_layer/%.c=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.o)
DEP_comm_layer=$(FILES_comm_layer:comm_layer/%.c=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d)

SFILES_comm_layer=$(wildcard comm_layer/*.S)
SOBJ_comm_layer=$(SFILES_comm_layer:comm_layer/%.S=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.o)
SDEP_comm_layer=$(SFILES_comm_layer:comm_layer/%.S=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d)


FILES_include=$(wildcard include/*.c)
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d)


FFILES_comm_layer=$(wildcard comm_layer/*.f)
FOBJ_comm_layer=$(FFILES_comm_layer:comm_layer/%.f=$(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o)

FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o)

all: $(OBJ_comm_layer) $(FOBJ_comm_layer) $(SOBJ_comm_layer) $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_src) $(FOBJ_src) $(SOBJ_src)

$(M_PWD)/include_modules/MPC_Active_Message:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Active_Message/comm_layer $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Active_Message/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Active_Message/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Active_Message


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_comm_layer) $(DEP_include) $(DEP_src) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_comm_layer):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:comm_layer/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,comm_layer/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_include):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_comm_layer):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:comm_layer/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,comm_layer/%.c,$@)" > $@

$(DEP_include):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,include/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_comm_layer):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,$(shell pwd)/comm_layer/%.c,$@) -o $@

$(OBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,$(shell pwd)/include/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,$(shell pwd)/src/%.c,$@) -o $@

$(SDEP_comm_layer):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:comm_layer/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,comm_layer/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_include):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:include/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Active_Message/lib/%.d:src/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_comm_layer):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,$(shell pwd)/comm_layer/%.S,$@) -o $@

$(SOBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,$(shell pwd)/include/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%.o,$(shell pwd)/src/%.S,$@) -o $@

$(FOBJ_comm_layer):$(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o:comm_layer/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o,comm_layer/%.f,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o:include/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o:src/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Active_Message/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
