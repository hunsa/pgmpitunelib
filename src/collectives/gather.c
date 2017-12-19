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

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_tune.h"
#include "bufmanager/pgmpi_buf.h"
#include "pgmpi_algid_store.h"
#include "collectives/collective_modules.h"
#include "util/pgmpi_parse_cli.h"

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  GATHER_DEFAULT = 0,
  GATHER_AS_ALLGATHER = 1,
  GATHER_AS_GATHERV = 2,
  GATHER_AS_REDUCE = 3
};

static alg_choice_t module_algs[4] = {
    { GATHER_DEFAULT, "default" },
    { GATHER_AS_ALLGATHER, "gather_as_allgather" },
    { GATHER_AS_GATHERV, "gather_as_gatherv" },
    { GATHER_AS_REDUCE, "gather_as_reduce" }
};

static module_alg_choices_t module_choices = { 4, module_algs };

static int alg_id = GATHER_DEFAULT;

static void parse_arguments(const char *argv) {
  alg_id = pgmpi_parse_module_params_get_cid(&module_choices, argv);
  pgmpi_check_algid_valid(&module_choices, alg_id);
}

static void set_algid(const int algid) {
  if( algid >= 0 && algid < module_choices.nb_choices ) {
    alg_id = algid;
  } else {
    ZF_LOGW("invalid algid, do not modify algorithm");
  }
}

void register_module_gather(module_t *module) {
  module->cli_prefix  = "gather";
  module->mpiname     = "MPI_Gather";
  module->cid         = CID_MPI_GATHER;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;
}

void deregister_module_gather(module_t *module) {
}


int MPI_Gather_as_Allgather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                            void* recvbuf, int recvcount, MPI_Datatype recvtype,
                            int root, MPI_Comm comm) {
  int buf_status = BUF_NO_ERROR;
  int n;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size;
  void *aux_buf1;

  ZF_LOGV("Calling MPI_Gather_as_Allgather");

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(sendtype, &lb, &type_extent);

  // we need a fake buffer with n elements in aux_buf1
  n = sendcount;    // buffer size per process
  fake_buf_size = size * n * type_extent;

  ZF_LOGV("fake_send_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  if (rank == root) {      // the real receive buffer is allocated on the root
    PGMPI(MPI_Allgather(sendbuf, sendcount, sendtype,
                        recvbuf, recvcount, recvtype,
                        comm));
  } else { // use a temporary buffer allocated on all other processes as the receive buffer
    PGMPI(MPI_Allgather(sendbuf, sendcount, sendtype,
                        aux_buf1, recvcount, recvtype,
                        comm));
  }

  release_msg_buffers();
  return MPI_SUCCESS;
}



int MPI_Gather_as_Gatherv(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                          void* recvbuf, int recvcount, MPI_Datatype recvtype,
                          int root, MPI_Comm comm) {
  int i;
  int n;
  int *recvcounts;
  int *displs;
  int buf_status = BUF_NO_ERROR;
  size_t fake_int_buf_size;
  int *aux_int_buf1, *aux_int_buf2;
  int size;

  ZF_LOGV("Calling MPI_Gather_as_Gatherv");

  MPI_Comm_size(comm, &size);

  // we need two fake int buffers with size elements: aux_int_buf1, aux_int_buf2
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


  n = sendcount;          // buffer size per process
  recvcounts = aux_int_buf1;
  displs = aux_int_buf2;

  for (i=0; i < size; i++) {
    recvcounts[i] = n;
    displs[i] = i * n;
  }

  PGMPI(MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm));

  release_int_buffers();
  return MPI_SUCCESS;
}



int MPI_Gather_as_Reduce(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                         void* recvbuf, int recvcount, MPI_Datatype recvtype,
                         int root, MPI_Comm comm) {
  int buf_status = BUF_NO_ERROR;
  int n;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size;
  void *aux_buf1;
  MPI_Op op;
  int count;

  ZF_LOGV("Calling MPI_Gather_as_Reduce");

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(sendtype, &lb, &type_extent);

  // we need a fake buffer with size * n elements in aux_buf1
  n = sendcount;        // buffer size per process
  op = MPI_BOR;
  count = size * n;
  fake_buf_size = count * type_extent;

  ZF_LOGV("fake_send_size: %zu", fake_buf_size);
  buf_status = grab_msg_buffer_1(fake_buf_size, &aux_buf1);
  ZF_LOGV("fake buffer 1 points to %p", aux_buf1);
  if (buf_status != BUF_NO_ERROR) {
    return MPI_ERR_NO_MEM;
  }

  memset(aux_buf1, 0, fake_buf_size);
  // copy sendbuf to the block corresponding to the current rank
  memcpy((char*)aux_buf1 + (rank * n * type_extent), sendbuf, n * type_extent);

  PGMPI(MPI_Reduce(aux_buf1, recvbuf, fake_buf_size, MPI_CHAR, op, root, comm));

  release_msg_buffers();
  return MPI_SUCCESS;
}


int MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
    void* recvbuf, int recvcount, MPI_Datatype recvtype,
    int root, MPI_Comm comm) {

  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Gather");

  MPI_Comm_size(comm, &size);
  if (get_pgmpi_context() == CONTEXT_TUNED) {
    (void)pgtune_get_algorithm(CID_MPI_GATHER, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), size, &alg_id);
  }

  switch (alg_id) {
  case GATHER_AS_ALLGATHER:
    ret_status = MPI_Gather_as_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case GATHER_AS_GATHERV:
    ret_status = MPI_Gather_as_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case GATHER_AS_REDUCE:
    ret_status = MPI_Gather_as_Reduce(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case GATHER_DEFAULT:
    call_default = 1;
    break;
  default:   // call the original function
    ZF_LOGW("cannot find alg id %d, using default", alg_id);
    call_default = 1;
  }

  if (ret_status != MPI_SUCCESS) {
    call_default = 1;
  }

  if (call_default == 1) {
    ZF_LOGV("Calling PMPI_Gather");
    PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_GATHER, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

