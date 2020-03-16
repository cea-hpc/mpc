      subroutine mpc_fortran_init_predefined_constants()
          include 'mpif.h'
          call mpc_predef_init_inplace(MPI_IN_PLACE)
          call mpc_predef_init_bottom(MPI_BOTTOM)
          call mpc_predef_init_status_ignore(MPI_STATUS_IGNORE)
          call mpc_predef_init_statuses_ignore(MPI_STATUSES_IGNORE)
          !!call mpc_predef_init_unweighted(MPI_UNWEIGHTED)
          return
      end subroutine mpc_fortran_init_predefined_constants
