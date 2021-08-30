AM_CPPFLAGS =  -I$(abs_top_builddir)/ -I$(abs_top_srcdir)/src/MPC_Lowcomm/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/include/ -I$(abs_top_builddir)/src/MPC_IO/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_Fortran/include/ -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_builddir) @MPC_MODULE_DEFINES@
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Common/include/
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_Fortran/src/

AM_FCFLAGS=-I$(abs_top_builddir)/src/MPC_Fortran/src/

constants.json: gen_constants.c
	$(CC) $(LDFLAGS) $(CFLAGS) $(AM_CPPFLAGS) $^ -o ./gen_constants
	./gen_constants > constants.json

preparsed_mpih.dat: preparsed_mpih.c
	$(CC) $(AM_CPPFLAGS) -E $^ -o $@


clean-local:
	rm -f $(builddir)/gen_constants
	rm -f $(builddir)/constants.json
	rm -f $(builddir)/*.c  $(builddir)/*.h
	rm -f $(builddir)/preparsed_mpih.dat
	rm -f $(builddir)/*.f90
	rm -f $(builddir)/*.mod
	rm -f $(builddir)/genfortraniface

if HAVE_PYTHON


MPCFORT_FILES= mpi_f08_c_iface.c mpi_constants.f90 mpi_f08_c.f90 mpi_f08_constants.f90 mpi_f08_ctypes.f90 mpi_f08.f90 mpi_f08_types.f90 mpi.f90 mpi_sizeofs.f90 mpi_base.f90

# We can generate, so generate and save (should not produce diff)
genfortraniface: genmod.py preparsed_mpih.dat constants.json
	@PYTHON@ $(srcdir)/genmod.py -m $(builddir)/preparsed_mpih.dat
	@cp mpif.h $(MPCFORT_FILES) $(srcdir)/pregenerated/
	@touch genfortraniface

else

# No generation is possible so just use the pre-generated versions
genfortraniface: genmod.py preparsed_mpih.dat constants.json
	@cp $(srcdir)/pregenerated/* $(builddir)/
	@touch genfortraniface

endif

BUILT_SOURCES= $(MPCFORT_FILES) mpif.h

$(MPCFORT_FILES) mpif.h : genfortraniface

# Let Fortran find out its dependency mess
mpi_module.f90: mpi_sizeofs.f90 mpi_constants.f90 mpi_base.f90 mpi.f90 $(srcdir)/predef_types.f90
	cat $^ > $@

# Let Fortran find out its dependency mess
mpi08_module.f90: mpi_f08_ctypes.f90 mpi_f08_types.f90 mpi_f08_constants.f90 mpi_f08_c.f90 mpi_f08.f90 mpi_f08_c.f90 $(srcdir)/predef_types_08.f90
	cat $^ > $@

lib_LTLIBRARIES = libmpcfortran.la

# F08 Iface
libmpcfortran_la_SOURCES = mpi_f08_c_iface.c  mpi08_module.f90 
# F90 Iface
libmpcfortran_la_SOURCES += mpi_module.f90 
# F77 Iface
libmpcfortran_la_SOURCES += mpc_fortran.c mpc_fortran_helpers.c
# OMP
libmpcfortran_la_SOURCES += omp.f90 

if MPC_HAVE_AUTOPRIV
# Generate a privatized library note that
# by defaut autopriv wrappers do not privatize

lib_LTLIBRARIES += libmpcfortran_priv.la

# We need to sequentialize compilation for the two fortran
# modules files as they have the same modules inside and
# lead to random failures as compiler manipulates .mod files
mpi08_module_priv.f90 :  mpi08_module.f90 libmpcfortran.la
	cat $< > $@

# F08 Iface
libmpcfortran_priv_la_SOURCES = mpi_f08_c_iface.c  mpi08_module_priv.f90 
# F90 Iface

mpi_module_priv.f90 :  mpi_module.f90 libmpcfortran.la
	cat $< > $@

libmpcfortran_priv_la_SOURCES += mpi_module_priv.f90 
# F77 Iface
libmpcfortran_priv_la_SOURCES += mpc_fortran.c mpc_fortran_helpers.c
# OMP

omp_priv.f90: omp.f90 libmpcfortran.la
	cat $< > $@

libmpcfortran_priv_la_SOURCES += omp_priv.f90 

libmpcfortran_priv_la_CFLAGS = -fmpc-privatize $(AM_CPPFLAGS)
libmpcfortran_priv_la_FCFLAGS = -fmpc-privatize $(AM_FCFLAGS)

endif

#
# Install Fortran modules
#

MPCFORT_MODULES = mpif.h mpi_base.mod mpi_constants.mod mpi_f08_c.mod mpi_f08_constants.mod mpi_f08_ctypes.mod mpi_f08.mod mpi_f08_types.mod mpi.mod mpi_sizeofs.mod omp_lib_kinds.mod omp_lib.mod

$(MPCFORT_MODULES): libmpcfortran.la

mpc_fort_headerdir=$(pkgincludedir)/fortran/
mpc_fort_header_HEADERS = $(MPCFORT_MODULES)
