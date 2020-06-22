/* (C) Jesper Larsson Traff, September, October 2019 */
/* Full-lane collectives, mock-up performance guideline implementations */
/* Version 0.9 */
/* See "Decomposing Collectives for Exploiting Multi-lane Communication" */
/* 2020 Sascha Hunold */

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#include "tuw_lanecoll.h"

#define SELFCOPY 5000
#define REGULAR  6000

typedef struct {
  MPI_Comm lanecomm;
  MPI_Comm nodecomm;
} laneattr;

static int lanedel(MPI_Comm comm, int keyval, void *attr, void *s) {
  laneattr *decomposition = (laneattr*) attr;

  MPI_Comm_free(&decomposition->nodecomm);
  MPI_Comm_free(&decomposition->lanecomm);

  return MPI_SUCCESS;
}

static int lanekey() {
  // hidden key value for type attributes
  static int lanekeyval = MPI_KEYVAL_INVALID;

  if (lanekeyval == MPI_KEYVAL_INVALID) {
    MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN, lanedel, &lanekeyval, NULL);
  }

  return lanekeyval;
}

int laneinit(MPI_Comm comm) {
  MPI_Comm nodecomm, lanecomm;

  int rank, size;
  int noderank, prevrank, nodesize, maxnsize;

  int isregcon; // communicator is regular and consecutively numbered

  laneattr *decomposition;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &nodecomm);
  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);

  // Check for regularity
  if (nodesize == size) {
    isregcon = 0;
  } else {
    MPI_Allreduce(&nodesize, &maxnsize, 1, MPI_INT, MPI_MAX, comm);
    isregcon = (nodesize == maxnsize);

    MPI_Sendrecv(&noderank, 1, MPI_INT, (rank + 1) % size, REGULAR, &prevrank, 1, MPI_INT, (rank - 1 + size) % size,
        REGULAR, comm, MPI_STATUS_IGNORE);

    isregcon = isregcon && (noderank == (prevrank + 1) % maxnsize);

    MPI_Allreduce(MPI_IN_PLACE, &isregcon, 1, MPI_INT, MPI_LAND, comm);
  }

  if (isregcon) {
    MPI_Comm_split(comm, noderank, 0, &lanecomm);
  } else {
    MPI_Comm_free(&nodecomm);
    MPI_Comm_dup(MPI_COMM_SELF, &nodecomm);
    MPI_Comm_dup(comm, &lanecomm);
  }

  decomposition = (laneattr*) malloc(sizeof(laneattr));
  //assert(decomposition != NULL);
  decomposition->nodecomm = nodecomm;
  decomposition->lanecomm = lanecomm;

  MPI_Comm_set_attr(comm, lanekey(), decomposition);

  return MPI_SUCCESS;
}

// MPI datatyped memory allocation with correct extent
void *mpitalloc(int count, MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint * extent) {
  MPI_Datatype fulltype;
  MPI_Aint space;
  int size;

  void *tempbuf;

  MPI_Type_get_extent(datatype, lb, extent);
  MPI_Type_size(datatype, &size);

  if ((MPI_Aint) size == *extent) {
    space = *extent * count;
  } else {
    MPI_Type_contiguous(count, datatype, &fulltype);
    MPI_Type_get_true_extent(fulltype, lb, &space);
    MPI_Type_free(&fulltype);
  }

  tempbuf = (void*) malloc(space);
  //assert(tempbuf != NULL);
  //tempbuf = (void*)((MPI_Aint*)tempbuf-lb);

  return tempbuf;
}

int Get_Lane_comms(MPI_Comm comm, MPI_Comm *nodecomm, MPI_Comm *lanecomm) {
  laneattr *decomposition;
  int flag;

  MPI_Comm_get_attr(comm, lanekey(), &decomposition, &flag);
  if (!flag) {
    laneinit(comm);
    MPI_Comm_get_attr(comm, lanekey(), &decomposition, &flag);
    //assert(flag);
  }

  *nodecomm = decomposition->nodecomm;
  *lanecomm = decomposition->lanecomm;

  return MPI_SUCCESS;
}

