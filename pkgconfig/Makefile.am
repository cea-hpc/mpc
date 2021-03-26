
mpcpkgconfigdir=$(libdir)/pkgconfig/

mpcpkgconfig_DATA = libmpclowcomm.pc

if MPC_THREAD_ENABLED
  mpcpkgconfig_DATA += libmpcthread.pc
endif

if MPC_MPI_ENABLED
  mpcpkgconfig_DATA += libmpcmpi.pc
endif