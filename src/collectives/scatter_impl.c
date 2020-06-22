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
#include "scatter_impl.h"


int MPI_Scatter_as_Bcast(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int size, rank;
  int count;
  int n;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size;
  void *aux_buf1, *bcast_buf;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(sendtype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Scatter_as_Bcast");

  // we need a fake buffer with size * n elements in aux_buf1
  n = sendcount;  // block size scattered per process
  count = size * n;
  fake_buf_size = count * type_extent;

  ZF_LOGV("fake_send_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  if (rank == root) { // sendbuf is only defined at the root
    bcast_buf = (void*)sendbuf;
  } else {      // all other processes will use the fake buffer to receive the broadcasted data
    bcast_buf = aux_buf1;
  }

  PGMPI(MPI_Bcast(bcast_buf, count, sendtype, root, comm));

  // copy results to the receive buffer on each process
  memcpy(recvbuf, (char*)bcast_buf + rank * n * type_extent, n * type_extent);

  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Scatter_as_Scatterv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int n;
  int i;
  int *sendcounts;
  int *displs;
  int size;
  size_t fake_int_buf_size;
  int *aux_int_buf1, *aux_int_buf2;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_size(comm, &size);

  ZF_LOGV("Calling MPI_Scatter_as_Scatterv");

  // we need two fake int buffers with size elements: aux_int_buf1, aux_int_buf2
  n = sendcount;            // buffer size scatters to each process

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

  sendcounts = aux_int_buf1;
  displs = aux_int_buf2;
  for (i = 0; i < size; i++) {
    sendcounts[i] = n;
    displs[i] = i * n;
  }

  PGMPI(MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm));

  release_int_buffers();
  return MPI_SUCCESS;
}
