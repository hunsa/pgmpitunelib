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
  REDUCESCATTERBLOCK_AS_REDUCE_SCATTER = 1
};

static alg_choice_t module_algs[2] = {
    { REDUCESCATTERBLOCK_DEFAULT, "default" },
    { REDUCESCATTERBLOCK_AS_REDUCE_SCATTER, "reducescatterblock_as_reduce_scatter" },
};

static module_alg_choices_t module_choices = { 2, module_algs };

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

int MPI_Reduce_scatter_block(const void* sendbuf, void* recvbuf, const int recvcount, MPI_Datatype datatype, MPI_Op op,
    MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Reduce_scatter_block");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_REDUCESCATTERBLOCK, recvcount, size, &alg_id);
  }

  switch (alg_id) {
  case REDUCESCATTERBLOCK_AS_REDUCE_SCATTER:
    ret_status = MPI_Reduce_scatter_block_as_Reduce_Scatter(sendbuf, recvbuf, recvcount, datatype, op, comm);
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
    pgmpi_save_algid_for_msg_size(CID_MPI_REDUCESCATTERBLOCK, recvcount, alg_id, call_default);
  }

  return MPI_SUCCESS;
}

