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

static const int MIN_SCATTER_CHUNK_SIZE = 4; // number of elements

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  REDUCE_DEFAULT = 0,
  REDUCE_AS_ALLREDUCE = 1,
  REDUCE_AS_REDUCESCATTERBLOCK_GATHER = 2,
  REDUCE_AS_REDUCESCATTER_GATHERV = 3
};

static alg_choice_t module_algs[4] = {
    { REDUCE_DEFAULT, "default" },
    { REDUCE_AS_ALLREDUCE, "reduce_as_allreduce" },
    { REDUCE_AS_REDUCESCATTERBLOCK_GATHER, "reduce_as_reducescatterblock_gather" },
    { REDUCE_AS_REDUCESCATTER_GATHERV, "reduce_as_reducescatter_gatherv" }
};

static module_alg_choices_t module_choices = { 4, module_algs };

static int alg_id = REDUCE_DEFAULT;

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

void register_module_reduce(module_t *module) {
  module->cli_prefix  = "reduce";
  module->mpiname     = "MPI_Reduce";
  module->cid         = CID_MPI_REDUCE;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;
}

void deregister_module_reduce(module_t *module) {
}

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
  int n;
  int n_padding_elems;
  void *sendbuf1;
  void *recvbuf1;
  void *sendbuf2;
  void *recvbuf2;
  int recvcount1, sendcount2, recvcount2;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size1, fake_buf_size2;
  void *aux_buf1, *aux_buf2;
  int buf_status = BUF_NO_ERROR;

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

  sendbuf1 = aux_buf1;   // padded send buffer
  recvbuf1 = aux_buf2;   // receive buffer after scatter
  recvcount1 = fake_buf_size2 / type_extent;

  // copy the send buffer to the beginning of the padded temporary buffer
  memcpy(sendbuf1, sendbuf, n * type_extent);

  PGMPI(MPI_Reduce_scatter_block(sendbuf1, recvbuf1, recvcount1, datatype, op, comm));

  sendbuf2 = recvbuf1;   // send buffer per process (reuse previous receive buffer)
  recvbuf2 = aux_buf1;   // padded receive buffer to hold the final result
  sendcount2 = recvcount1;
  recvcount2 = recvcount1;

  ZF_LOGV("recvcount2: %d", recvcount2);
  PGMPI(MPI_Gather(sendbuf2, sendcount2, datatype, recvbuf2, recvcount2, datatype, root, comm));

  if (rank == root) {           // copy only the needed data to the final receive buffer
    memcpy(recvbuf, recvbuf2, n * type_extent);
  }

  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Reduce_as_Reduce_scatter_Gatherv(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype,
    MPI_Op op, int root, MPI_Comm comm) {
  int n;
  int min_elems_per_proc, nchunks;
  int i, chunk_id;
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

  min_elems_per_proc = MIN_SCATTER_CHUNK_SIZE;
  nchunks = n / min_elems_per_proc;   // handle the remainder separately

  recvcounts = aux_int_buf1;
  for (i = 0; i < size; i++) {
    recvcounts[i] = 0;
  }

  // assign chunks to processes (round robin) - at least MIN_SCATTER_CHUNK_SIZE elements per process
  i = 0;
  for (chunk_id = 0; chunk_id < nchunks; chunk_id++) {
    recvcounts[i] += min_elems_per_proc;
    i = (i + 1) % size;
  }

  // last chunk can be smaller than chunk_size
  if (n % min_elems_per_proc != 0) {
    recvcounts[i] += n % min_elems_per_proc;
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

int MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {

  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Reduce");

  MPI_Comm_size(comm, &size);
  if (get_pgmpi_context() == CONTEXT_TUNED) {
    (void) pgtune_get_algorithm(CID_MPI_REDUCE, pgmpi_convert_type_count_2_bytes(count, datatype), size, &alg_id);
  }

  switch (alg_id) {
  case REDUCE_AS_ALLREDUCE:
    ret_status = MPI_Reduce_as_Allreduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    break;
  case REDUCE_AS_REDUCESCATTERBLOCK_GATHER:
    ret_status = MPI_Reduce_as_Reduce_scatter_block_Gather(sendbuf, recvbuf, count, datatype, op, root, comm);
    break;
  case REDUCE_AS_REDUCESCATTER_GATHERV:
    ret_status = MPI_Reduce_as_Reduce_scatter_Gatherv(sendbuf, recvbuf, count, datatype, op, root, comm);
    break;
  case REDUCE_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Reduce");
    PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_REDUCE, pgmpi_convert_type_count_2_bytes(count, datatype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

