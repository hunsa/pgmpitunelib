/*  PGMPITuneLib - Library for Autotuning MPI Collectives using Performance Guidelines
 *
 *  Copyright 2017 Sascha Hunold, Alexandra Carpen-Amarie
 *      Research Group for Parallel Computing
 *      Faculty of Informatics
 *      Vienna University of Technology, Austria
 *
 *  <license>
 *      This library is free software; you can redistribute it
 *      and/or modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free
 *      Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *      Boston, MA 02110-1301 USA
 *  </license>
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgmpi_tune.h"
#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "bufmanager/pgmpi_buf.h"
#include "util/pgmpi_parse_cli.h"
#include "reduce_impl.h"

static const int MIN_SCATTER_CHUNK_SIZE = 4; // number of elements


int MPI_Reduce_as_Allreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
    MPI_Comm comm) {
  int ret = BUF_NO_ERROR;
  int n;
  int rank;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size;
  void *aux_buf1;

  ZF_LOGV("Calling MPI_Reduce_as_Allreduce");

  MPI_Comm_rank(comm, &rank);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  // we need a fake buffer with n elements in aux_buf1
  n = count;          // buffer size per process
  fake_buf_size = count * type_extent;

  ZF_LOGV("fake_send_size: %zu", fake_buf_size);
  ret = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (ret != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  if (rank == root) {      // the real receive buffer is allocated on the root
    PGMPI(MPI_Allreduce(sendbuf, recvbuf, n, datatype, op, comm));

  } else { // use a temporary buffer allocated on all other processes as the receive buffer
    PGMPI(MPI_Allreduce(sendbuf, aux_buf1, n, datatype, op, comm));
  }


  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Reduce_as_Reduce_scatter_block_Gather(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype,
    MPI_Op op, int root, MPI_Comm comm) {
  int n, i;
  int n_padding_elems;
  int recvcount1, sendcount2, recvcount2;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size1, fake_buf_size2;
  void *aux_buf1, *aux_buf2;
  int buf_status = BUF_NO_ERROR;
  MPI_Aint padding_offset;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Reduce_as_Reduce_scatter_block_Gather");

  // we need two fake buffers: aux_buf1 with (n + padding) elements
  //    and aux_buf2 with (n + padding)/size elements
  n = count;            // buffer size per process
  if (n % size == 0) {
    n_padding_elems = 0;
  } else {
    n_padding_elems = size - (n % size);
  }

  fake_buf_size1 = (n + n_padding_elems) * type_extent;       // max size for the padded buffer
  ZF_LOGV("fake_buf_size1: %zu", fake_buf_size1);
  buf_status = grab_msg_buffer_1(fake_buf_size1, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }
  fake_buf_size2 = fake_buf_size1 / size;       // max size for the scattered buffer per process
  ZF_LOGV("fake_buf_size2: %zu", fake_buf_size2);
  buf_status = grab_msg_buffer_2(fake_buf_size2, &aux_buf2);
  ZF_LOGV("fake buffer 2 points to %p", aux_buf2);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  recvcount1 = fake_buf_size2 / type_extent;

  // copy the send buffer to the beginning of the padded temporary buffer
  memcpy(aux_buf1, sendbuf, n * type_extent);

  // fill in the padded elements to make sure we apply Reduce on user data
  padding_offset = n * type_extent;
  for (i=0; i<n_padding_elems; i++) {
    memcpy((char*)aux_buf1 + padding_offset, sendbuf, type_extent);  // only use the first element from sendbuf

    padding_offset += type_extent;
  }

  PGMPI(MPI_Reduce_scatter_block(aux_buf1, aux_buf2, recvcount1, datatype, op, comm));

  sendcount2 = recvcount1;
  recvcount2 = recvcount1;

  ZF_LOGV("recvcount2: %d", recvcount2);
  PGMPI(MPI_Gather(aux_buf2, sendcount2, datatype, aux_buf1, recvcount2, datatype, root, comm));

  if (rank == root) {           // copy only the needed data to the final receive buffer
    memcpy(recvbuf, aux_buf1, n * type_extent);
  }

  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Reduce_as_Reduce_scatter_Gatherv(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype,
    MPI_Op op, int root, MPI_Comm comm) {
  int n;
  int nchunks;
  int i;
  int *recvcounts;
  int *displs;
  int sendcount;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size, fake_int_buf_size;
  int *aux_int_buf1, *aux_int_buf2;
  void *aux_buf1;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Reduce_as_Reduce_scatter_Gatherv");

  // we need one fake data buffer: aux_buf1 with at most ((n * type_extent)/(chunk_size * size) + 1) elements
  //    and two fake int buffers with size elements: aux_int_buf1, aux_int_buf2
  n = count;            // buffer size per process

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

  // int buffers are allocated - now try to obtain data buffer

  nchunks = n / MIN_SCATTER_CHUNK_SIZE;   // handle the remainder separately
  recvcounts = aux_int_buf1;

  // assign chunks to processes (round robin) - at least MIN_SCATTER_CHUNK_SIZE elements per process
  for (i = 0; i < size; i++) {
    recvcounts[i] = MIN_SCATTER_CHUNK_SIZE * (nchunks/size);

    if (i < nchunks % size) {
      recvcounts[i] += MIN_SCATTER_CHUNK_SIZE;
    } else if (i == nchunks % size ) { // last chunk can be smaller than MIN_SCATTER_CHUNK_SIZE
      recvcounts[i] += n % MIN_SCATTER_CHUNK_SIZE;
    }
  }

  ZF_LOGV("nchunks=%d count=%d i=%d", nchunks, n, i);
  if (rank == root) {
    for (i = 0; i < size; i++) {
      ZF_LOGV("recvcounts[%d]=%d", i, recvcounts[i]);
    }
  }

  fake_buf_size = recvcounts[0] * type_extent;  // max recv buffer size per process
                                                // (to make sure all processes obtain the same buf_status)
  ZF_LOGV("fake_buf_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  // aux_buf1 needs recvcounts[rank]  elements on each process
  PGMPI(MPI_Reduce_scatter(sendbuf, aux_buf1, recvcounts, datatype, op, comm));

  displs = aux_int_buf2;
  displs[0] = 0;
  for (i = 1; i < size; i++) {
    displs[i] = displs[i - 1] + recvcounts[i - 1];
  }
  sendcount = recvcounts[rank];

  PGMPI(MPI_Gatherv(aux_buf1, sendcount, datatype, recvbuf, recvcounts, displs, datatype, root, comm));

  release_msg_buffers();
  release_int_buffers();
  return MPI_SUCCESS;
}
