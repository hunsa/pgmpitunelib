
#ifndef SRC_COLLECTIVES_REDUCE_SCATTER_BLOCK_IMPL_H_
#define SRC_COLLECTIVES_REDUCE_SCATTER_BLOCK_IMPL_H_

#include <mpi.h>

int MPI_Reduce_scatter_block_as_Reduce_Scatter(const void* sendbuf, void* recvbuf, const int recvcount,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int MPI_Reduce_scatter_block_as_Reduce_scatter(const void* sendbuf, void* recvbuf, const int recvcount,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int MPI_Reduce_scatter_block_as_Allreduce(const void* sendbuf, void* recvbuf, const int recvcount,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);


#endif /* SRC_COLLECTIVES_REDUCE_SCATTER_BLOCK_IMPL_H_ */
