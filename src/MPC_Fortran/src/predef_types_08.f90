
        subroutine mpc_fortran08_init_predefined_constants()
          use mpi_f08
          call mpc_predef08_init_inplace(MPI_IN_PLACE)
          call mpc_predef08_init_bottom(MPI_BOTTOM)
          call mpc_predef08_init_status_ignore(MPI_STATUS_IGNORE)
          call mpc_predef08_init_statuses_ignore(MPI_STATUSES_IGNORE)
          !!call mpc_predef08_init_unweighted(MPI_UNWEIGHTED)
          return
        end subroutine mpc_fortran08_init_predefined_constants
