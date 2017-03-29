
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

static const int mockup_root_rank = 0;

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  ALLGATHER_DEFAULT = 0,
  ALLGATHER_AS_ALLGATHERV = 1,
  ALLGATHER_AS_ALLREDUCE = 2,
  ALLGATHER_AS_ALLTOALL  = 3,
  ALLGATHER_AS_GATHERBCAST = 4
};

static alg_choice_t module_algs[5] = {
    { ALLGATHER_DEFAULT, "default" },
    { ALLGATHER_AS_ALLGATHERV, "allgather_as_allgatherv" },
    { ALLGATHER_AS_ALLREDUCE, "allgather_as_allreduce" },
    { ALLGATHER_AS_ALLTOALL, "allgather_as_alltoall" },
    { ALLGATHER_AS_GATHERBCAST, "allgather_as_gather_bcast" }
};

static module_alg_choices_t module_choices = { 5, module_algs };

static int alg_id = ALLGATHER_DEFAULT;

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

void register_module_allgather(module_t *module) {
  module->cli_prefix  = "allgather";
  module->mpiname     = "MPI_Allgather";
  module->cid         = CID_MPI_ALLGATHER;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 0;
}

void deregister_module_allgather(module_t *module) {
  // nothing to do
}


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
  memcpy(aux_buf1 + (rank * sendbuf_size), sendbuf, sendbuf_size);

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
    memcpy((char*)(aux_buf1 + (i*sendbuf_size)), sendbuf, sendbuf_size);
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


int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
    void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {

  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Allgather");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_ALLGATHER, sendcount, size, &alg_id);
  }

  switch (alg_id) {
  case ALLGATHER_AS_ALLGATHERV:
    ret_status = MPI_Allgather_as_Allgatherv(sendbuf, sendcount, sendtype, recvbuf,
            recvcount, recvtype, comm);
    break;
  case ALLGATHER_AS_ALLREDUCE:
    ret_status = MPI_Allgather_as_Allreduce(sendbuf, sendcount, sendtype, recvbuf,
            recvcount, recvtype, comm);
    break;
  case ALLGATHER_AS_ALLTOALL:
    ret_status = MPI_Allgather_as_Alltoall(sendbuf, sendcount, sendtype, recvbuf,
        recvcount, recvtype, comm);
    break;
  case ALLGATHER_AS_GATHERBCAST:
    ret_status = MPI_Allgather_as_GatherBcast(sendbuf, sendcount, sendtype, recvbuf,
        recvcount, recvtype, comm);
    break;
  case ALLGATHER_DEFAULT:
    call_default = 1;
    break;
  default:   // call the original function
    ZF_LOGW("cannot find alg id %d, using default", alg_id);
    call_default = 1;
  }

  if (ret_status != MPI_SUCCESS) {
    call_default = 1;
  }

  if( call_default == 1 ) {
    ZF_LOGV("Calling PMPI_Allgather");
    PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_ALLGATHER, sendcount, alg_id, call_default);
  }

  return MPI_SUCCESS;
}

