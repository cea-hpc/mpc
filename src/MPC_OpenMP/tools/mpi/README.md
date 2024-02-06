# WIP
This is work in progress, to be standardized through PMPI.
Currently, one should directly modify its code using
```
MPI_Request req;
MPI_Status status;

MPC_OMP_TASK_TRACE_ALLREDUCE(1, 0, 0, 0, 0);
MPI_Iallreduce(gnewdt, newdt, 1, size, MPI_MIN, MPI_COMM_WORLD, &req);
MPIX_Wait(&req, &status);
MPC_OMP_TASK_TRACE_ALLREDUCE(1, 0, 0, 0, 1);
```

# How to compile
Edit Makefile to your conveniance then type
> make

# How to link with your app
Add `-L/path/to/the/lib -lmpcmpitrace` to the linker flags and `export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/the/lib`
