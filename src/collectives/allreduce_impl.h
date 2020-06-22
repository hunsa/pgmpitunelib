
#ifndef SRC_COLLECTIVES_ALLREDUCE_IMPL_H_
#define SRC_COLLECTIVES_ALLREDUCE_IMPL_H_

#include <mpi.h>

int MPI_Allreduce_as_Reduce_Bcast(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int MPI_Allreduce_as_Reduce_scatter_Allgatherv(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int MPI_Allreduce_as_Reduce_scatter_block_Allgather(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

#endif /* SRC_COLLECTIVES_ALLREDUCE_IMPL_H_ */