int Allgather_hier(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int rank, size, noderank, nodesize, lanerank;

  const void *takebuf;
  int takecount;
  MPI_Datatype taketype;

  MPI_Aint lb, extent;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  MPI_Type_get_extent(recvtype, &lb, &extent);

  if (sendbuf == MPI_IN_PLACE && noderank != 0) {
    takebuf = (char*) recvbuf + rank * recvcount * extent;
    takecount = recvcount;
    taketype = recvtype;
  } else {
    takebuf = sendbuf;
    takecount = sendcount;
    taketype = sendtype;
  }

  PMPI_Gather(takebuf, takecount, taketype, (char*) recvbuf + lanerank * nodesize * recvcount * extent, recvcount,
      recvtype, 0, nodecomm);

  if (noderank == 0) {
    PMPI_Allgather(MPI_IN_PLACE, nodesize * recvcount, recvtype, recvbuf, nodesize * recvcount, recvtype, lanecomm);
  }

  PMPI_Bcast(recvbuf, size * recvcount, recvtype, 0, nodecomm);

  return MPI_SUCCESS;
}

int Allgather_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int rank, size;
  int lanerank, lanesize, noderank, nodesize;

  MPI_Aint lb, extent;

  void *tempbuf;

  MPI_Datatype nt1, nt2, nodetype;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(lanecomm, &lanerank);
  MPI_Comm_size(lanecomm, &lanesize);
  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);

  MPI_Type_get_extent(recvtype, &lb, &extent);

  tempbuf = mpitalloc(size * recvcount, recvtype, &lb, &extent);

  if (sendbuf != MPI_IN_PLACE) {
    MPI_Sendrecv(sendbuf, sendcount, sendtype, 0, SELFCOPY,
        (char*) tempbuf + (lanerank + noderank * lanesize) * recvcount * extent, recvcount, recvtype, 0, SELFCOPY,
        MPI_COMM_SELF, MPI_STATUS_IGNORE);
  } else {
    MPI_Sendrecv((char*) recvbuf + rank * recvcount * extent, recvcount, recvtype, 0, SELFCOPY,
        (char*) tempbuf + (lanerank + noderank * lanesize) * recvcount * extent, recvcount, recvtype, 0, SELFCOPY,
        MPI_COMM_SELF, MPI_STATUS_IGNORE);
  }

  PMPI_Allgather(MPI_IN_PLACE, sendcount, sendtype, (char*) tempbuf + noderank * lanesize * recvcount * extent,
      recvcount, recvtype, lanecomm);

  PMPI_Allgather(MPI_IN_PLACE, sendcount, sendtype, tempbuf, lanesize * recvcount, recvtype, nodecomm);

  MPI_Type_vector(lanesize, recvcount, nodesize * recvcount, recvtype, &nt1);
  MPI_Type_create_resized(nt1, 0, recvcount * extent, &nt2);
  MPI_Type_contiguous(nodesize, nt2, &nodetype);
  MPI_Type_commit(&nodetype);

  MPI_Sendrecv(tempbuf, size * recvcount, recvtype, 0, SELFCOPY, recvbuf, 1, nodetype, 0, SELFCOPY,
  MPI_COMM_SELF, MPI_STATUS_IGNORE);

  free(tempbuf);

  MPI_Type_free(&nt1);
  MPI_Type_free(&nt2);
  MPI_Type_free(&nodetype);

  return MPI_SUCCESS;
}


int Allgather_lane_zerocopy(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm)
{
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int lanesize, noderank, nodesize;

  MPI_Aint lb, extent;

  MPI_Datatype nt, nodetype;
  MPI_Datatype lt, lanetype;

  if (nodecomm==MPI_COMM_NULL) {
    Get_Lane_comms(comm,&nodecomm,&lanecomm);
  }

  MPI_Comm_size(lanecomm,&lanesize);
  MPI_Comm_rank(nodecomm,&noderank);
  MPI_Comm_size(nodecomm,&nodesize);

  MPI_Type_get_extent(recvtype,&lb,&extent);

  MPI_Type_contiguous(recvcount,recvtype,&lt);
  MPI_Type_create_resized(lt,0,nodesize*recvcount*extent,&lanetype);
  MPI_Type_commit(&lanetype);

  MPI_Type_vector(lanesize,recvcount,nodesize*recvcount,recvtype,&nt);
  MPI_Type_create_resized(nt,0,recvcount*extent,&nodetype);
  MPI_Type_commit(&nodetype);

  PMPI_Allgather(sendbuf,sendcount,sendtype,
      (char*)recvbuf+noderank*recvcount*extent,1,lanetype,
      lanecomm);

  PMPI_Allgather(MPI_IN_PLACE,sendcount,sendtype,
      recvbuf,1,nodetype,nodecomm);

  MPI_Type_free(&nt);
  MPI_Type_free(&nodetype);

  MPI_Type_free(&lt);
  MPI_Type_free(&lanetype);

  return MPI_SUCCESS;
}

