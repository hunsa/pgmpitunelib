
#ifndef SRC_COLLECTIVES_GATHER_IMPL_H_
#define SRC_COLLECTIVES_GATHER_IMPL_H_

#include <mpi.h>

int MPI_Gather_as_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                            void* recvbuf, int recvcount, MPI_Datatype recvtype,
                            int root, MPI_Comm comm);

int MPI_Gather_as_Gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                          void* recvbuf, int recvcount, MPI_Datatype recvtype,
                          int root, MPI_Comm comm);

int MPI_Gather_as_Reduce(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                         void* recvbuf, int recvcount, MPI_Datatype recvtype,
                         int root, MPI_Comm comm);


#endif /* SRC_COLLECTIVES_GATHER_IMPL_H_ */
