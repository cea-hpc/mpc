AM_CPPFLAGS =  -I$(abs_top_builddir)/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/ -I$(abs_top_srcdir)/src/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_MPI/include/ -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_builddir)

AM_FCFLAGS=-I$(abs_top_builddir)/src/MPC_Fortran/src/

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

FORTRAN_MOD_FILES=mpi_constants.f90 mpi_sizeofs.f90 mpi_base.f90 mpi.f90 mpi_f08_ctypes.f90 mpi_f08_types.f90 mpi_f08_constants.f90 mpi_f08_c.f90 mpi_f08.f90 mpi_f08_c_iface.c

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

$(FORTRAN_MOD_FILES) mpif.h: genfortraniface

lib_LTLIBRARIES = libmpcfortran.la

# F90 module fortran deps
mpi_base.lo: mpi_constants.lo
mpi.lo:mpi_sizeofs.lo mpi_base.lo

# We need to explicit this dependency as the C file is generated
mpi_f08_c_iface.$(OBJEXT): gen_fortran_iface
# F08 module fortran deps
mpi_f08_types.lo: mpi_f08_ctypes.lo
mpi_f08_constants.lo: mpi_f08_types.lo
mpi_f08_c.lo: mpi_f08_constants.lo
mpi_f08.lo: mpi_f08_constants.lo mpi_f08_c.lo
predef_types_08.lo: mpi_f08.lo

predef_types.lo: mpi.lo

libmpcfortran_la_SOURCES = $(FORTRAN_MOD_FILES) predef_types.f90 predef_types_08.f90 omp.f90
libmpcfortran_la_LIBADD = $(abs_top_builddir)/src/lib/libmpcframework.la