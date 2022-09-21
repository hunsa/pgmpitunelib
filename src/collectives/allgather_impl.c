
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "bufmanager/pgmpi_buf.h"
#include "allgather_impl.h"

static const int mockup_root_rank = 0;

int MPI_Allgather_as_Allgatherv(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm) {
  int n;
  int i;
  int *recvcounts;
  int *displs;
  int size;
  int *aux_int_buf1, *aux_int_buf2;
  size_t fake_int_buf_size;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_size(comm, &size);

  ZF_LOGV("Calling MPI_Allgather_as_Allgatherv");

  // we need two fake int buffers with size elements: aux_int_buf1, aux_int_buf2
  n = sendcount;            // send count per process

  fake_int_buf_size = size * sizeof(int);     // same size for both int buffers
  ZF_LOGV("fake_int_buf_size: %zu", fake_int_buf_size);
  buf_status = grab_int_buffer_1(fake_int_buf_size, &aux_int_buf1);
  ZF_LOGV("fake int buffer 1 points to %p", aux_int_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }
  buf_status = grab_int_buffer_2(fake_int_buf_size, &aux_int_buf2);
  ZF_LOGV("fake int buffer 2 points to %p", aux_int_buf2);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  recvcounts = aux_int_buf1;
  displs = aux_int_buf2;

  for (i = 0; i < size; i++) {
    recvcounts[i] = n;
    displs[i] = i * n;
  }
  PGMPI(MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm));

  release_int_buffers();
  return MPI_SUCCESS;
}

int MPI_Allgather_as_Allreduce(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm) {
  int size, rank;
  MPI_Aint lb, type_extent;
  size_t fake_buf_size, sendbuf_size;
  int n;
  void *aux_buf1;
  int buf_status = BUF_NO_ERROR;
  MPI_Op op = MPI_BOR;

  ZF_LOGV("Calling MPI_Allgather_as_Allreduce");

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(sendtype, &lb, &type_extent);

  // we need one fake data buffer: aux_buf1 with n elements
  n = sendcount;  // send count per process
  sendbuf_size = n * type_extent;

  fake_buf_size = size * sendbuf_size;   // allreduce sendbuf for all processes
  ZF_LOGV("fake_buf_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  memset(aux_buf1, 0, fake_buf_size);
  // copy sendbuf to the block corresponding to the current rank
  memcpy((char*)aux_buf1 + (rank * sendbuf_size), sendbuf, sendbuf_size);

  PGMPI(MPI_Allreduce(aux_buf1, recvbuf, fake_buf_size, MPI_CHAR, op, comm));

  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Allgather_as_Alltoall(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm) {
  int i, size;
  MPI_Aint lb, type_extent;
  size_t fake_buf_size, sendbuf_size;
  void *aux_buf1;
  int n;
  int buf_status = BUF_NO_ERROR;

  ZF_LOGV("Calling MPI_Allgather_as_Alltoall");

  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(sendtype, &lb, &type_extent);

  // we need one fake data buffer: aux_buf1 with n elements
  n = sendcount;  // send count per process
  sendbuf_size = n * type_extent;

  fake_buf_size = size * sendbuf_size;   // sendbuf for all to all
  ZF_LOGV("fake_buf_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  for(i=0; i<size; i++) {
    ZF_LOGV("copy to aux_buf into %zu", i*sendbuf_size);
    memcpy((char*)aux_buf1 + (i*sendbuf_size), sendbuf, sendbuf_size);
  }

  PGMPI(MPI_Alltoall(aux_buf1, sendcount, sendtype, recvbuf, recvcount, recvtype, comm));

  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Allgather_as_GatherBcast(const void *sendbuf, int sendcount,
    MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
    MPI_Comm comm) {
  int size;

  MPI_Comm_size(comm, &size);
  int bcast_count = recvcount * size;

  ZF_LOGV("Calling MPI_Allgather_as_GatherBcast");

  PGMPI(MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, mockup_root_rank, comm));
  PGMPI(MPI_Bcast(recvbuf, bcast_count, recvtype, mockup_root_rank, comm));

  return MPI_SUCCESS;
}
