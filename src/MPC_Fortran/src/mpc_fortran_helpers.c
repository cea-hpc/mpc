#include <mpc_mpi.h>

#include <mpc_common_types.h>
#include <sctk_debug.h>

/************************
 * FORTRAN PREDEF TYPES *
 ************************/

/* Now Define Special Fortran pointers */

void **mpi_predef08_bottom(void)
{
        static void *mpi_bottom_ptr = NULL;

        return &mpi_bottom_ptr;
}

void **mpi_predef08_inplace(void)
{
        static void *mpi_in_place_ptr = NULL;

        return &mpi_in_place_ptr;
}

void **mpi_predef08_status_ignore(void)
{
        static void *mpi_status_ignore_ptr = NULL;

        return &mpi_status_ignore_ptr;
}

void **mpi_predef08_statuses_ignore(void)
{
        static void *mpi_statuses_ignore_ptr = NULL;

        return &mpi_statuses_ignore_ptr;
}

void mpc_predef08_init_inplace_(void *inplace)
{
        *(mpi_predef08_inplace() ) = inplace;
}

void mpc_predef08_init_bottom_(void *bottom)
{
        *(mpi_predef08_bottom() ) = bottom;
}

void mpc_predef08_init_status_ignore_(void *status_ignore)
{
        *(mpi_predef08_status_ignore() ) = status_ignore;
}

void mpc_predef08_init_statuses_ignore_(void *statuses_ignore)
{
        *(mpi_predef08_statuses_ignore() ) = statuses_ignore;
}

void **mpi_predef_bottom(void)
{
        static void *mpi_bottom_ptr = NULL;

        return &mpi_bottom_ptr;
}

void **mpi_predef_inplace(void)
{
        static void *mpi_in_place_ptr = NULL;

        return &mpi_in_place_ptr;
}

void **mpi_predef_status_ignore(void)
{
        static void *mpi_status_ignore_ptr = NULL;

        return &mpi_status_ignore_ptr;
}

void **mpi_predef_statuses_ignore(void)
{
        static void *mpi_statuses_ignore_ptr = NULL;

        return &mpi_statuses_ignore_ptr;
}

void mpc_predef_init_inplace_(void *inplace)
{
        *(mpi_predef_inplace() ) = inplace;
}

void mpc_predef_init_bottom_(void *bottom)
{
        *(mpi_predef_bottom() ) = bottom;
}

void mpc_predef_init_status_ignore_(void *status_ignore)
{
        *(mpi_predef_status_ignore() ) = status_ignore;
}

void mpc_predef_init_statuses_ignore_(void *statuses_ignore)
{
        *(mpi_predef_statuses_ignore() ) = statuses_ignore;
}
