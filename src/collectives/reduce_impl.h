
#ifndef SRC_COLLECTIVES_REDUCE_IMPL_H_
#define SRC_COLLECTIVES_REDUCE_IMPL_H_

#include <mpi.h>

int MPI_Reduce_as_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
    MPI_Comm comm);

int MPI_Reduce_as_Reduce_scatter_block_Gather(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype,
    MPI_Op op, int root, MPI_Comm comm);

int MPI_Reduce_as_Reduce_scatter_Gatherv(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype,
    MPI_Op op, int root, MPI_Comm comm);

#endif /* SRC_COLLECTIVES_REDUCE_IMPL_H_ */
