
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

noinst_HEADERS =

AM_CPPFLAGS = -I$(top_builddir)/src/include -I$(top_builddir)/src/MPI_IO/src/ -I$(top_srcdir)/src/MPI_IO/src/ -I$(top_builddir)/src/MPC_IO/include
AM_CPPFLAGS += -I$(top_builddir)/src/MPC_IO/src/ -I$(top_srcdir)/src/MPC_IO/src/ -I$(top_srcdir)/src/MPC_IO/src/romio/mpi-io/ -I$(top_srcdir)/src/MPC_IO/include/
AM_CPPFLAGS += -I$(top_srcdir)/src/MPC_MPI/include/ -I$(top_srcdir)/src/include/ -I$(top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_builddir)/src/MPC_Common/include/
AM_CPPFLAGS += -I$(top_srcdir)/src/MPC_Lowcomm/include/ -I$(top_srcdir)/src/MPC_Common/include/ -I$(top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(top_builddir)/adio/include  -I$(top_srcdir)/src/MPC_IO/src/romio/adio/include -I$(top_srcdir)/src/MPC_Threads/include/


# ------------------------------------------------------------------------
# variables to be populated by the included Makefile.mk fragments:

# These are files that contain MPI routines (e.g., MPI_File_open).
# In MPICH these will have an MPI_ and a PMPI_ version.  Other implementations
# (like OMPI) only want these to be MPI_ routines, possibly with some
# name-shifting prefix.
romio_mpi_sources =

# regular old source files that implement ROMIO, such as ADIO code
romio_other_sources =



if MPIO_GLUE_DEFAULT
romio_other_sources +=              \
    mpi-io/glue/default/mpio_file.c \
    mpi-io/glue/default/mpio_err.c
endif MPIO_GLUE_DEFAULT


if MPIO_GLUE_MPICH
romio_other_sources +=             \
    mpi-io/glue/mpich/mpio_file.c \
    mpi-io/glue/mpich/mpio_err.c
endif MPIO_GLUE_MPICH

if MPIO_GLUE_OPENMPI
romio_other_sources +=              \
    mpi-io/glue/openmpi/mpio_file.c \
    mpi-io/glue/openmpi/mpio_err.c
endif MPIO_GLUE_OPENMPI


if !BUILD_ROMIO_EMBEDDED
romio_other_sources += \
    mpi-io/glue/large_count.c
endif


if BUILD_F77_BINDINGS
romio_mpi_sources +=              \
    mpi-io/fortran/closef.c       \
    mpi-io/fortran/deletef.c      \
    mpi-io/fortran/fsyncf.c       \
    mpi-io/fortran/get_amodef.c   \
    mpi-io/fortran/get_atomf.c    \
    mpi-io/fortran/get_bytofff.c  \
    mpi-io/fortran/get_errhf.c    \
    mpi-io/fortran/get_extentf.c  \
    mpi-io/fortran/get_groupf.c   \
    mpi-io/fortran/get_infof.c    \
    mpi-io/fortran/get_posn_shf.c \
    mpi-io/fortran/get_posnf.c    \
    mpi-io/fortran/get_sizef.c    \
    mpi-io/fortran/get_viewf.c    \
    mpi-io/fortran/iotestf.c      \
    mpi-io/fortran/iowaitf.c      \
    mpi-io/fortran/iread_atf.c    \
    mpi-io/fortran/iread_shf.c    \
    mpi-io/fortran/ireadf.c       \
    mpi-io/fortran/iwrite_atf.c   \
    mpi-io/fortran/iwrite_shf.c   \
    mpi-io/fortran/iwritef.c      \
    mpi-io/fortran/openf.c        \
    mpi-io/fortran/preallocf.c    \
    mpi-io/fortran/rd_atallbf.c   \
    mpi-io/fortran/rd_atallef.c   \
    mpi-io/fortran/read_allbf.c   \
    mpi-io/fortran/read_allef.c   \
    mpi-io/fortran/read_allf.c    \
    mpi-io/fortran/read_atallf.c  \
    mpi-io/fortran/read_atf.c     \
    mpi-io/fortran/read_ordbf.c   \
    mpi-io/fortran/read_ordef.c   \
    mpi-io/fortran/read_ordf.c    \
    mpi-io/fortran/read_shf.c     \
    mpi-io/fortran/readf.c        \
    mpi-io/fortran/seek_shf.c     \
    mpi-io/fortran/seekf.c        \
    mpi-io/fortran/set_atomf.c    \
    mpi-io/fortran/set_errhf.c    \
    mpi-io/fortran/set_infof.c    \
    mpi-io/fortran/set_sizef.c    \
    mpi-io/fortran/set_viewf.c    \
    mpi-io/fortran/wr_atallbf.c   \
    mpi-io/fortran/wr_atallef.c   \
    mpi-io/fortran/write_allbf.c  \
    mpi-io/fortran/write_allef.c  \
    mpi-io/fortran/write_allf.c   \
    mpi-io/fortran/write_atallf.c \
    mpi-io/fortran/write_atf.c    \
    mpi-io/fortran/write_ordbf.c  \
    mpi-io/fortran/write_ordef.c  \
    mpi-io/fortran/write_ordf.c   \
    mpi-io/fortran/write_shf.c    \
    mpi-io/fortran/writef.c
endif BUILD_F77_BINDINGS


noinst_HEADERS += mpi-io/mpioimpl.h mpi-io/mpioprof.h

romio_mpi_sources +=          \
    mpi-io/close.c            \
    mpi-io/delete.c           \
    mpi-io/file_c2f.c         \
    mpi-io/file_f2c.c         \
    mpi-io/fsync.c            \
    mpi-io/get_amode.c        \
    mpi-io/get_atom.c         \
    mpi-io/get_bytoff.c       \
    mpi-io/get_extent.c       \
    mpi-io/get_group.c        \
    mpi-io/get_info.c         \
    mpi-io/get_posn.c         \
    mpi-io/get_posn_sh.c      \
    mpi-io/get_size.c         \
    mpi-io/get_view.c         \
    mpi-io/iread.c            \
    mpi-io/iread_all.c        \
    mpi-io/iread_at.c         \
    mpi-io/iread_atall.c      \
    mpi-io/iread_sh.c         \
    mpi-io/iwrite.c           \
    mpi-io/iwrite_all.c       \
    mpi-io/iwrite_at.c        \
    mpi-io/iwrite_atall.c     \
    mpi-io/iwrite_sh.c        \
    mpi-io/open.c             \
    mpi-io/prealloc.c         \
    mpi-io/rd_atallb.c        \
    mpi-io/rd_atalle.c        \
    mpi-io/read.c             \
    mpi-io/read_all.c         \
    mpi-io/read_allb.c        \
    mpi-io/read_alle.c        \
    mpi-io/read_at.c          \
    mpi-io/read_atall.c       \
    mpi-io/read_ord.c         \
    mpi-io/read_ordb.c        \
    mpi-io/read_orde.c        \
    mpi-io/read_sh.c          \
    mpi-io/register_datarep.c \
    mpi-io/seek.c             \
    mpi-io/seek_sh.c          \
    mpi-io/set_atom.c         \
    mpi-io/set_info.c         \
    mpi-io/set_size.c         \
    mpi-io/set_view.c         \
    mpi-io/wr_atallb.c        \
    mpi-io/wr_atalle.c        \
    mpi-io/write.c            \
    mpi-io/write_all.c        \
    mpi-io/write_allb.c       \
    mpi-io/write_alle.c       \
    mpi-io/write_at.c         \
    mpi-io/write_atall.c      \
    mpi-io/write_ord.c        \
    mpi-io/write_ordb.c       \
    mpi-io/write_orde.c       \
    mpi-io/write_sh.c


# non-MPI/PMPI sources that will be included in libromio
romio_other_sources +=       \
    mpi-io/mpich_fileutil.c \
    mpi-io/mpir-mpioinit.c   \
    mpi-io/mpiu_greq.c \
    mpi-io/mpiu_external32.c \
    mpi-io/mpir_cst_filesys.c

# helper variables for conditionally compiled sources
mpio_request_sources=   \
    mpi-io/ioreq_c2f.c  \
    mpi-io/ioreq_f2c.c  \
    mpi-io/iotest.c     \
    mpi-io/iotestall.c  \
    mpi-io/iotestany.c  \
    mpi-io/iotestsome.c \
    mpi-io/iowait.c     \
    mpi-io/iowaitall.c  \
    mpi-io/iowaitany.c  \
    mpi-io/iowaitsome.c

mpio_extra_sources =  \
    mpi-io/get_errh.c \
    mpi-io/set_errh.c

# not used in MPICH, we use generalized requests instead
if BUILD_MPIO_REQUEST
romio_other_sources += $(mpio_request_sources)
endif BUILD_MPIO_REQUEST

# not used in MPICH
if BUILD_MPIO_ERRHAN
romio_other_sources += $(mpio_request_sources)
endif BUILD_MPIO_ERRHAN

noinst_HEADERS +=                      \
    adio/include/adio.h                \
    adio/include/adio_cb_config_list.h \
    adio/include/adio_extern.h         \
    adio/include/adioi.h               \
    adio/include/adioi_errmsg.h        \
    adio/include/adioi_error.h         \
    adio/include/adioi_fs_proto.h      \
    adio/include/heap_sort.h           \
    adio/include/mpio_error.h          \
    adio/include/mpipr.h               \
    adio/include/mpiu_greq.h           \
    adio/include/nopackage.h           \
    adio/include/mpiu_external32.h     \
    adio/include/hint_fns.h



if BUILD_AD_DAOS

noinst_HEADERS += adio/ad_daos/ad_daos.h

romio_other_sources += \
    adio/ad_daos/ad_daos.c \
    adio/ad_daos/ad_daos_close.c \
    adio/ad_daos/ad_daos_common.c \
    adio/ad_daos/ad_daos_fcntl.c \
    adio/ad_daos/ad_daos_features.c \
    adio/ad_daos/ad_daos_hhash.c \
    adio/ad_daos/ad_daos_hints.c \
    adio/ad_daos/ad_daos_io.c \
    adio/ad_daos/ad_daos_io_str.c \
    adio/ad_daos/ad_daos_open.c \
    adio/ad_daos/ad_daos_resize.c

endif BUILD_AD_DAOS


if BUILD_AD_GPFS

noinst_HEADERS +=                                                    \
    adio/ad_gpfs/ad_gpfs_aggrs.h                                         \
    adio/ad_gpfs/ad_gpfs.h                                               \
    adio/ad_gpfs/ad_gpfs_tuning.h

romio_other_sources +=                                               \
    adio/ad_gpfs/ad_gpfs_aggrs.c                                         \
    adio/ad_gpfs/ad_gpfs_close.c                                         \
    adio/ad_gpfs/ad_gpfs_flush.c                                         \
    adio/ad_gpfs/ad_gpfs_tuning.c                                        \
    adio/ad_gpfs/ad_gpfs.c                                               \
    adio/ad_gpfs/ad_gpfs_open.c                                          \
    adio/ad_gpfs/ad_gpfs_hints.c                                         \
    adio/ad_gpfs/ad_gpfs_rdcoll.c                                        \
    adio/ad_gpfs/ad_gpfs_wrcoll.c

endif BUILD_AD_GPFS

if BUILD_AD_BG

noinst_HEADERS +=                                                    \
    adio/ad_gpfs/bg/ad_bg_aggrs.h                                         \
    adio/ad_gpfs/bg/ad_bg_pset.h

romio_other_sources +=                                               \
    adio/ad_gpfs/bg/ad_bg_aggrs.c                                         \
    adio/ad_gpfs/bg/ad_bg_pset.c

endif BUILD_AD_BG

if BUILD_AD_PE

noinst_HEADERS +=                                                    \
    adio/ad_gpfs/pe/ad_pe_aggrs.h

romio_other_sources +=                                               \
    adio/ad_gpfs/pe/ad_pe_aggrs.c

endif BUILD_AD_PE


if BUILD_AD_LUSTRE

noinst_HEADERS += adio/ad_lustre/ad_lustre.h

romio_other_sources +=                   \
    adio/ad_lustre/ad_lustre.c           \
    adio/ad_lustre/ad_lustre_open.c      \
    adio/ad_lustre/ad_lustre_rwcontig.c  \
    adio/ad_lustre/ad_lustre_wrcoll.c    \
    adio/ad_lustre/ad_lustre_wrstr.c     \
    adio/ad_lustre/ad_lustre_hints.c     \
    adio/ad_lustre/ad_lustre_aggregate.c

if LUSTRE_LOCKAHEAD
romio_other_sources +=                   \
    adio/ad_lustre/ad_lustre_lock.c
endif LUSTRE_LOCKAHEAD

endif BUILD_AD_LUSTRE


if BUILD_AD_NFS

noinst_HEADERS += adio/ad_nfs/ad_nfs.h

romio_other_sources +=            \
    adio/ad_nfs/ad_nfs_read.c     \
    adio/ad_nfs/ad_nfs_open.c     \
    adio/ad_nfs/ad_nfs_write.c    \
    adio/ad_nfs/ad_nfs_done.c     \
    adio/ad_nfs/ad_nfs_fcntl.c    \
    adio/ad_nfs/ad_nfs_iread.c    \
    adio/ad_nfs/ad_nfs_iwrite.c   \
    adio/ad_nfs/ad_nfs_wait.c     \
    adio/ad_nfs/ad_nfs_setsh.c    \
    adio/ad_nfs/ad_nfs_getsh.c    \
    adio/ad_nfs/ad_nfs.c          \
    adio/ad_nfs/ad_nfs_resize.c   \
    adio/ad_nfs/ad_nfs_features.c

endif BUILD_AD_NFS

## NTFS builds are handled entirely by the separate Windows build system
##include $(top_srcdir)/src/MPC_IO/src/romio/adio//ad_ntfs/Makefile.mk

if BUILD_PANFS_OPEN6
# override open with one that uses newer features
panfs_open = adio/ad_panfs/ad_panfs_open6.c
else
panfs_open = adio/ad_panfs/ad_panfs_open.c
endif

if BUILD_AD_PANFS

noinst_HEADERS += adio/ad_panfs/ad_panfs.h

romio_other_sources +=              \
    adio/ad_panfs/ad_panfs.c        \
    $(panfs_open)                   \
    adio/ad_panfs/ad_panfs_hints.c  \
    adio/ad_panfs/ad_panfs_read.c   \
    adio/ad_panfs/ad_panfs_resize.c \
    adio/ad_panfs/ad_panfs_write.c

endif BUILD_AD_PANFS

if BUILD_AD_PVFS2

noinst_HEADERS +=                       \
    adio/ad_pvfs2/ad_pvfs2.h            \
    adio/ad_pvfs2/ad_pvfs2_io.h         \
    adio/ad_pvfs2/ad_pvfs2_common.h

romio_other_sources +=                          \
    adio/ad_pvfs2/ad_pvfs2_close.c              \
    adio/ad_pvfs2/ad_pvfs2_read.c               \
    adio/ad_pvfs2/ad_pvfs2_open.c               \
    adio/ad_pvfs2/ad_pvfs2_write.c              \
    adio/ad_pvfs2/ad_pvfs2_fcntl.c              \
    adio/ad_pvfs2/ad_pvfs2_flush.c              \
    adio/ad_pvfs2/ad_pvfs2_resize.c             \
    adio/ad_pvfs2/ad_pvfs2_hints.c              \
    adio/ad_pvfs2/ad_pvfs2_delete.c             \
    adio/ad_pvfs2/ad_pvfs2.c                    \
    adio/ad_pvfs2/ad_pvfs2_common.c             \
    adio/ad_pvfs2/ad_pvfs2_aio.c                \
    adio/ad_pvfs2/ad_pvfs2_read_list_classic.c  \
    adio/ad_pvfs2/ad_pvfs2_io_list.c            \
    adio/ad_pvfs2/ad_pvfs2_io_dtype.c           \
    adio/ad_pvfs2/ad_pvfs2_write_list_classic.c \
    adio/ad_pvfs2/ad_pvfs2_features.c

endif BUILD_AD_PVFS2

if BUILD_AD_TESTFS

noinst_HEADERS += adio/ad_testfs/ad_testfs.h

romio_other_sources +=                \
    adio/ad_testfs/ad_testfs_close.c  \
    adio/ad_testfs/ad_testfs_read.c   \
    adio/ad_testfs/ad_testfs_rdcoll.c \
    adio/ad_testfs/ad_testfs_wrcoll.c \
    adio/ad_testfs/ad_testfs_open.c   \
    adio/ad_testfs/ad_testfs_write.c  \
    adio/ad_testfs/ad_testfs_done.c   \
    adio/ad_testfs/ad_testfs_fcntl.c  \
    adio/ad_testfs/ad_testfs_iread.c  \
    adio/ad_testfs/ad_testfs_iwrite.c \
    adio/ad_testfs/ad_testfs_wait.c   \
    adio/ad_testfs/ad_testfs_flush.c  \
    adio/ad_testfs/ad_testfs_seek.c   \
    adio/ad_testfs/ad_testfs_resize.c \
    adio/ad_testfs/ad_testfs_hints.c  \
    adio/ad_testfs/ad_testfs_delete.c \
    adio/ad_testfs/ad_testfs.c

endif BUILD_AD_TESTFS

if BUILD_AD_UFS

noinst_HEADERS += adio/ad_ufs/ad_ufs.h

romio_other_sources +=        \
    adio/ad_ufs/ad_ufs.c      \
    adio/ad_ufs/ad_ufs_open.c

endif BUILD_AD_UFS

if BUILD_AD_XFS

noinst_HEADERS += adio/ad_xfs/ad_xfs.h

romio_other_sources +=          \
    adio/ad_xfs/ad_xfs.c        \
    adio/ad_xfs/ad_xfs_fcntl.c  \
    adio/ad_xfs/ad_xfs_hints.c  \
    adio/ad_xfs/ad_xfs_open.c   \
    adio/ad_xfs/ad_xfs_read.c   \
    adio/ad_xfs/ad_xfs_resize.c \
    adio/ad_xfs/ad_xfs_write.c

endif BUILD_AD_XFS

if BUILD_AD_IME

noinst_HEADERS += adio/ad_ime/ad_ime.h adio/ad_ime/ad_ime_common.h

romio_other_sources +=                    \
    adio/ad_ime/ad_ime.c            \
    adio/ad_ime/ad_ime_close.c      \
    adio/ad_ime/ad_ime_common.c     \
    adio/ad_ime/ad_ime_delete.c     \
    adio/ad_ime/ad_ime_fcntl.c      \
    adio/ad_ime/ad_ime_flush.c      \
    adio/ad_ime/ad_ime_io.c         \
    adio/ad_ime/ad_ime_open.c       \
    adio/ad_ime/ad_ime_resize.c     \
    adio/ad_ime/ad_ime_features.c

endif BUILD_AD_IME


romio_other_sources +=                  \
    adio/common/ad_aggregate.c          \
    adio/common/ad_aggregate_new.c      \
    adio/common/ad_close.c              \
    adio/common/ad_coll_build_req_new.c \
    adio/common/ad_coll_exch_new.c      \
    adio/common/ad_darray.c             \
    adio/common/ad_delete.c             \
    adio/common/ad_done.c               \
    adio/common/ad_done_fake.c          \
    adio/common/ad_end.c                \
    adio/common/ad_fcntl.c              \
    adio/common/ad_features.c           \
    adio/common/ad_flush.c              \
    adio/common/ad_fstype.c             \
    adio/common/ad_get_sh_fp.c          \
    adio/common/ad_hints.c              \
    adio/common/ad_init.c               \
    adio/common/ad_io_coll.c            \
    adio/common/ad_iopen.c              \
    adio/common/ad_iread.c              \
    adio/common/ad_iread_coll.c         \
    adio/common/ad_iread_fake.c         \
    adio/common/ad_iwrite.c             \
    adio/common/ad_iwrite_coll.c        \
    adio/common/ad_iwrite_fake.c        \
    adio/common/ad_open.c               \
    adio/common/ad_opencoll.c           \
    adio/common/ad_opencoll_failsafe.c  \
    adio/common/ad_opencoll_scalable.c  \
    adio/common/ad_prealloc.c           \
    adio/common/ad_read.c               \
    adio/common/ad_read_coll.c          \
    adio/common/ad_read_str.c           \
    adio/common/ad_read_str_naive.c     \
    adio/common/ad_resize.c             \
    adio/common/ad_seek.c               \
    adio/common/ad_set_sh_fp.c          \
    adio/common/ad_set_view.c           \
    adio/common/ad_subarray.c           \
    adio/common/ad_wait.c               \
    adio/common/ad_wait_fake.c          \
    adio/common/ad_write.c              \
    adio/common/ad_write_coll.c         \
    adio/common/ad_write_nolock.c       \
    adio/common/ad_write_str.c          \
    adio/common/ad_write_str_naive.c    \
    adio/common/adi_close.c             \
    adio/common/byte_offset.c           \
    adio/common/cb_config_list.c        \
    adio/common/eof_offset.c            \
    adio/common/error.c                 \
    adio/common/flatten.c               \
    adio/common/get_fp_posn.c           \
    adio/common/greq_fns.c              \
    adio/common/heap_sort.c             \
    adio/common/iscontig.c              \
    adio/common/lock.c                  \
    adio/common/malloc.c                \
    adio/common/shfp_fname.c            \
    adio/common/status_setb.c           \
    adio/common/strfns.c                \
    adio/common/system_hints.c          \
    adio/common/hint_fns.c              \
    adio/common/ad_threaded_io.c        \
    adio/common/p2p_aggregation.c       \
    adio/common/onesided_aggregation.c  \
    adio/common/ad_tuning.c             \
    adio/common/utils.c


# Build a libtool convenience library that the enclosing MPI implementation can
# use by adding it to the right _LIBADD variable.
noinst_LTLIBRARIES = libromio.la
libromio_la_SOURCES = $(romio_mpi_sources) $(romio_other_sources) $(mpio_extra_sources)

noinst_LTLIBRARIES += libpromio.la

libpromio_la_SOURCES = $(romio_mpi_sources)
libpromio_la_CPPFLAGS = $(AM_CPPFLAGS) -DMPIO_BUILD_PROFILING
