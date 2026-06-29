# MPC_IO/src/romio/CMakeLists.txt
# Builds libromio.a and libpromio.a (static, noinst)

# Generate internal ROMIO mpio.h (use same template as autotools)
configure_file(
    "${CMAKE_SOURCE_DIR}/src/MPC_IO/include/mpio.h.in"
    "${CMAKE_BINARY_DIR}/src/MPC_IO/include/mpio.h"
    @ONLY
)

# ------------------------------------------------------------------
# ROMIO source lists
# ------------------------------------------------------------------

# MPICH glue (default, matches autotools MPIO_GLUE_MPICH=true)
set(ROMIO_GLUE_SOURCES
    mpi-io/glue/mpich/mpio_file.c
    mpi-io/glue/mpich/mpio_err.c
)

# large_count.c provides fallback MPI_Type_size_x when ROMIO is built
# standalone against an external MPI.  When embedded in MPC (default,
# matching autotools BUILD_ROMIO_EMBEDDED=true), it is excluded to
# avoid infinite recursion through the internal MPI implementation.
if(NOT BUILD_ROMIO_EMBEDDED)
    list(APPEND ROMIO_GLUE_SOURCES mpi-io/glue/large_count.c)
endif()

# MPI-I/O API sources (always)
set(ROMIO_MPI_SOURCES
    mpi-io/close.c
    mpi-io/delete.c
    mpi-io/file_c2f.c
    mpi-io/file_f2c.c
    mpi-io/fsync.c
    mpi-io/get_amode.c
    mpi-io/get_atom.c
    mpi-io/get_bytoff.c
    mpi-io/get_extent.c
    mpi-io/get_group.c
    mpi-io/get_info.c
    mpi-io/get_posn.c
    mpi-io/get_posn_sh.c
    mpi-io/get_size.c
    mpi-io/get_view.c
    mpi-io/iread.c
    mpi-io/iread_all.c
    mpi-io/iread_at.c
    mpi-io/iread_atall.c
    mpi-io/iread_sh.c
    mpi-io/iwrite.c
    mpi-io/iwrite_all.c
    mpi-io/iwrite_at.c
    mpi-io/iwrite_atall.c
    mpi-io/iwrite_sh.c
    mpi-io/open.c
    mpi-io/prealloc.c
    mpi-io/rd_atallb.c
    mpi-io/rd_atalle.c
    mpi-io/read.c
    mpi-io/read_all.c
    mpi-io/read_allb.c
    mpi-io/read_alle.c
    mpi-io/read_at.c
    mpi-io/read_atall.c
    mpi-io/read_ord.c
    mpi-io/read_ordb.c
    mpi-io/read_orde.c
    mpi-io/read_sh.c
    mpi-io/register_datarep.c
    mpi-io/seek.c
    mpi-io/seek_sh.c
    mpi-io/set_atom.c
    mpi-io/set_info.c
    mpi-io/set_size.c
    mpi-io/set_view.c
    mpi-io/wr_atallb.c
    mpi-io/wr_atalle.c
    mpi-io/write.c
    mpi-io/write_all.c
    mpi-io/write_allb.c
    mpi-io/write_alle.c
    mpi-io/write_at.c
    mpi-io/write_atall.c
    mpi-io/write_ord.c
    mpi-io/write_ordb.c
    mpi-io/write_orde.c
    mpi-io/write_sh.c
)

# Non-MPI utility sources (always)
set(ROMIO_OTHER_SOURCES
    mpi-io/mpich_fileutil.c
    mpi-io/mpir-mpioinit.c
    mpi-io/mpiu_greq.c
    mpi-io/mpiu_external32.c
    mpi-io/mpir_cst_filesys.c
    mpi-io/get_errh.c
    mpi-io/set_errh.c
    mpi-io/ioreq_c2f.c
    mpi-io/ioreq_f2c.c
)

