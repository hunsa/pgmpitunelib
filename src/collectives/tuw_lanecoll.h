/* (C) Jesper Larsson Traff, September, October 2019 */
/* Full-lane collectives, mock-up performance guideline implementations */

/* 2020 Sascha Hunold */

/* See "Decomposing Collectives for Exploiting Multi-lane Communication" */

#ifndef _LANECOLLECTIVES
#define _LANECOLLECTIVES

#include <mpi.h>

int Get_Lane_comms(MPI_Comm comm, MPI_Comm *nodecomm, MPI_Comm *lanecomm);

int Alltoall_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm);

int Allgather_hier(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm);

int Allgather_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm);

int Allgather_lane_zerocopy(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm);

int Allreduce_hier(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Allreduce_lane(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Bcast_hier(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

int Bcast_lane(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

int Exscan_hier(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Exscan_lane(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Gather_hier(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm);

int Gather_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm);


int Reduce_hier(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int Reduce_lane(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int Reduce_scatter_block_hier(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Reduce_scatter_block_lane(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Scan_hier(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Scan_lane(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int Scatter_hier(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm);

int Scatter_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm);

#endif
