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
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#                                                                      #
########################################################################

BUILD_DIR=`pwd`
#
# Rule 'all'
#
all:make_all

#
# Rule 'configure'
#
configure:make_configure

#
# Rule 'build'
#
build:make_build

#
# Rule 'install'
#
install:install_all

#
# Rule check
#
check:
	cd mpc_build && $(MAKE) check 

#
# Module mpc-hydra / Version 1.4.0
#
mpc-hydra-1.4.0_build/config.status: Makefile /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-hydra/configure
	mkdir -p mpc-hydra-1.4.0_build
	cd mpc-hydra-1.4.0_build && /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-hydra/configure       1.4.0 --prefix=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-hydra-1.4.0 '' '' '' '' '' ' CFLAGS=-fPIC'
	touch mpc-hydra-1.4.0_build/config.status

mpc-hydra-1.4.0_config:mpc-hydra-1.4.0_build/config.status

mpc-hydra-1.4.0_make: mpc-hydra-1.4.0_config
	$(MAKE) -C mpc-hydra-1.4.0_build/hydra 
	$(MAKE) -C mpc-hydra-1.4.0_build/simple 

mpc-hydra-1.4.0_install: mpc-hydra-1.4.0_make
	$(MAKE) -C mpc-hydra-1.4.0_build/hydra install 

#
# Module mpc-hwloc / Version 1.3
#
export HWLOC_PREFIX_INSTALL=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-hwloc-1.3
export HWLOC_PREFIX_BUILD=/data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-hwloc-1.3_build/hwloc-1.3
mpc-hwloc-1.3_build/config.status: Makefile /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-hwloc/configure
	mkdir -p mpc-hwloc-1.3_build
	cd mpc-hwloc-1.3_build && /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-hwloc/configure       1.3 --prefix=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-hwloc-1.3 '' '' '' '' '' ''
	touch mpc-hwloc-1.3_build/config.status

mpc-hwloc-1.3_config:mpc-hwloc-1.3_build/config.status

mpc-hwloc-1.3_make: mpc-hwloc-1.3_config
	$(MAKE) -C mpc-hwloc-1.3_build/hwloc-1.3 

mpc-hwloc-1.3_install: mpc-hwloc-1.3_make
	$(MAKE) -C mpc-hwloc-1.3_build/hwloc-1.3 install 

#
# Module mpc-openpa / Version 1.0.2
#
export OPENPA_PREFIX_INSTALL=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-openpa-1.0.2
export OPENPA_PREFIX_BUILD=/data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-openpa-1.0.2_build/openpa-1.0.2
mpc-openpa-1.0.2_build/config.status: Makefile /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-openpa/configure
	mkdir -p mpc-openpa-1.0.2_build
	cd mpc-openpa-1.0.2_build && /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-openpa/configure       1.0.2 --prefix=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-openpa-1.0.2 '' '' '' '' '' ' CFLAGS=-fPIC'
	touch mpc-openpa-1.0.2_build/config.status

mpc-openpa-1.0.2_config:mpc-openpa-1.0.2_build/config.status

mpc-openpa-1.0.2_make: mpc-openpa-1.0.2_config
	$(MAKE) -C mpc-openpa-1.0.2_build/openpa-1.0.2 

mpc-openpa-1.0.2_install: mpc-openpa-1.0.2_make
	$(MAKE) -C mpc-openpa-1.0.2_build/openpa-1.0.2 install 

#
# Module mpc-libxml2 / Version 2.8.0
#
export LIBXML2_PREFIX_INSTALL=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-libxml2-2.8.0
export LIBXML2_PREFIX_BUILD=/data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-libxml2-2.8.0_build/libxml2-2.8.0
mpc-libxml2-2.8.0_build/config.status: Makefile /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-libxml2/configure
	mkdir -p mpc-libxml2-2.8.0_build
	cd mpc-libxml2-2.8.0_build && /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc-libxml2/configure       2.8.0 --prefix=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev//mpc-libxml2-2.8.0 '' '' '' '' '' ''
	touch mpc-libxml2-2.8.0_build/config.status

mpc-libxml2-2.8.0_config:mpc-libxml2-2.8.0_build/config.status

mpc-libxml2-2.8.0_make: mpc-libxml2-2.8.0_config
	$(MAKE) -C mpc-libxml2-2.8.0_build/libxml2-2.8.0 

mpc-libxml2-2.8.0_install: mpc-libxml2-2.8.0_make
	$(MAKE) -C mpc-libxml2-2.8.0_build/libxml2-2.8.0 install 

#
# Rule clean
#
clean:
	@rm -fr mpc_build  mpc-hydra-1.4.0_build mpc-hwloc-1.3_build mpc-openpa-1.0.2_build mpc-libxml2-2.8.0_build

#
# Rule for MPC
#
mpc_build/config.status: mpc-hydra-1.4.0_build/config.status mpc-hwloc-1.3_build/config.status mpc-openpa-1.0.2_build/config.status mpc-libxml2-2.8.0_build/config.status Makefile
	mkdir -p mpc_build
	cd mpc_build && /data/samba/spulvera/07-ConfigMPC/dev/mpc_clone/mpc//configure   --with-libxml2=embeded  --with-openpa=embeded --with-hwloc=embeded --with-hydra --enable-shell-colors --enable-debug-messages --enable-debug --prefix=/data/samba/spulvera/07-ConfigMPC/install/mpc-dev/  
	touch mpc_build/config.status

mpc_config:mpc_build/config.status

mpc_make: mpc_build/config.status  mpc-hydra-1.4.0_make mpc-hwloc-1.3_make mpc-openpa-1.0.2_make mpc-libxml2-2.8.0_make
	$(MAKE) -C mpc_build 

mpc_install: mpc_make  mpc-hydra-1.4.0_install mpc-hwloc-1.3_install mpc-openpa-1.0.2_install mpc-libxml2-2.8.0_install
	$(MAKE) -C mpc_build install 

#
# Rule make_all
#
make_all: make_build
#
# Rule make_build
#
make_build:  mpc-hydra-1.4.0_make mpc-hwloc-1.3_make mpc-openpa-1.0.2_make mpc-libxml2-2.8.0_make mpc_make 
#
# Rule make_configure
#
make_configure:  mpc-hydra-1.4.0_config mpc-hwloc-1.3_config mpc-openpa-1.0.2_config mpc-libxml2-2.8.0_config mpc_config 
#
# Rule install_all
#
install_all:  mpc-hydra-1.4.0_install mpc-hwloc-1.3_install mpc-openpa-1.0.2_install mpc-libxml2-2.8.0_install mpc_install 
