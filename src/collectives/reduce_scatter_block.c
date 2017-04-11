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
  REDUCESCATTERBLOCK_DEFAULT = 0,
  REDUCESCATTERBLOCK_AS_REDUCE_SCATTER = 1,
  REDUCESCATTERBLOCK_AS_REDUCESCATTER = 2,
  REDUCESCATTERBLOCK_AS_ALLREDUCE = 3
};

static alg_choice_t module_algs[4] = {
    { REDUCESCATTERBLOCK_DEFAULT, "default" },
    { REDUCESCATTERBLOCK_AS_REDUCE_SCATTER, "reducescatterblock_as_reduce_scatter" },
    { REDUCESCATTERBLOCK_AS_REDUCESCATTER, "reducescatterblock_as_reducescatter" },
    { REDUCESCATTERBLOCK_AS_ALLREDUCE, "reducescatterblock_as_allreduce" },
};

static module_alg_choices_t module_choices = { 4, module_algs };

static int alg_id = REDUCESCATTERBLOCK_DEFAULT;

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

void register_module_reduce_scatter_block(module_t *module) {
  module->cli_prefix  = "reduce_scatter_block";
  module->mpiname     = "MPI_Reduce_scatter_block";
  module->cid         = CID_MPI_REDUCESCATTERBLOCK;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;

}

void deregister_module_reduce_scatter_block(module_t *module) {
}

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
  memcpy(recvbuf, (char*)(aux_buf1 + rank * recvbuf_size), recvbuf_size);

  release_msg_buffers();
  return MPI_SUCCESS;
}


int MPI_Reduce_scatter_block(const void* sendbuf, void* recvbuf, const int recvcount, MPI_Datatype datatype, MPI_Op op,
    MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Reduce_scatter_block");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void) pgtune_get_algorithm(CID_MPI_REDUCESCATTERBLOCK, pgmpi_convert_type_count_2_bytes(recvcount, datatype), size,
        &alg_id);
  }

  switch (alg_id) {
  case REDUCESCATTERBLOCK_AS_REDUCE_SCATTER:
    ret_status = MPI_Reduce_scatter_block_as_Reduce_Scatter(sendbuf, recvbuf, recvcount, datatype, op, comm);
    break;
  case REDUCESCATTERBLOCK_AS_REDUCESCATTER:
    ret_status = MPI_Reduce_scatter_block_as_Reduce_scatter(sendbuf, recvbuf, recvcount, datatype, op, comm);
    break;
  case REDUCESCATTERBLOCK_AS_ALLREDUCE:
    ret_status = MPI_Reduce_scatter_block_as_Allreduce(sendbuf, recvbuf, recvcount, datatype, op, comm);
    break;
  case REDUCESCATTERBLOCK_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Reduce_scatter_block");
    PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_REDUCESCATTERBLOCK, pgmpi_convert_type_count_2_bytes(recvcount, datatype),
        alg_id, call_default);
  }

  return MPI_SUCCESS;
}

