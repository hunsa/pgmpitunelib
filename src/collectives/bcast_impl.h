
#ifndef SRC_COLLECTIVES_BCAST_IMPL_H_
#define SRC_COLLECTIVES_BCAST_IMPL_H_

#include <mpi.h>

int MPI_Bcast_as_Allgatherv(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

int MPI_Bcast_as_Scatter_Allgather(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

#endif /* SRC_COLLECTIVES_BCAST_IMPL_H_ */
