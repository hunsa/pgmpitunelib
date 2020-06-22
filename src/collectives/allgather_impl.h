
#ifndef SRC_COLLECTIVES_ALLGATHER_IMPL_H_
#define SRC_COLLECTIVES_ALLGATHER_IMPL_H_

#include <mpi.h>

int MPI_Allgather_as_Allgatherv(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm);

int MPI_Allgather_as_Allreduce(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm);

int MPI_Allgather_as_Alltoall(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm);

int MPI_Allgather_as_GatherBcast(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm);

#endif /* SRC_COLLECTIVES_ALLGATHER_IMPL_H_ */