int Allreduce_hier(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int rank, size, noderank, nodesize, lanerank;

  const void *takebuf;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  takebuf = (sendbuf == MPI_IN_PLACE && noderank != 0) ? recvbuf : sendbuf;

  PMPI_Reduce(takebuf, recvbuf, count, datatype, op, 0, nodecomm);

  if (noderank == 0) {
    PMPI_Allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op, lanecomm);
  }

  PMPI_Bcast(recvbuf, count, datatype, 0, nodecomm);

  return MPI_SUCCESS;
}

int Allreduce_lane(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize;
  int i;

  MPI_Aint lb, extent;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);

  MPI_Type_get_extent(datatype, &lb, &extent);

  int block;
  int counts[nodesize];
  int displs[nodesize];

  block = count / nodesize;

  if (count % nodesize == 0) {
    counts[noderank] = block;
    PMPI_Reduce_scatter_block(sendbuf, (char*) recvbuf + noderank * block * extent, block, datatype, op, nodecomm);
  } else {
    for (i = 0; i < nodesize; i++)
      counts[i] = block;
    counts[nodesize - 1] += count % nodesize;

    PMPI_Reduce_scatter(sendbuf, (char*) recvbuf + noderank * block * extent, counts, datatype, op, nodecomm);
  }

  PMPI_Allreduce(MPI_IN_PLACE, (char*) recvbuf + noderank * block * extent, counts[noderank], datatype, op, lanecomm);

  if (count % nodesize == 0) {
    PMPI_Allgather(MPI_IN_PLACE, 0, datatype, recvbuf, block, datatype, nodecomm);
  } else {
    displs[0] = 0;
    for (i = 1; i < nodesize; i++)
      displs[i] = displs[i - 1] + counts[i - 1];

    PMPI_Allgatherv(MPI_IN_PLACE, 0, datatype, recvbuf, counts, displs, datatype, nodecomm);
  }

  return MPI_SUCCESS;
}

int Alltoall_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int size, lanesize, nodesize;

  void *tempbuf;

  MPI_Aint lb, extent;

  MPI_Datatype nt, nodetype;

  MPI_Comm_size(comm, &size);

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_size(lanecomm, &lanesize);
  MPI_Comm_size(nodecomm, &nodesize);

  MPI_Type_get_extent(recvtype, &lb, &extent);

  tempbuf = mpitalloc(size * recvcount, recvtype, &lb, &extent);

  MPI_Type_vector(lanesize, recvcount, nodesize * recvcount, recvtype, &nt);
  MPI_Type_create_resized(nt, 0, recvcount * extent, &nodetype);
  MPI_Type_commit(&nodetype);

  PMPI_Alltoall(sendbuf, nodesize * sendcount, sendtype, tempbuf, nodesize * recvcount, recvtype, lanecomm);

  PMPI_Alltoall(tempbuf, 1, nodetype, recvbuf, 1, nodetype, nodecomm);

  free(tempbuf);

  MPI_Type_free(&nt);
  MPI_Type_free(&nodetype);

  return MPI_SUCCESS;
}


int Bcast_hier(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int size;
  int lanerank, noderank, nodesize;
  int rootnode, noderoot;

  MPI_Comm_size(comm, &size);
  if (count == 0 || size == 1)
    return MPI_SUCCESS;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  if (noderank == noderoot) {
    PMPI_Bcast(buffer, count, datatype, rootnode, lanecomm);
  }
  PMPI_Bcast(buffer, count, datatype, noderoot, nodecomm);

  return MPI_SUCCESS;
}

int Bcast_lane(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int size;
  int lanerank, noderank, nodesize;
  int rootnode, noderoot;
  int i;

  MPI_Aint lb, extent;

  MPI_Comm_size(comm, &size);
  if (count == 0 || size == 1)
    return MPI_SUCCESS;

  MPI_Type_get_extent(datatype, &lb, &extent);

  if (nodecomm == MPI_COMM_NULL) {
    Get_Lane_comms(comm, &nodecomm, &lanecomm);
  }

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  int block, blockcount;
  int counts[nodesize];
  int displs[nodesize];

  block = count / nodesize;

  for (i = 0; i < nodesize; i++) {
    counts[i] = block;
  }
  counts[nodesize - 1] += count % nodesize;

  displs[0] = 0;
  for (i = 1; i < nodesize; i++) {
    displs[i] = displs[i - 1] + counts[i - 1];
  }
  blockcount = counts[noderank];

  if (lanerank == rootnode) {
    void *recbuf = (noderank == noderoot) ? MPI_IN_PLACE :
                                            (char*) buffer + noderank * block * extent;
    if (count % nodesize == 0) {
      PMPI_Scatter(buffer, blockcount, datatype, recbuf, blockcount, datatype, noderoot, nodecomm);
    } else {
      PMPI_Scatterv(buffer, counts, displs, datatype, recbuf, blockcount, datatype, noderoot, nodecomm);
    }
  }
  PMPI_Bcast((char*) buffer + noderank * block * extent, blockcount, datatype, rootnode, lanecomm);

  if (count % nodesize == 0) {
    PMPI_Allgather(MPI_IN_PLACE, 0, datatype, buffer, blockcount, datatype, nodecomm);
  } else {
    PMPI_Allgatherv(MPI_IN_PLACE, 0, datatype, buffer, counts, displs, datatype, nodecomm);
  }

  return MPI_SUCCESS;
}

