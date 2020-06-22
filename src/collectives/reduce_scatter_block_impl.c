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
#include "reduce_scatter_block_impl.h"

static const int mockup_root_rank = 0;


int MPI_Reduce_scatter_block_as_Reduce_Scatter(const void* sendbuf, void* recvbuf, const int recvcount,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int size;
  int count, sendcount;
  int n;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size;
  void *aux_buf1;

  int buf_status = BUF_NO_ERROR;

  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Reduce_scatter_block_as_Reduce_Scatter");

  n = recvcount;  // block size scattered per process
  count = size * n;
  fake_buf_size = count * type_extent;

  ZF_LOGV("fake_send_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  // reduce entire buffer to root
  PGMPI(MPI_Reduce(sendbuf, aux_buf1, count, datatype, op, mockup_root_rank, comm));

  // scatter from root the corresponding block per process
  sendcount = n;
  PGMPI(MPI_Scatter(aux_buf1, sendcount, datatype, recvbuf, recvcount, datatype, mockup_root_rank, comm));
  release_msg_buffers();
  return MPI_SUCCESS;
}


int MPI_Reduce_scatter_block_as_Reduce_scatter(const void* sendbuf, void* recvbuf, const int recvcount,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int size, i;
  int n;
  MPI_Aint type_extent, lb;
  size_t fake_int_buf_size;
  int *aux_int_buf, *recvcounts;

  int buf_status = BUF_NO_ERROR;

  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Reduce_scatter_block_as_Reduce_scatter");

  // we need int buffer for recv counts
  n = recvcount;  // block size scattered per process

  fake_int_buf_size = size * sizeof(int);
  ZF_LOGV("fake_int_buf_size: %zu", fake_int_buf_size);
  buf_status = grab_int_buffer_1(fake_int_buf_size, &aux_int_buf);
  ZF_LOGV("fake int buffer 1 points to %p", aux_int_buf);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  recvcounts = aux_int_buf;
  for (i = 0; i < size; i++) {
    recvcounts[i] = n;
  }

  PGMPI(MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm));

  release_int_buffers();
  return MPI_SUCCESS;
}


int MPI_Reduce_scatter_block_as_Allreduce(const void* sendbuf, void* recvbuf, const int recvcount,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int size, rank;
  int count;
  int n;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size, recvbuf_size;
  void *aux_buf1;

  int buf_status = BUF_NO_ERROR;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Reduce_scatter_block_as_Allreduce");

  // we need a fake buffer with size*n elements in aux_buf1
  n = recvcount;  // block size scattered per process
  recvbuf_size = n * type_extent;

  count = size * n;
  fake_buf_size = count * type_extent;

  ZF_LOGV("fake_send_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  // reduce entire buffer to root
  PGMPI(MPI_Allreduce(sendbuf, aux_buf1, count, datatype, op, comm));

  ZF_LOGV("copy from aux_buf1 into recvbuf: %zu", recvbuf_size);
  memcpy(recvbuf, (char*)(aux_buf1) + rank * recvbuf_size, recvbuf_size);

  release_msg_buffers();
  return MPI_SUCCESS;
}
