AM_CPPFLAGS =  -I$(abs_top_builddir)/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_IO/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_Fortran/include/ -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_builddir) @MPC_MODULE_DEFINES@
AM_CPPFLAGS += -I$(abs_top_builddir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Arch/include/

AM_FCFLAGS=-I$(abs_top_builddir)/src/MPC_Fortran/src/ @MPC_MODULE_DEFINES@

constants.json: gen_constants.c
	$(CC) $(AM_CPPFLAGS) $^ -o ./gen_constants
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

# We can generate, so generate and save (should not produce diff)
genfortraniface: genmod.py preparsed_mpih.dat constants.json
	python $(srcdir)/genmod.py -m $(builddir)/preparsed_mpih.dat
	@cp *.c *.h *.f90 $(srcdir)/pregenerated/
	@touch genfortraniface

else

# No generation is possible so just use the pre-generated versions
genfortraniface: genmod.py preparsed_mpih.dat constants.json
	@cp $(srcdir)/pregenerated/* $(builddir)/
	@touch genfortraniface

endif

MPCFORT_FILES=mpi_base.f90 mpi_constants.f90 mpi_f08_c.f90 mpi_f08_c_iface.c mpi_f08_constants.f90 mpi_f08_ctypes.f90 mpi_f08.f90 mpi_f08_types.f90 mpi.f90 mpi_sizeofs.f90

$(MPCFORT_FILES) : genfortraniface

mpi_f08_types.lo: mpi_f08_ctypes.lo
mpi_f08_constants.lo: mpi_f08_types.lo
mpi_f08_c.lo: mpi_f08_constants.lo
mpi_f08.lo: mpi_f08_constants.lo mpi_f08_c.lo
predef_types_08.lo: mpi_f08.lo
mpi_base.lo: mpi_constants.lo
mpi.f90: mpi_sizeofs.lo mpi_base.lo 

lib_LTLIBRARIES = libmpcfortran.la

libmpcfortran_la_SOURCES = mpc_fortran.c mpc_fortran_helpers.c omp.f90 predef_types_08.f90 predef_types.f90 $(MPCFORT_FILES)