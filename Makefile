##########################################################################

#Global setup variables
PREFIX=/home/jvet/mpc2
SUBPREFIX=x86_64
PROJECT_SOURCE_DIR=/home/jvet/EXASCALE/EXASCALE/MPC/.

#Global targets
all: install-generic-subprefix
install-all : x86_64

#final install to make generic
install-generic-subprefix: install-all
	mkdir -p $(PREFIX)/generic/bin
	for exe in $(PREFIX)/host/bin/*; do ln -sf $(PREFIX)/generic/bin/arch_wrapper $(PREFIX)/generic/bin/`basename "$$exe"`; done
	cp $(PROJECT_SOURCE_DIR)/tools/arch_wrapper $(PREFIX)/generic/bin/

.PHONY: all install-all install-generic-subprefix
##########################################################################

x86_64: 
	$(MAKE) -C $@

.PHONY: x86_64

