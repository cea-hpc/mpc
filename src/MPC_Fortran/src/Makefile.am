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

# Default configuration file and schema installation
install-data-local: genfortraniface
	test -d $(pkgdatadir)/fortran/ || mkdir $(pkgdatadir)/fortran/
	install -m 0644 -D $(builddir)/mpif.h $(pkgdatadir)/fortran/
	install -m 0644 -D $(builddir)/*.f90 $(pkgdatadir)/fortran/
	install -m 0644 -D $(builddir)/*.c $(pkgdatadir)/fortran/
	install -m 0644 -D $(srcdir)/mpc_fortran.c $(pkgdatadir)/fortran/
	install -m 0644 -D $(srcdir)/mpc_fortran_helpers.c $(pkgdatadir)/fortran/
	install -m 0644 -D $(srcdir)/mpc_fortran_helpers.h $(pkgdatadir)/fortran/
	install -m 0644 -D $(srcdir)/mpc_fortran.c $(pkgdatadir)/fortran/
	install -m 0644 -D $(srcdir)/*.f90 $(pkgdatadir)/fortran/
	install -m 0644 -D $(srcdir)/Makefile.deploy $(pkgdatadir)/fortran/Makefile
