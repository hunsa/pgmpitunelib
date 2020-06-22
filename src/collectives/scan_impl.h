
#ifndef SRC_COLLECTIVES_SCAN_IMPL_H_
#define SRC_COLLECTIVES_SCAN_IMPL_H_

#include <mpi.h>

int MPI_Scan_as_Exscan_Reduce_local(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
    MPI_Comm comm);


#endif /* SRC_COLLECTIVES_SCAN_IMPL_H_ */
