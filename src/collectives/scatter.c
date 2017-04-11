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
  SCATTER_DEFAULT = 0,
  SCATTER_AS_BCAST = 1,
  SCATTER_AS_SCATTERV = 2
};

static alg_choice_t module_algs[3] = {
    { SCATTER_DEFAULT, "default" },
    { SCATTER_AS_BCAST, "scatter_as_bcast" },
    { SCATTER_AS_SCATTERV, "scatter_as_scatterv" }
};

static module_alg_choices_t module_choices = { 3, module_algs };

static int alg_id = SCATTER_DEFAULT;

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

void register_module_scatter(module_t *module) {
  module->cli_prefix  = "scatter";
  module->mpiname     = "MPI_Scatter";
  module->cid         = CID_MPI_SCATTER;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;
}

void deregister_module_scatter(module_t *module) {
}

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
  memcpy(recvbuf, bcast_buf + rank * n * type_extent, n * type_extent);

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

int MPI_Scatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Scatter");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_SCATTER, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), size, &alg_id);
  }

  switch (alg_id) {
  case SCATTER_AS_BCAST:
    ret_status = MPI_Scatter_as_Bcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case SCATTER_AS_SCATTERV:
    ret_status = MPI_Scatter_as_Scatterv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case SCATTER_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Scatter");
    PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_SCATTER, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

