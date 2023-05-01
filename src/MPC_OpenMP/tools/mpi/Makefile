MPC_CXX=mpc_cc -cc=gcc
CXX=I_MPI_CXX="$(MPC_CXX)" MPICH_CXX="$(MPC_CXX)" OMPI_CXX="$(MPC_CXX)" mpicxx

SRCS=trace.cc
OBJS=$(SRCS:.cc=.o)

CFLAGS=-Wall -Wextra
LDFLAGS=-shared
TARGET=libmpcmpitrace.so

all: $(TARGET)

%.o: %.cc
	$(CXX) $(CFLAGS) -o $@ -c $< -fPIC

$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET)
