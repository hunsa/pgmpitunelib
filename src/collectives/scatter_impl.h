
#ifndef SRC_COLLECTIVES_SCATTER_IMPL_H_
#define SRC_COLLECTIVES_SCATTER_IMPL_H_

#include <mpi.h>

int MPI_Scatter_as_Bcast(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm);

int MPI_Scatter_as_Scatterv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm);

#endif /* SRC_COLLECTIVES_SCATTER_IMPL_H_ */