int Exscan_hier(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize, lanerank;

  MPI_Aint lb, extent;

  void *takebuf, *tempbuf;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  tempbuf = mpitalloc(count, datatype, &lb, &extent);

  takebuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
  if (noderank == nodesize - 1) {
    MPI_Sendrecv(takebuf, count, datatype, 0, SELFCOPY, tempbuf, count, datatype, 0, SELFCOPY,
    MPI_COMM_SELF, MPI_STATUS_IGNORE);
  }

  MPI_Exscan(sendbuf, recvbuf, count, datatype, op, nodecomm);

  if (noderank == nodesize - 1) {
    MPI_Reduce_local(recvbuf, tempbuf, count, datatype, op);
    MPI_Exscan(MPI_IN_PLACE, tempbuf, count, datatype, op, lanecomm);
  }
  if (lanerank > 0) {
    if (noderank == 0) {
      MPI_Bcast(recvbuf, count, datatype, nodesize - 1, nodecomm);
    } else {
      MPI_Bcast(tempbuf, count, datatype, nodesize - 1, nodecomm);
      MPI_Reduce_local(tempbuf, recvbuf, count, datatype, op);
    }
  }

  free(tempbuf);

  return MPI_SUCCESS;
}
int Exscan_lane(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize, lanerank;
  int block;
  int i;

  MPI_Aint lb, extent;

  void *takebuf, *tempbuf;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  int counts[nodesize];
  int displs[nodesize];

  block = count / nodesize;

  for (i = 0; i < nodesize; i++)
    counts[i] = block;
  counts[nodesize - 1] += count % nodesize;
  displs[0] = 0;
  for (i = 1; i < nodesize; i++)
    displs[i] = displs[i - 1] + counts[i - 1];

  tempbuf = mpitalloc(count, datatype, &lb, &extent);

  takebuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
  if (count % nodesize == 0) {
    MPI_Reduce_scatter_block(takebuf, (char*) tempbuf + noderank * block * extent, block, datatype, op, nodecomm);
  } else {
    MPI_Reduce_scatter(takebuf, (char*) tempbuf + noderank * block * extent, counts, datatype, op, nodecomm);
  }

  MPI_Exscan(MPI_IN_PLACE, (char*) tempbuf + noderank * block * extent, counts[noderank], datatype, op, lanecomm);

  MPI_Exscan(sendbuf, recvbuf, count, datatype, op, nodecomm); // could be overlapped
  if (lanerank > 0) {
    if (count % nodesize == 0) {
      if (noderank == 0) {
        MPI_Allgather(tempbuf, block, datatype, recvbuf, block, datatype, nodecomm);
      } else {
        MPI_Allgather(MPI_IN_PLACE, block, datatype, tempbuf, block, datatype, nodecomm);
        MPI_Reduce_local(tempbuf, recvbuf, count, datatype, op);
      }
    } else {
      if (noderank == 0) {
        MPI_Allgatherv(tempbuf, counts[noderank], datatype, recvbuf, counts, displs, datatype, nodecomm);
      } else {
        MPI_Allgatherv(MPI_IN_PLACE, counts[noderank], datatype, tempbuf, counts, displs, datatype, nodecomm);
        MPI_Reduce_local(tempbuf, recvbuf, count, datatype, op);
      }
    }
  }

  free(tempbuf);

  return MPI_SUCCESS;
}