# ADIO common sources (always)
set(ROMIO_ADIO_COMMON_SOURCES
    adio/common/ad_aggregate.c
    adio/common/ad_aggregate_new.c
    adio/common/ad_close.c
    adio/common/ad_coll_build_req_new.c
    adio/common/ad_coll_exch_new.c
    adio/common/ad_darray.c
    adio/common/ad_delete.c
    adio/common/ad_done.c
    adio/common/ad_done_fake.c
    adio/common/ad_end.c
    adio/common/ad_fcntl.c
    adio/common/ad_features.c
    adio/common/ad_flush.c
    adio/common/ad_fstype.c
    adio/common/ad_get_sh_fp.c
    adio/common/ad_hints.c
    adio/common/ad_init.c
    adio/common/ad_io_coll.c
    adio/common/ad_iopen.c
    adio/common/ad_iread.c
    adio/common/ad_iread_coll.c
    adio/common/ad_iread_fake.c
    adio/common/ad_iwrite.c
    adio/common/ad_iwrite_coll.c
    adio/common/ad_iwrite_fake.c
    adio/common/ad_open.c
    adio/common/ad_opencoll.c
    adio/common/ad_opencoll_failsafe.c
    adio/common/ad_opencoll_scalable.c
    adio/common/ad_prealloc.c
    adio/common/ad_read.c
    adio/common/ad_read_coll.c
    adio/common/ad_read_str.c
    adio/common/ad_read_str_naive.c
    adio/common/ad_resize.c
    adio/common/ad_seek.c
    adio/common/ad_set_sh_fp.c
    adio/common/ad_set_view.c
    adio/common/ad_subarray.c
    adio/common/ad_threaded_io.c
    adio/common/ad_tuning.c
    adio/common/ad_wait.c
    adio/common/ad_wait_fake.c
    adio/common/ad_write.c
    adio/common/ad_write_coll.c
    adio/common/ad_write_nolock.c
    adio/common/ad_write_str.c
    adio/common/ad_write_str_naive.c
    adio/common/adi_close.c
    adio/common/byte_offset.c
    adio/common/cb_config_list.c
    adio/common/eof_offset.c
    adio/common/error.c
    adio/common/flatten.c
    adio/common/get_fp_posn.c
    adio/common/greq_fns.c
    adio/common/heap_sort.c
    adio/common/hint_fns.c
    adio/common/iscontig.c
    adio/common/lock.c
    adio/common/malloc.c
    adio/common/onesided_aggregation.c
    adio/common/p2p_aggregation.c
    adio/common/shfp_fname.c
    adio/common/status_setb.c
    adio/common/strfns.c
    adio/common/system_hints.c
    adio/common/utils.c
)

# NFS driver (enabled by default, matches autotools)
set(ROMIO_NFS_SOURCES
    adio/ad_nfs/ad_nfs.c
    adio/ad_nfs/ad_nfs_done.c
    adio/ad_nfs/ad_nfs_fcntl.c
    adio/ad_nfs/ad_nfs_features.c
    adio/ad_nfs/ad_nfs_getsh.c
    adio/ad_nfs/ad_nfs_hints.c
    adio/ad_nfs/ad_nfs_iread.c
    adio/ad_nfs/ad_nfs_iwrite.c
    adio/ad_nfs/ad_nfs_open.c
    adio/ad_nfs/ad_nfs_read.c
    adio/ad_nfs/ad_nfs_resize.c
    adio/ad_nfs/ad_nfs_setsh.c
    adio/ad_nfs/ad_nfs_wait.c
    adio/ad_nfs/ad_nfs_write.c
)

# UFS driver (enabled by default, matches autotools)
set(ROMIO_UFS_SOURCES
    adio/ad_ufs/ad_ufs.c
    adio/ad_ufs/ad_ufs_open.c
)

