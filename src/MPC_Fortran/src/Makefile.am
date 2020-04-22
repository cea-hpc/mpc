AM_CPPFLAGS = -I$(abs_top_srcdir)/src/MPC_Common/include/ -I$(abs_top_srcdir)/src/include/ -I$(abs_top_builddir)/src/MPC_Arch/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Arch/include/ -I$(abs_top_srcdir)/src/MPC_Threads/include/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Config/include/ -I$(abs_top_srcdir)/src/MPC_Config/src/generated/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Topology/include/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/include/ -I$(abs_top_srcdir)/src/MPC_Message_Passing/src/
AM_CPPFLAGS += -I$(abs_top_srcdir)/src/MPC_Launch/include/ -I$(abs_top_srcdir)/src/MPC_MPI/include/  -I$(abs_top_builddir)/src/include/

MPC_DEFAULT_C_COMPILER="@MPC_DEFAULT_C_COMPILER@"
MPC_DEFAULT_FC_COMPILER="@MPC_DEFAULT_FC_COMPILER@"

all:
	$(abs_top_srcdir)/src/MPC_Fortran/src/fortran_build.sh "$(MPC_DEFAULT_C_COMPILER)" "$(MPC_DEFAULT_FC_COMPILER)" "$(AM_CPPFLAGS)" "$(abs_top_builddir)/src/MPC_Fortran/src/"