int Gather_hier(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int lanerank, lanesize, noderank, nodesize;
  int rootnode, noderoot;

  MPI_Aint lb, extent;

  void *tempbuf = NULL;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);
  MPI_Comm_size(lanecomm, &lanesize);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  if (lanerank == rootnode) {
    if (noderank == noderoot) {
      MPI_Type_get_extent(recvtype, &lb, &extent);
    } else
      extent = 0;

    PMPI_Gather(sendbuf, sendcount, sendtype, (char*) recvbuf + lanerank * nodesize * recvcount * extent, recvcount,
        recvtype, noderoot, nodecomm);
  } else {
    if (noderank == noderoot) {
      tempbuf = mpitalloc(nodesize * sendcount, sendtype, &lb, &extent);
    }

    PMPI_Gather(sendbuf, sendcount, sendtype, tempbuf, sendcount, sendtype, noderoot, nodecomm);
  }

  if (noderank == noderoot) {
    if (lanerank == rootnode) {
      PMPI_Gather(MPI_IN_PLACE, nodesize * sendcount, sendtype, recvbuf, nodesize * recvcount, recvtype, rootnode,
          lanecomm);
    } else {
      PMPI_Gather(tempbuf, nodesize * sendcount, sendtype, recvbuf, nodesize * recvcount, recvtype, rootnode, lanecomm);

      free(tempbuf);
    }
  }

  return MPI_SUCCESS;
}

int Gather_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int lanerank, lanesize, noderank, nodesize;
  int rootnode, noderoot;

  MPI_Aint lb, extent;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);
  MPI_Comm_size(lanecomm, &lanesize);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  if (lanerank == rootnode) {
    if (noderank == noderoot) {
      // rank==root

      MPI_Datatype nt, nodetype, lt, lanetype;

      MPI_Type_get_extent(recvtype, &lb, &extent);

      MPI_Type_contiguous(recvcount, recvtype, &nt);
      MPI_Type_create_resized(nt, 0, nodesize * recvcount * extent, &nodetype);
      MPI_Type_commit(&nodetype);

      MPI_Type_vector(lanesize, recvcount, nodesize * recvcount, recvtype, &lt);
      MPI_Type_create_resized(lt, 0, recvcount * extent, &lanetype);
      MPI_Type_commit(&lanetype);

      PMPI_Gather(sendbuf, sendcount, sendtype, (char*) recvbuf + noderank * recvcount * extent, 1, nodetype, rootnode,
          lanecomm);
      PMPI_Gather(MPI_IN_PLACE, lanesize * sendcount, sendtype, recvbuf, 1, lanetype, noderoot, nodecomm);

      MPI_Type_free(&nt);
      MPI_Type_free(&nodetype);
      MPI_Type_free(&lt);
      MPI_Type_free(&lanetype);
    } else {
      void *tempbuf;

      tempbuf = mpitalloc(lanesize * sendcount, sendtype, &lb, &extent);

      PMPI_Gather(sendbuf, sendcount, sendtype, tempbuf, sendcount, sendtype, rootnode, lanecomm);
      // only sendbuf is significant
      PMPI_Gather(tempbuf, lanesize * sendcount, sendtype, recvbuf, lanesize * recvcount, recvtype, noderoot, nodecomm);

      free(tempbuf);
    }
  } else {
    // only sendbuf is significant
    PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, rootnode, lanecomm);
  }

  return MPI_SUCCESS;
}

int Reduce_hier(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize, lanerank;
  int rootnode, noderoot;

  MPI_Aint lb, extent;

  void *tempbuf = NULL;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  if (lanerank == rootnode) {
    PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, noderoot, nodecomm);
    tempbuf = MPI_IN_PLACE;
  } else {
    if (noderank == noderoot) {
      tempbuf = mpitalloc(count, datatype, &lb, &extent);
    } else
      tempbuf = recvbuf;
    PMPI_Reduce(sendbuf, tempbuf, count, datatype, op, noderoot, nodecomm);
  }
  if (noderank == noderoot) {
    PMPI_Reduce(tempbuf, recvbuf, count, datatype, op, rootnode, lanecomm);
    if (lanerank != rootnode)
      free(tempbuf);
  }

  return MPI_SUCCESS;
}

