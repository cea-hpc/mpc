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
#   - JAEGER Julien julien.jaeger@cea.fr                               #
#                                                                      #
########################################################################
noinst_HEADERS=$(srcdir)/include/mpc_runtime_config.h

# Probably better to replace this with:
# configdir =/path
# configdir_DATA= files
install-data-local:
	test -d $(DESTDIR)$(pkgdatadir) || mkdir -p $(DESTDIR)$(pkgdatadir)
	cp -r $(abs_srcdir)/editor $(DESTDIR)$(pkgdatadir)
	install -m 0644 -D $(abs_srcdir)/src/generated/mpc_config_meta.js $(DESTDIR)$(pkgdatadir)/editor/javascript/mpc_config_meta.js
	install -m 0644 -D $(abs_srcdir)/src/generated/mpc_config_xsd.js $(DESTDIR)$(pkgdatadir)/editor/javascript/mpc_config_xsd.js
	install -m 0644 -D $(abs_srcdir)/src/generated/mpc-config.xsd $(DESTDIR)$(pkgdatadir)/mpc-config.xsd
	install -m 0644 -D $(abs_srcdir)/extra/config.xml.example $(DESTDIR)$(pkgdatadir)/config.xml

uninstall-local:
	rm -rf $(DESTDIR)$(pkgdatadir)/editor
	rm -f $(DESTDIR)$(pkgdatadir)/config.xml
	rm -f $(DESTDIR)$(pkgdatadir)/mpc-config.xsd


SUBDIRS=src