# Fortran bindings (conditional on MPC_ENABLE_FORTRAN)
set(ROMIO_FORTRAN_SOURCES
    mpi-io/fortran/closef.c
    mpi-io/fortran/deletef.c
    mpi-io/fortran/fsyncf.c
    mpi-io/fortran/get_amodef.c
    mpi-io/fortran/get_atomf.c
    mpi-io/fortran/get_bytofff.c
    mpi-io/fortran/get_errhf.c
    mpi-io/fortran/get_extentf.c
    mpi-io/fortran/get_groupf.c
    mpi-io/fortran/get_infof.c
    mpi-io/fortran/get_posn_shf.c
    mpi-io/fortran/get_posnf.c
    mpi-io/fortran/get_sizef.c
    mpi-io/fortran/get_viewf.c
    mpi-io/fortran/iotestf.c
    mpi-io/fortran/iowaitf.c
    mpi-io/fortran/iread_atf.c
    mpi-io/fortran/iread_shf.c
    mpi-io/fortran/ireadf.c
    mpi-io/fortran/iwrite_atf.c
    mpi-io/fortran/iwrite_shf.c
    mpi-io/fortran/iwritef.c
    mpi-io/fortran/openf.c
    mpi-io/fortran/preallocf.c
    mpi-io/fortran/rd_atallbf.c
    mpi-io/fortran/rd_atallef.c
    mpi-io/fortran/read_allbf.c
    mpi-io/fortran/read_allef.c
    mpi-io/fortran/read_allf.c
    mpi-io/fortran/read_atallf.c
    mpi-io/fortran/read_atf.c
    mpi-io/fortran/read_ordbf.c
    mpi-io/fortran/read_ordef.c
    mpi-io/fortran/read_ordf.c
    mpi-io/fortran/read_shf.c
    mpi-io/fortran/readf.c
    mpi-io/fortran/seek_shf.c
    mpi-io/fortran/seekf.c
    mpi-io/fortran/set_atomf.c
    mpi-io/fortran/set_errhf.c
    mpi-io/fortran/set_infof.c
    mpi-io/fortran/set_sizef.c
    mpi-io/fortran/set_viewf.c
    mpi-io/fortran/wr_atallbf.c
    mpi-io/fortran/wr_atallef.c
    mpi-io/fortran/write_allbf.c
    mpi-io/fortran/write_allef.c
    mpi-io/fortran/write_allf.c
    mpi-io/fortran/write_atallf.c
    mpi-io/fortran/write_atf.c
    mpi-io/fortran/write_ordbf.c
    mpi-io/fortran/write_ordef.c
    mpi-io/fortran/write_ordf.c
    mpi-io/fortran/write_shf.c
    mpi-io/fortran/writef.c
)

# ------------------------------------------------------------------
# libromio (static, noinst)
# ------------------------------------------------------------------

set(ROMIO_ALL_SOURCES
    ${ROMIO_GLUE_SOURCES}
    ${ROMIO_MPI_SOURCES}
    ${ROMIO_OTHER_SOURCES}
    ${ROMIO_ADIO_COMMON_SOURCES}
    ${ROMIO_NFS_SOURCES}
    ${ROMIO_UFS_SOURCES}
)

if(MPC_ENABLE_FORTRAN)
    list(APPEND ROMIO_ALL_SOURCES ${ROMIO_FORTRAN_SOURCES})
endif()

add_library(romio OBJECT ${ROMIO_ALL_SOURCES})
set_target_properties(romio PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(romio PRIVATE -include limits.h)

target_include_directories(romio PRIVATE
    ${CMAKE_SOURCE_DIR}/src/MPC_IO/include
    ${CMAKE_SOURCE_DIR}/src/MPC_IO/src
    ${CMAKE_CURRENT_SOURCE_DIR}/adio/include
    ${CMAKE_BINARY_DIR}/src/MPC_IO/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/mpi-io
    ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
    ${CMAKE_BINARY_DIR}/src/MPC_Common/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
    ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_BINARY_DIR}/src/include
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(romio PRIVATE mpcmpi mpcarch ${OPA_LIBRARIES} Threads::Threads)
target_link_options(romio PRIVATE ${OPA_LDFLAGS})

# ------------------------------------------------------------------
# libpromio (static, noinst, profiling variant)
# ------------------------------------------------------------------

set(PROMIO_ALL_SOURCES
    ${ROMIO_MPI_SOURCES}
)

add_library(promio OBJECT ${PROMIO_ALL_SOURCES})
set_target_properties(promio PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(promio PRIVATE MPIO_BUILD_PROFILING)
target_compile_options(promio PRIVATE -include limits.h)

target_include_directories(promio PRIVATE
    ${CMAKE_SOURCE_DIR}/src/MPC_IO/include
    ${CMAKE_SOURCE_DIR}/src/MPC_IO/src
    ${CMAKE_CURRENT_SOURCE_DIR}/adio/include
    ${CMAKE_BINARY_DIR}/src/MPC_IO/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/mpi-io
    ${CMAKE_SOURCE_DIR}/src/MPC_Common/include
    ${CMAKE_BINARY_DIR}/src/MPC_Common/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Arch/include
    ${CMAKE_BINARY_DIR}/src/MPC_Arch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_MPI/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Lowcomm/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Launch/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Topology/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Config/include
    ${CMAKE_SOURCE_DIR}/src/MPC_Threads/include
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_BINARY_DIR}/src/include
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(promio PRIVATE mpcmpi mpcarch ${OPA_LIBRARIES} Threads::Threads)
target_link_options(promio PRIVATE ${OPA_LDFLAGS})