int Reduce_lane(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int rank;
  int noderank, nodesize, lanerank;
  int rootnode, noderoot;
  int i;

  MPI_Aint lb, extent;

  void *tempbuf, *takebuf;

  MPI_Comm_rank(comm, &rank);

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  int block;
  int counts[nodesize];
  int displs[nodesize];

  block = count / nodesize;

  takebuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;

  if (count % nodesize == 0) {
    counts[noderank] = block;

    tempbuf = mpitalloc(block, datatype, &lb, &extent);

    PMPI_Reduce_scatter_block(takebuf, tempbuf, block, datatype, op, nodecomm);
  } else {
    for (i = 0; i < nodesize; i++)
      counts[i] = block;
    counts[nodesize - 1] += count % nodesize;

    tempbuf = mpitalloc(counts[noderank], datatype, &lb, &extent);

    PMPI_Reduce_scatter(takebuf, tempbuf, counts, datatype, op, nodecomm);
  }

  if (lanerank == rootnode) {
    PMPI_Reduce(MPI_IN_PLACE, tempbuf, counts[noderank], datatype, op, rootnode, lanecomm);
  } else {
    PMPI_Reduce(tempbuf, tempbuf, counts[noderank], datatype, op, rootnode, lanecomm);
  }

  if (lanerank == rootnode) {
    if (count % nodesize == 0) {
      PMPI_Gather(tempbuf, counts[noderank], datatype, recvbuf, block, datatype, noderoot, nodecomm);
    } else {
      displs[0] = 0;
      for (i = 1; i < nodesize; i++)
        displs[i] = displs[i - 1] + counts[i - 1];

      PMPI_Gatherv(tempbuf, counts[noderank], datatype, recvbuf, counts, displs, datatype, noderoot, nodecomm);
    }
  }

  free(tempbuf);

  return MPI_SUCCESS;
}

int Reduce_scatter_block_hier(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize, size;

  //MPI_Datatype lt, bt, permtype;
  MPI_Aint lb, extent;

  void *tempbuf = NULL;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_size(comm, &size);

  if (noderank == 0) {
    tempbuf = mpitalloc(size * count, datatype, &lb, &extent);
  }

  if (sendbuf == MPI_IN_PLACE) {
    PMPI_Reduce(recvbuf, tempbuf, size * count, datatype, op, 0, nodecomm);
  } else {
    PMPI_Reduce(sendbuf, tempbuf, size * count, datatype, op, 0, nodecomm);
  }

  if (noderank == 0) {
    PMPI_Reduce_scatter_block(MPI_IN_PLACE, tempbuf, nodesize * count, datatype, op, lanecomm);
  }

  PMPI_Scatter(tempbuf, count, datatype, recvbuf, count, datatype, 0, nodecomm);

  if (noderank == 0) {
    free(tempbuf);
  }

  return MPI_SUCCESS;
}

int Reduce_scatter_block_lane(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int nodesize, lanesize;

  MPI_Datatype lt, bt, permtype;
  MPI_Aint lb, extent;

  void *permbuf, *tempbuf;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_size(lanecomm, &lanesize);

  permbuf = mpitalloc(nodesize * lanesize * count, datatype, &lb, &extent);

  //#define USEINPLACE
//#ifndef USEINPLACE
  tempbuf = mpitalloc(lanesize * count, datatype, &lb, &extent);
//#endif

  MPI_Type_vector(lanesize, count, nodesize * count, datatype, &lt);
  MPI_Type_create_resized(lt, 0, count * extent, &bt);
  MPI_Type_contiguous(nodesize, bt, &permtype);
  MPI_Type_commit(&permtype);

  if (sendbuf != MPI_IN_PLACE) {
    MPI_Sendrecv(sendbuf, 1, permtype, 0, SELFCOPY, permbuf, nodesize * lanesize * count, datatype, 0, SELFCOPY,
    MPI_COMM_SELF, MPI_STATUS_IGNORE);
  } else {
    MPI_Sendrecv(recvbuf, 1, permtype, 0, SELFCOPY, permbuf, nodesize * lanesize * count, datatype, 0, SELFCOPY,
    MPI_COMM_SELF, MPI_STATUS_IGNORE);
  }

//#ifndef USEINPLACE
  PMPI_Reduce_scatter_block(permbuf, tempbuf, lanesize * count, datatype, op, nodecomm);
  PMPI_Reduce_scatter_block(tempbuf, recvbuf, count, datatype, op, lanecomm);
//#else
//  MPI_Reduce_scatter_block(MPI_IN_PLACE,permbuf,lanesize*count,datatype,op,nodecomm);
//  MPI_Reduce_scatter_block(permbuf,recvbuf,count,datatype,op,lanecomm);
//#endif

  MPI_Type_free(&lt);
  MPI_Type_free(&bt);
  MPI_Type_free(&permtype);

  free(permbuf);
//#ifndef USEINPLACE
  free(tempbuf);
//#endif

  return MPI_SUCCESS;
}

