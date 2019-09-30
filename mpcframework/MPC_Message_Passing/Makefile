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

FILES_ib=$(wildcard ib/*.c)
OBJ_ib=$(FILES_ib:ib/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_ib=$(FILES_ib:ib/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_ib=$(wildcard ib/*.S)
SOBJ_ib=$(SFILES_ib:ib/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_ib=$(SFILES_ib:ib/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_include=$(wildcard include/*.c)
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_portals=$(wildcard portals/*.c)
OBJ_portals=$(FILES_portals:portals/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_portals=$(FILES_portals:portals/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_portals=$(wildcard portals/*.S)
SOBJ_portals=$(SFILES_portals:portals/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_portals=$(SFILES_portals:portals/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_portals_am=$(wildcard portals_am/*.c)
OBJ_portals_am=$(FILES_portals_am:portals_am/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_portals_am=$(FILES_portals_am:portals_am/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_portals_am=$(wildcard portals_am/*.S)
SOBJ_portals_am=$(SFILES_portals_am:portals_am/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_portals_am=$(SFILES_portals_am:portals_am/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_shm=$(wildcard shm/*.c)
OBJ_shm=$(FILES_shm:shm/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_shm=$(FILES_shm:shm/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_shm=$(wildcard shm/*.S)
SOBJ_shm=$(SFILES_shm:shm/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_shm=$(SFILES_shm:shm/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_src=$(wildcard src/*.c)
OBJ_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_src=$(FILES_src:src/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_src=$(wildcard src/*.S)
SOBJ_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_src=$(SFILES_src:src/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_tcp=$(wildcard tcp/*.c)
OBJ_tcp=$(FILES_tcp:tcp/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_tcp=$(FILES_tcp:tcp/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_tcp=$(wildcard tcp/*.S)
SOBJ_tcp=$(SFILES_tcp:tcp/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_tcp=$(SFILES_tcp:tcp/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FFILES_ib=$(wildcard ib/*.f)
FOBJ_ib=$(FFILES_ib:ib/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_portals=$(wildcard portals/*.f)
FOBJ_portals=$(FFILES_portals:portals/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_portals_am=$(wildcard portals_am/*.f)
FOBJ_portals_am=$(FFILES_portals_am:portals_am/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_shm=$(wildcard shm/*.f)
FOBJ_shm=$(FFILES_shm:shm/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_src=$(wildcard src/*.f)
FOBJ_src=$(FFILES_src:src/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_tcp=$(wildcard tcp/*.f)
FOBJ_tcp=$(FFILES_tcp:tcp/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

all: $(OBJ_ib) $(FOBJ_ib) $(SOBJ_ib) $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_portals) $(FOBJ_portals) $(SOBJ_portals) $(OBJ_portals_am) $(FOBJ_portals_am) $(SOBJ_portals_am) $(OBJ_shm) $(FOBJ_shm) $(SOBJ_shm) $(OBJ_src) $(FOBJ_src) $(SOBJ_src) $(OBJ_tcp) $(FOBJ_tcp) $(SOBJ_tcp)

$(M_PWD)/include_modules/MPC_Message_Passing:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/ib $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/portals $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/portals_am $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/shm $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/src $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/tcp $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Message_Passing


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_ib) $(DEP_include) $(DEP_portals) $(DEP_portals_am) $(DEP_shm) $(DEP_src) $(DEP_tcp) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:ib/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,ib/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_portals):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:portals/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,portals/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_portals_am):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:portals_am/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,portals_am/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_shm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:shm/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,shm/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_src):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,src/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:tcp/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,tcp/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:ib/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,ib/%.c,$@)" > $@

$(DEP_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:include/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.c,$@)" > $@

$(DEP_portals):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:portals/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,portals/%.c,$@)" > $@

$(DEP_portals_am):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:portals_am/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,portals_am/%.c,$@)" > $@

$(DEP_shm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:shm/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,shm/%.c,$@)" > $@

$(DEP_src):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:src/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,src/%.c,$@)" > $@

$(DEP_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:tcp/%.c
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,tcp/%.c,$@)" > $@

endif

$(OBJ_ib):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/ib/%.c,$@) -o $@

$(OBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/include/%.c,$@) -o $@

$(OBJ_portals):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/portals/%.c,$@) -o $@

$(OBJ_portals_am):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/portals_am/%.c,$@) -o $@

$(OBJ_shm):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/shm/%.c,$@) -o $@

$(OBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/src/%.c,$@) -o $@

$(OBJ_tcp):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/tcp/%.c,$@) -o $@

$(SDEP_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:ib/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,ib/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:include/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_portals):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:portals/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,portals/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_portals_am):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:portals_am/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,portals_am/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_shm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:shm/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,shm/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_src):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:src/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,src/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:tcp/%.S
	@echo "DEP $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,tcp/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_ib):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/ib/%.S,$@) -o $@

$(SOBJ_include):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/include/%.S,$@) -o $@

$(SOBJ_portals):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/portals/%.S,$@) -o $@

$(SOBJ_portals_am):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/portals_am/%.S,$@) -o $@

$(SOBJ_shm):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/shm/%.S,$@) -o $@

$(SOBJ_src):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/src/%.S,$@) -o $@

$(SOBJ_tcp):%.o:%.d
	@echo "CC  $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,$(shell pwd)/tcp/%.S,$@) -o $@

$(FOBJ_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:ib/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,ib/%.f,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:include/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_portals):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:portals/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,portals/%.f,$@) -o $@

$(FOBJ_portals_am):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:portals_am/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,portals_am/%.f,$@) -o $@

$(FOBJ_shm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:shm/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,shm/%.f,$@) -o $@

$(FOBJ_src):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:src/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,src/%.f,$@) -o $@

$(FOBJ_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:tcp/%.f
	@echo "F77 $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,tcp/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
