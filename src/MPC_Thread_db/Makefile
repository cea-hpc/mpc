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
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Debugger/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Debugger/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Debugger/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Debugger/lib/%.d)


FILES_dbg_thread=$(wildcard sctk_dbg_thread/*.c)
OBJ_dbg_thread=$(FILES_dbg_thread:sctk_dbg_thread/%.c=$(M_PWD)/BUILD_MPC_Debugger/lib/%.o)
DEP_dbg_thread=$(FILES_dbg_thread:sctk_dbg_thread/%.c=$(M_PWD)/BUILD_MPC_Debugger/lib/%.d)

SFILES_dbg_thread=$(wildcard sctk_dbg_thread/*.S)
SOBJ_dbg_thread=$(SFILES_dbg_thread:sctk_dbg_thread/%.S=$(M_PWD)/BUILD_MPC_Debugger/lib/%.o)
SDEP_dbg_thread=$(SFILES_dbg_thread:sctk_dbg_thread/%.S=$(M_PWD)/BUILD_MPC_Debugger/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Debugger/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Debugger/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Debugger/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Debugger/lib/%.d)


FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o)

FFILES_dbg_thread=$(wildcard sctk_dbg_thread/*.f)
FOBJ_dbg_thread=$(FFILES_dbg_thread:sctk_dbg_thread/%.f=$(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_dbg_thread) $(FOBJ_dbg_thread) $(SOBJ_dbg_thread) $(OBJ_src) $(FOBJ_src) $(SOBJ_src)

$(M_PWD)/include_modules/MPC_Debugger:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Debugger/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Debugger/sctk_dbg_thread $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Debugger/src $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Debugger


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_dbg_thread) $(DEP_src)

$(DEPENDS):Makefile
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_dbg_thread):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:sctk_dbg_thread/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,sctk_dbg_thread/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,include/%.c,$@)" > $@

$(DEP_dbg_thread):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:sctk_dbg_thread/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,sctk_dbg_thread/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,src/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,$(shell pwd)/include/%.c,$@) -o $@

$(OBJ_dbg_thread):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,$(shell pwd)/sctk_dbg_thread/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,$(shell pwd)/src/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:include/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_dbg_thread):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:sctk_dbg_thread/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,sctk_dbg_thread/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Debugger/lib/%.d:src/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,$(shell pwd)/include/%.S,$@) -o $@

$(SOBJ_dbg_thread):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,$(shell pwd)/sctk_dbg_thread/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%.o,$(shell pwd)/src/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o:include/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_dbg_thread):$(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o:sctk_dbg_thread/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o,sctk_dbg_thread/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o:src/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Debugger/lib/%_f.o,src/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