int Scan_hier(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize, lanerank;

  MPI_Aint lb, extent;

  void *tempbuf;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  tempbuf = mpitalloc(count, datatype, &lb, &extent);

  PMPI_Scan(sendbuf, recvbuf, count, datatype, op, nodecomm);

  if (noderank == nodesize - 1) {
    PMPI_Exscan(recvbuf, tempbuf, count, datatype, op, lanecomm);
  }
  if (lanerank > 0) {
    PMPI_Bcast(tempbuf, count, datatype, nodesize - 1, nodecomm);
    PMPI_Reduce_local(tempbuf, recvbuf, count, datatype, op);
  }

  free(tempbuf);

  return MPI_SUCCESS;
}
int Scan_lane(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int noderank, nodesize, lanerank;
  int block;
  int i;

  MPI_Aint lb, extent;

  const void *takebuf;
  void *tempbuf;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);

  int counts[nodesize];
  int displs[nodesize];

  block = count / nodesize;

  for (i = 0; i < nodesize; i++)
    counts[i] = block;
  counts[nodesize - 1] += count % nodesize;
  displs[0] = 0;
  for (i = 1; i < nodesize; i++)
    displs[i] = displs[i - 1] + counts[i - 1];

  tempbuf = mpitalloc(count, datatype, &lb, &extent);

  takebuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
  if (count % nodesize == 0) {
    PMPI_Reduce_scatter_block(takebuf, (char*) tempbuf + noderank * block * extent, block, datatype, op, nodecomm);
  } else {
    PMPI_Reduce_scatter(takebuf, (char*) tempbuf + noderank * block * extent, counts, datatype, op, nodecomm);
  }

  PMPI_Exscan(MPI_IN_PLACE, (char*) tempbuf + noderank * block * extent, counts[noderank], datatype, op, lanecomm);

  PMPI_Scan(sendbuf, recvbuf, count, datatype, op, nodecomm); // could be overlapped
  if (lanerank > 0) {
    if (count % nodesize == 0) {
      PMPI_Allgather(MPI_IN_PLACE, block, datatype, tempbuf, block, datatype, nodecomm);
    } else {
      PMPI_Allgatherv(MPI_IN_PLACE, counts[noderank], datatype, tempbuf, counts, displs, datatype, nodecomm);
    }

    PMPI_Reduce_local(tempbuf, recvbuf, count, datatype, op);
  }

  free(tempbuf);

  return MPI_SUCCESS;
}

int Scatter_hier(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int lanerank, lanesize, noderank, nodesize;
  int rootnode, noderoot;

  MPI_Aint lb, extent;

  void *tempbuf = NULL;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);
  MPI_Comm_size(lanecomm, &lanesize);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  if (noderank == noderoot) {
    if (lanerank == rootnode) {
      // rank==root
      PMPI_Scatter(sendbuf, nodesize * sendcount, sendtype,
      MPI_IN_PLACE, nodesize * recvcount, recvtype, rootnode, lanecomm);
    } else {
      if (noderank == noderoot) {
        tempbuf = mpitalloc(nodesize * recvcount, sendtype, &lb, &extent);
      }

      PMPI_Scatter(sendbuf, nodesize * sendcount, sendtype, tempbuf, nodesize * recvcount, recvtype, rootnode, lanecomm);
    }
  }

  if (lanerank == rootnode) {
    if (noderank == noderoot) {
      MPI_Type_get_extent(sendtype, &lb, &extent);
    } else
      extent = 0;

    PMPI_Scatter((char*) sendbuf + lanerank * nodesize * sendcount * extent, sendcount, sendtype, recvbuf, recvcount,
        recvtype, noderoot, nodecomm);
  } else {
    PMPI_Scatter(tempbuf, recvcount, recvtype, recvbuf, recvcount, recvtype, noderoot, nodecomm);

    if (noderank == noderoot)
      free(tempbuf);
  }

  return MPI_SUCCESS;
}

