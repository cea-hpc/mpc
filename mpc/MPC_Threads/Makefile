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
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Threads/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Threads/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Threads/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Threads/lib/%.d)


FILES_thread=$(wildcard sctk_thread/*.c)
OBJ_thread=$(FILES_thread:sctk_thread/%.c=$(M_PWD)/BUILD_MPC_Threads/lib/%.o)
DEP_thread=$(FILES_thread:sctk_thread/%.c=$(M_PWD)/BUILD_MPC_Threads/lib/%.d)

SFILES_thread=$(wildcard sctk_thread/*.S)
SOBJ_thread=$(SFILES_thread:sctk_thread/%.S=$(M_PWD)/BUILD_MPC_Threads/lib/%.o)
SDEP_thread=$(SFILES_thread:sctk_thread/%.S=$(M_PWD)/BUILD_MPC_Threads/lib/%.d)


FILES_topology=$(wildcard sctk_topology/*.c)
OBJ_topology=$(FILES_topology:sctk_topology/%.c=$(M_PWD)/BUILD_MPC_Threads/lib/%.o)
DEP_topology=$(FILES_topology:sctk_topology/%.c=$(M_PWD)/BUILD_MPC_Threads/lib/%.d)

SFILES_topology=$(wildcard sctk_topology/*.S)
SOBJ_topology=$(SFILES_topology:sctk_topology/%.S=$(M_PWD)/BUILD_MPC_Threads/lib/%.o)
SDEP_topology=$(SFILES_topology:sctk_topology/%.S=$(M_PWD)/BUILD_MPC_Threads/lib/%.d)


FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Threads/lib/%_f.o)

FFILES_thread=$(wildcard sctk_thread/*.f)
FOBJ_thread=$(FFILES_thread:sctk_thread/%.f=$(M_PWD)/BUILD_MPC_Threads/lib/%_f.o)

FFILES_topology=$(wildcard sctk_topology/*.f)
FOBJ_topology=$(FFILES_topology:sctk_topology/%.f=$(M_PWD)/BUILD_MPC_Threads/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_thread) $(FOBJ_thread) $(SOBJ_thread) $(OBJ_topology) $(FOBJ_topology) $(SOBJ_topology)

$(M_PWD)/include_modules/MPC_Threads:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Threads/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Threads/sctk_thread $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Threads/sctk_topology $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Threads


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_thread) $(DEP_topology) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_thread):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:sctk_thread/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_thread/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_thread/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_topology):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:sctk_topology/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_topology/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_topology/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,include/%.c,$@)" > $@

$(DEP_thread):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:sctk_thread/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_thread/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_thread/%.c,$@)" > $@

$(DEP_topology):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:sctk_topology/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_topology/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_topology/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,include/%.c,$@) -o $@

$(OBJ_thread):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_thread/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_thread/%.c,$@) -o $@

$(OBJ_topology):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_topology/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_topology/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_thread):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:sctk_thread/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_thread/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_thread/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_topology):$(M_PWD)/BUILD_MPC_Threads/lib/%.d:sctk_topology/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_topology/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,sctk_topology/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,include/%.S,$@) -o $@

$(SOBJ_thread):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_thread/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_thread/%.S,$@) -o $@

$(SOBJ_topology):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_topology/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%.o,sctk_topology/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Threads/lib/%_f.o:include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%_f.o,include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_thread):$(M_PWD)/BUILD_MPC_Threads/lib/%_f.o:sctk_thread/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%_f.o,sctk_thread/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%_f.o,sctk_thread/%.f,$@) -o $@

$(FOBJ_topology):$(M_PWD)/BUILD_MPC_Threads/lib/%_f.o:sctk_topology/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%_f.o,sctk_topology/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Threads/lib/%_f.o,sctk_topology/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
