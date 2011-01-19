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

##########################################
# Variables provided by the caller:

# BUILD_DIR -> Build directory for the executable
# DIR_DEST_EXE -> directory to put the executable
# PREFIX_EXE -> Prefix for the executables
# SUFFIX_EXE -> suffix for the executables
# CC -> C compiler
# CFLAGS -> C compiler flags
# LDFLAGS ->
# SCTK_MAKE_NO_DEP ->
# YACC ->
# LEX ->

##########################################

#
# Local variables
# The following variables can be modified
#
EXE=mpc_trace_expand


FILES=$(wildcard *.c)
OBJ=$(FILES:%.c=$(BUILD_DIR)/%.o)
DEP=$(FILES:%.c=$(BUILD_DIR)/%.d)


all: $(DIR_DEST_EXE)/$(PREFIX_EXE)$(EXE)$(SUFFIX_EXE)

ifneq ($(SCTK_MAKE_NO_DEP),yes)
DEPENDS=$(DEP) 

$(DEPENDS):Makefile 
endif

ifneq ($(SCTK_MAKE_NO_DEP),yes)
$(DEP):$(BUILD_DIR)/%.d:%.c
	@echo "    Generate dependencies $(patsubst $(BUILD_DIR)/%.d,%.c,$@)"
	@$(CC) -M $(CFLAGS) $(patsubst $(BUILD_DIR)/%.d,%.c,$@) | \
		sed 's,$(patsubst $(BUILD_DIR)/%.d,%.o,$@):,$(patsubst %.d,%.o,$@) $@:,g'> $@ 2> /dev/null

else
$(DEP):$(BUILD_DIR)/%.d:%.c
	@echo "    Generate fake dependencies $(patsubst $(BUILD_DIR)/%.d,%.c,$@)"
	@echo "$(patsubst $(BUILD_DIR)/%.d,%.c,$@)" > $@

endif

$(OBJ):%.o:%.d
	@echo "    Build $(patsubst $(BUILD_DIR)/%.o,%.c,$@)"
	@$(CC) -c $(CFLAGS) $(patsubst $(BUILD_DIR)/%.o,%.c,$@) -o $@


$(DIR_DEST_EXE)/$(PREFIX_EXE)$(EXE)$(SUFFIX_EXE):$(OBJ)
	@echo "    Build $@"
	@$(CC) $(OBJ) $(CFLAGS) -o $@ $(LDFLAGS)


ifneq ($(SCTK_MAKE_NO_DEP),yes)
ifeq ($(MAKECMDGOALS),all)
-include $(DEPENDS)
endif
endif