int Scatter_lane(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  static MPI_Comm nodecomm = MPI_COMM_NULL;
  static MPI_Comm lanecomm = MPI_COMM_NULL;

  int lanerank, lanesize, noderank, nodesize;
  int rootnode, noderoot;

  MPI_Aint lb, extent;

  if (nodecomm == MPI_COMM_NULL)
    Get_Lane_comms(comm, &nodecomm, &lanecomm);

  MPI_Comm_rank(nodecomm, &noderank);
  MPI_Comm_size(nodecomm, &nodesize);
  MPI_Comm_rank(lanecomm, &lanerank);
  MPI_Comm_size(lanecomm, &lanesize);

  rootnode = root / nodesize;
  noderoot = root % nodesize;

  if (lanerank == rootnode) {
    if (noderank == noderoot) {
      // rank==root

      MPI_Datatype nt, nodetype, lt, lanetype;

      MPI_Type_get_extent(sendtype, &lb, &extent);

      MPI_Type_contiguous(sendcount, sendtype, &lt);
      MPI_Type_create_resized(lt, 0, nodesize * sendcount * extent, &lanetype);
      MPI_Type_commit(&lanetype);

      MPI_Type_vector(lanesize, sendcount, nodesize * sendcount, sendtype, &nt);
      MPI_Type_create_resized(nt, 0, sendcount * extent, &nodetype);
      MPI_Type_commit(&nodetype);

//      //#define NONBLOCKING
//#ifdef NONBLOCKING
//      MPI_Request request[nodesize];
//      int i, j;
//      j=0;
//      for (i=0; i<nodesize; i++) {
//        if (i==noderank) continue;
//        MPI_Isend((char*)sendbuf+i*sendcount*extent,1,nodetype,i,888,
//            nodecomm,&request[j++]);
//      }
//#else
      PMPI_Scatter(sendbuf, 1, nodetype,
      MPI_IN_PLACE, recvcount, recvtype, noderoot, nodecomm);
//#endif
      PMPI_Scatter((char*) sendbuf + noderank * sendcount * extent, 1, lanetype, recvbuf, recvcount, recvtype, rootnode,
          lanecomm);
//#ifdef NONBLOCKING
//      MPI_Waitall(j,request,MPI_STATUSES_IGNORE);
//#endif

      MPI_Type_free(&nt);
      MPI_Type_free(&nodetype);
      MPI_Type_free(&lt);
      MPI_Type_free(&lanetype);
    } else {
      void *tempbuf;

      tempbuf = mpitalloc(lanesize * recvcount, sendtype, &lb, &extent);

      // only recvbuf is significant
//#ifdef NONBLOCKING
//      MPI_Recv(tempbuf,lanesize*recvcount,recvtype,noderoot,888,nodecomm,
//          MPI_STATUS_IGNORE);
//#else
      PMPI_Scatter(sendbuf, lanesize * sendcount, sendtype, tempbuf, lanesize * recvcount, recvtype, noderoot, nodecomm);
//#endif
      PMPI_Scatter(tempbuf, recvcount, recvtype, recvbuf, recvcount, recvtype, rootnode, lanecomm);

      free(tempbuf);
    }
  } else {
    // only recvbuf is significant
    PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, rootnode, lanecomm);
  }

  return MPI_SUCCESS;
}

//#ifdef SCANLANE0
//int Scan_hier(void *sendbuf,
//    void *recvbuf, int count, MPI_Datatype datatype,
//    MPI_Op op, MPI_Comm comm)
//{
//  static MPI_Comm nodecomm = MPI_COMM_NULL;
//  static MPI_Comm lanecomm = MPI_COMM_NULL;
//
//  int noderank, nodesize, lanerank;
//
//  MPI_Aint lb, extent;
//
//  void *takebuf, *tempbuf;
//
//  if (nodecomm==MPI_COMM_NULL) Get_Lane_comms(comm,&nodecomm,&lanecomm);
//
//  MPI_Comm_rank(nodecomm,&noderank);
//  MPI_Comm_size(nodecomm,&nodesize);
//  MPI_Comm_rank(lanecomm,&lanerank);
//
//  if (noderank==0) {
//    tempbuf = mpitalloc(count,datatype,&lb,&extent);
//  }
//
//  takebuf = (sendbuf==MPI_IN_PLACE) ? recvbuf : sendbuf;
//  MPI_Reduce(takebuf,tempbuf,count,datatype,op,0,nodecomm);
//  if (noderank==0) {
//    MPI_Exscan(MPI_IN_PLACE,tempbuf,count,datatype,op,lanecomm);
//  }
//  if (lanerank==0) {
//    MPI_Scan(sendbuf,recvbuf,count,datatype,op,nodecomm);
//  } else {
//    if (noderank==0) {
//      MPI_Reduce_local(takebuf,tempbuf,count,datatype,op);
//      MPI_Scan(tempbuf,recvbuf,count,datatype,op,nodecomm);
//    } else {
//      MPI_Scan(sendbuf,recvbuf,count,datatype,op,nodecomm);
//    }
//  }
//
//  if (noderank==0) free(tempbuf);
//
//  return MPI_SUCCESS;
//}
//#else // SCANLANEn-1
//#endif

