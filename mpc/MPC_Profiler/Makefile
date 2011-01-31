# Makefile generated with ../MPC_Tools/mpc_gen_makefile Mon Jan 31 16:28:05 CET 2011
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
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Profiler/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Profiler/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Profiler/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Profiler/lib/%.d)


FILES_profiling=$(wildcard sctk_profiling/*.c)
OBJ_profiling=$(FILES_profiling:sctk_profiling/%.c=$(M_PWD)/BUILD_MPC_Profiler/lib/%.o)
DEP_profiling=$(FILES_profiling:sctk_profiling/%.c=$(M_PWD)/BUILD_MPC_Profiler/lib/%.d)

SFILES_profiling=$(wildcard sctk_profiling/*.S)
SOBJ_profiling=$(SFILES_profiling:sctk_profiling/%.S=$(M_PWD)/BUILD_MPC_Profiler/lib/%.o)
SDEP_profiling=$(SFILES_profiling:sctk_profiling/%.S=$(M_PWD)/BUILD_MPC_Profiler/lib/%.d)


FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o)

FFILES_profiling=$(wildcard sctk_profiling/*.f)
FOBJ_profiling=$(FFILES_profiling:sctk_profiling/%.f=$(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_profiling) $(FOBJ_profiling) $(SOBJ_profiling) build_sctk_trace_expand

$(M_PWD)/include_modules/MPC_Profiler:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Profiler/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Profiler/sctk_profiling $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Profiler


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_profiling) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_Profiler/lib/%.d:include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_profiling):$(M_PWD)/BUILD_MPC_Profiler/lib/%.d:sctk_profiling/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,sctk_profiling/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,sctk_profiling/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_Profiler/lib/%.d:include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,include/%.c,$@)" > $@

$(DEP_profiling):$(M_PWD)/BUILD_MPC_Profiler/lib/%.d:sctk_profiling/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,sctk_profiling/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,sctk_profiling/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,include/%.c,$@) -o $@

$(OBJ_profiling):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,sctk_profiling/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,sctk_profiling/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_Profiler/lib/%.d:include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_profiling):$(M_PWD)/BUILD_MPC_Profiler/lib/%.d:sctk_profiling/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,sctk_profiling/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,sctk_profiling/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,include/%.S,$@) -o $@

$(SOBJ_profiling):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,sctk_profiling/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%.o,sctk_profiling/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o:include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o,include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_profiling):$(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o:sctk_profiling/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o,sctk_profiling/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Profiler/lib/%_f.o,sctk_profiling/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
build_sctk_trace_expand:
	@mkdir -p $(M_PWD)/BUILD_MPC_Profiler/sctk_trace_expand
	@$(MAKE) BUILD_DIR=$(M_PWD)/BUILD_MPC_Profiler/sctk_trace_expand -C sctk_trace_expand all