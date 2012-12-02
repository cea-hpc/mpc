# Makefile generated with ../MPC_Tools/mpc_gen_makefile Mon Oct 29 20:52:52 CET 2012
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
OBJ_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_include=$(FILES_include:include/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_include=$(wildcard include/*.S)
SOBJ_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_include=$(SFILES_include:include/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_mpc=$(wildcard mpc/*.c)
OBJ_mpc=$(FILES_mpc:mpc/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_mpc=$(FILES_mpc:mpc/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_mpc=$(wildcard mpc/*.S)
SOBJ_mpc=$(SFILES_mpc:mpc/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_mpc=$(SFILES_mpc:mpc/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_driver_ib=$(wildcard sctk_driver_ib/*.c)
OBJ_driver_ib=$(FILES_driver_ib:sctk_driver_ib/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_driver_ib=$(FILES_driver_ib:sctk_driver_ib/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_driver_ib=$(wildcard sctk_driver_ib/*.S)
SOBJ_driver_ib=$(SFILES_driver_ib:sctk_driver_ib/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_driver_ib=$(SFILES_driver_ib:sctk_driver_ib/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_driver_tcp=$(wildcard sctk_driver_tcp/*.c)
OBJ_driver_tcp=$(FILES_driver_tcp:sctk_driver_tcp/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_driver_tcp=$(FILES_driver_tcp:sctk_driver_tcp/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_driver_tcp=$(wildcard sctk_driver_tcp/*.S)
SOBJ_driver_tcp=$(SFILES_driver_tcp:sctk_driver_tcp/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_driver_tcp=$(SFILES_driver_tcp:sctk_driver_tcp/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_inter_thread_comm=$(wildcard sctk_inter_thread_comm/*.c)
OBJ_inter_thread_comm=$(FILES_inter_thread_comm:sctk_inter_thread_comm/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_inter_thread_comm=$(FILES_inter_thread_comm:sctk_inter_thread_comm/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_inter_thread_comm=$(wildcard sctk_inter_thread_comm/*.S)
SOBJ_inter_thread_comm=$(SFILES_inter_thread_comm:sctk_inter_thread_comm/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_inter_thread_comm=$(SFILES_inter_thread_comm:sctk_inter_thread_comm/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FILES_low_level_comm=$(wildcard sctk_low_level_comm/*.c)
OBJ_low_level_comm=$(FILES_low_level_comm:sctk_low_level_comm/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
DEP_low_level_comm=$(FILES_low_level_comm:sctk_low_level_comm/%.c=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)

SFILES_low_level_comm=$(wildcard sctk_low_level_comm/*.S)
SOBJ_low_level_comm=$(SFILES_low_level_comm:sctk_low_level_comm/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o)
SDEP_low_level_comm=$(SFILES_low_level_comm:sctk_low_level_comm/%.S=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d)


FFILES_include=$(wildcard include/*.f)
FOBJ_include=$(FFILES_include:include/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_mpc=$(wildcard mpc/*.f)
FOBJ_mpc=$(FFILES_mpc:mpc/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_driver_ib=$(wildcard sctk_driver_ib/*.f)
FOBJ_driver_ib=$(FFILES_driver_ib:sctk_driver_ib/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_driver_tcp=$(wildcard sctk_driver_tcp/*.f)
FOBJ_driver_tcp=$(FFILES_driver_tcp:sctk_driver_tcp/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_inter_thread_comm=$(wildcard sctk_inter_thread_comm/*.f)
FOBJ_inter_thread_comm=$(FFILES_inter_thread_comm:sctk_inter_thread_comm/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

FFILES_low_level_comm=$(wildcard sctk_low_level_comm/*.f)
FOBJ_low_level_comm=$(FFILES_low_level_comm:sctk_low_level_comm/%.f=$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o)

all: $(OBJ_include) $(FOBJ_include) $(SOBJ_include) $(OBJ_mpc) $(FOBJ_mpc) $(SOBJ_mpc) $(OBJ_driver_ib) $(FOBJ_driver_ib) $(SOBJ_driver_ib) $(OBJ_driver_tcp) $(FOBJ_driver_tcp) $(SOBJ_driver_tcp) $(OBJ_inter_thread_comm) $(FOBJ_inter_thread_comm) $(SOBJ_inter_thread_comm) $(OBJ_low_level_comm) $(FOBJ_low_level_comm) $(SOBJ_low_level_comm)

$(M_PWD)/include_modules/MPC_Message_Passing:
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/include $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/mpc $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/sctk_driver_ib $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/sctk_driver_tcp $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/sctk_inter_thread_comm $(M_PWD)/include_modules
	@../MPC_Tools/mpc_prepare_includes $(MPC_SOURCE_DIR)MPC_Message_Passing/sctk_low_level_comm $(M_PWD)/include_modules
	@touch $@

prepare_includes:$(M_PWD)/include_modules/MPC_Message_Passing


ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP_include) $(DEP_mpc) $(DEP_driver_ib) $(DEP_driver_tcp) $(DEP_inter_thread_comm) $(DEP_low_level_comm) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:include/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_mpc):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:mpc/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,mpc/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,mpc/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_driver_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_driver_ib/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_ib/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_ib/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_driver_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_driver_tcp/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_tcp/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_tcp/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_inter_thread_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_inter_thread_comm/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_inter_thread_comm/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_inter_thread_comm/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(DEP_low_level_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_low_level_comm/%.c
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_low_level_comm/%.c,$@)"
	@$(CC) $(SCTK_CC_DEP) $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_low_level_comm/%.c,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:include/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.c,$@)" > $@

$(DEP_mpc):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:mpc/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,mpc/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,mpc/%.c,$@)" > $@

$(DEP_driver_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_driver_ib/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_ib/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_ib/%.c,$@)" > $@

$(DEP_driver_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_driver_tcp/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_tcp/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_tcp/%.c,$@)" > $@

$(DEP_inter_thread_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_inter_thread_comm/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_inter_thread_comm/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_inter_thread_comm/%.c,$@)" > $@

$(DEP_low_level_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_low_level_comm/%.c
	@echo "    Generate fake dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_low_level_comm/%.c,$@)"
	@echo "$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_low_level_comm/%.c,$@)" > $@

endif

$(OBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,include/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,include/%.c,$@) -o $@

$(OBJ_mpc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,mpc/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,mpc/%.c,$@) -o $@

$(OBJ_driver_ib):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_ib/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_ib/%.c,$@) -o $@

$(OBJ_driver_tcp):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_tcp/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_tcp/%.c,$@) -o $@

$(OBJ_inter_thread_comm):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_inter_thread_comm/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_inter_thread_comm/%.c,$@) -o $@

$(OBJ_low_level_comm):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_low_level_comm/%.c,$@)"
	@$(CC) -c $(LOCAL_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_low_level_comm/%.c,$@) -o $@

$(SDEP_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:include/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,include/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_mpc):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:mpc/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,mpc/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,mpc/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_driver_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_driver_ib/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_ib/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_ib/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_driver_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_driver_tcp/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_tcp/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_driver_tcp/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_inter_thread_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_inter_thread_comm/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_inter_thread_comm/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_inter_thread_comm/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SDEP_low_level_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d:sctk_low_level_comm/%.S
	@echo "    Generate dependencies $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_low_level_comm/%.S,$@)"
	@$(GCC) -M $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,sctk_low_level_comm/%.S,$@) | \
		sed 's,$(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

$(SOBJ_include):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,include/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,include/%.S,$@) -o $@

$(SOBJ_mpc):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,mpc/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,mpc/%.S,$@) -o $@

$(SOBJ_driver_ib):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_ib/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_ib/%.S,$@) -o $@

$(SOBJ_driver_tcp):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_tcp/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_driver_tcp/%.S,$@) -o $@

$(SOBJ_inter_thread_comm):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_inter_thread_comm/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_inter_thread_comm/%.S,$@) -o $@

$(SOBJ_low_level_comm):%.o:%.d
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_low_level_comm/%.S,$@)"
	@$(GCC) -c $(GNU_CFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%.o,sctk_low_level_comm/%.S,$@) -o $@

$(FOBJ_include):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:include/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,include/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,include/%.f,$@) -o $@

$(FOBJ_mpc):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:mpc/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,mpc/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,mpc/%.f,$@) -o $@

$(FOBJ_driver_ib):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:sctk_driver_ib/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_driver_ib/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_driver_ib/%.f,$@) -o $@

$(FOBJ_driver_tcp):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:sctk_driver_tcp/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_driver_tcp/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_driver_tcp/%.f,$@) -o $@

$(FOBJ_inter_thread_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:sctk_inter_thread_comm/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_inter_thread_comm/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_inter_thread_comm/%.f,$@) -o $@

$(FOBJ_low_level_comm):$(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o:sctk_low_level_comm/%.f
	@echo "    Build $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_low_level_comm/%.f,$@)"
	@$(F77) -c $(FFLAGS) $(patsubst $(M_PWD)/BUILD_MPC_Message_Passing/lib/%_f.o,sctk_low_level_comm/%.f,$@) -o $@


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
