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
#include "scan_impl.h"

int MPI_Scan_as_Exscan_Reduce_local(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
    MPI_Comm comm) {
  MPI_Aint lb, type_extent;
  int rank;

  MPI_Comm_rank(comm, &rank);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Scan_as_Exscan_Reduce_local");

  PGMPI(MPI_Exscan(sendbuf, recvbuf, count, datatype, op, comm));

  if (rank == 0) {        // recvbuf is not modified by Exscan on process 0 (should be identical to sendbuf)
    memcpy(recvbuf, sendbuf, count * type_extent);
  } else {
    PGMPI(MPI_Reduce_local(sendbuf, recvbuf, count, datatype, op));
  }
  return MPI_SUCCESS;
}
