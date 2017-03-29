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
  ALLTOALL_DEFAULT = 0,
  ALLTOALL_AS_ALLTOALLV = 1
};

static alg_choice_t module_algs[2] = {
    { ALLTOALL_DEFAULT, "default" },
    { ALLTOALL_AS_ALLTOALLV, "alltoall_as_alltoallv" }
};

static module_alg_choices_t module_choices = { 2, module_algs };

static int alg_id = ALLTOALL_DEFAULT;

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

void register_module_alltoall(module_t *module) {
  module->cli_prefix = "alltoall";
  module->mpiname    = "MPI_Alltoall";
  module->cid         = CID_MPI_ALLTOALL;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 0;
}

void deregister_module_alltoall(module_t *module) {
}

int MPI_Alltoall_as_Alltoallv(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm) {
  int i, size;
  size_t fake_int_buf_size;
  int *sendcounts, *recvcounts;
  int *sdispls, *rdispls;
  int n;
  int *aux_int_buf1, *aux_int_buf2;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_size(comm, &size);

  ZF_LOGV("Calling MPI_Alltoall_as_Alltoallv");

  // we need two fake int buffers with size elements: aux_int_buf1, aux_int_buf2
  n = sendcount;            // buffer size per process

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
  sdispls = aux_int_buf2;
  for (i = 0; i < size; i++) {
    sendcounts[i] = n;
    sdispls[i] = i * n;
  }
  recvcounts = sendcounts;
  rdispls = sdispls;

  PGMPI(MPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm));

  release_int_buffers();
  return MPI_SUCCESS;
}

int MPI_Alltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Alltoall");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_ALLTOALL, sendcount, size, &alg_id);
  }

  switch (alg_id) {
  case ALLTOALL_AS_ALLTOALLV:
    ret_status = MPI_Alltoall_as_Alltoallv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    break;
  case ALLTOALL_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Alltoall");
    PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_ALLTOALL, sendcount, alg_id, call_default);
  }

  return MPI_SUCCESS;
}

