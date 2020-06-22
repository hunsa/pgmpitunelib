
#ifndef SRC_COLLECTIVES_ALLTOALL_IMPL_H_
#define SRC_COLLECTIVES_ALLTOALL_IMPL_H_

#include <mpi.h>

int MPI_Alltoall_as_Alltoallv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm);

#endif /* SRC_COLLECTIVES_ALLTOALL_IMPL_H_ */
