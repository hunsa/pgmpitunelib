
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
static const int mockup_root_rank = 0;

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  ALLREDUCE_DEFAULT = 0,
  ALLREDUCE_AS_REDUCE_BCAST = 1,
  ALLREDUCE_AS_REDUCESCATTERBLOCK_ALLGATHER = 2,
  ALLREDUCE_AS_REDUCESCATTER_ALLGATHERV = 3
};

static alg_choice_t module_algs[4] = {
    { ALLREDUCE_DEFAULT, "default" },
    { ALLREDUCE_AS_REDUCE_BCAST, "allreduce_as_reduce_bcast" },
    { ALLREDUCE_AS_REDUCESCATTERBLOCK_ALLGATHER, "allreduce_as_reducescatterblock_allgather" },
    { ALLREDUCE_AS_REDUCESCATTER_ALLGATHERV, "allreduce_as_reducescatter_allgatherv" }
};

static module_alg_choices_t module_choices = { 4, module_algs };

static int alg_id = ALLREDUCE_DEFAULT;

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


void register_module_allreduce(module_t *module) {
  module->cli_prefix = "allreduce";
  module->mpiname    = "MPI_Allreduce";
  module->cid         = CID_MPI_ALLREDUCE;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 0;
}

void deregister_module_allreduce(module_t *module) {
}


int MPI_Allreduce_as_Reduce_Bcast(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

  ZF_LOGV("Calling MPI_Allreduce_as_Reduce_Bcast");

  // count is the buffer size per process
  PGMPI(MPI_Reduce(sendbuf, recvbuf, count, datatype, op, mockup_root_rank, comm));
  PGMPI(MPI_Bcast(recvbuf, count, datatype, mockup_root_rank, comm));

  return MPI_SUCCESS;
}

int MPI_Allreduce_as_Reduce_scatter_block_Allgather(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int n, i;
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
  MPI_Aint padding_offset;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Allreduce_as_Reduce_scatter_block_Allgather");

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

  // fill in the padded elements to make sure we apply Reduce on user data
  padding_offset = n * type_extent;
  for (i=0; i<n_padding_elems; i++) {
    memcpy((char*)sendbuf1 + padding_offset, sendbuf, type_extent);  // only use the first element from sendbuf

    padding_offset += type_extent;
  }

  PGMPI(MPI_Reduce_scatter_block(sendbuf1, recvbuf1, recvcount1, datatype, op, comm));

  sendbuf2 = recvbuf1;   // send buffer per process (reuse previous receive buffer)
  recvbuf2 = aux_buf1;   // padded receive buffer to hold the final result
  sendcount2 = recvcount1;
  recvcount2 = recvcount1;

  PGMPI(MPI_Allgather(sendbuf2, sendcount2, datatype, recvbuf2, recvcount2, datatype, comm));

  // copy only the needed data (n elements) to the final receive buffer
  memcpy(recvbuf, recvbuf2, n * type_extent);

  release_msg_buffers();
  return MPI_SUCCESS;
}


int MPI_Allreduce_as_Reduce_scatter_Allgatherv(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int n;
  int nchunks;
  int i;
  int *recvcounts;
  int *displs;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_int_buf_size;
  int *aux_int_buf1, *aux_int_buf2;
  void *aux_buf;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Allreduce_as_Reduce_scatter_Allgatherv");

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

  nchunks = n / MIN_SCATTER_CHUNK_SIZE;   // handle the remainder separately
  recvcounts = aux_int_buf1;

  // assign chunks to processes (round robin) - at least MIN_SCATTER_CHUNK_SIZE elements per process
  for (i = 0; i < size; i++) {
    recvcounts[i] = MIN_SCATTER_CHUNK_SIZE * (nchunks/size);

    if (i < nchunks % size) {
      recvcounts[i] += MIN_SCATTER_CHUNK_SIZE;
    } else if (i == nchunks % size ) { // last chunk can be smaller than MIN_SCATTER_CHUNK_SIZE
      recvcounts[i] += n % MIN_SCATTER_CHUNK_SIZE;
    }
  }

  ZF_LOGV("nchunks=%d count=%d i=%d", nchunks, n, i);

  if (rank == mockup_root_rank) {
    for (i = 0; i < size; i++) {
      ZF_LOGV("recvcounts[%d]=%d", i, recvcounts[i]);
    }
  }

  displs = aux_int_buf2;
  displs[0] = 0;
  for (i = 1; i < size; i++) {
    displs[i] = displs[i - 1] + recvcounts[i - 1];
  }

  // set the pointer where the scatter results will be saved for the current rank within the final receive buffer
  if (recvcounts[rank] > 0) {
    aux_buf = (char*)(recvbuf) + displs[rank] * type_extent;
  } else {  // point at the beginning of the receive buffer when there is nothing to receive
     aux_buf = recvbuf;
  }

  PGMPI(MPI_Reduce_scatter(sendbuf, aux_buf, recvcounts, datatype, op, comm));
  PGMPI(MPI_Allgatherv(MPI_IN_PLACE, 0, datatype, recvbuf, recvcounts, displs, datatype, comm));


  release_msg_buffers();
  release_int_buffers();
  return MPI_SUCCESS;
}


int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Allreduce");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void) pgtune_get_algorithm(CID_MPI_ALLREDUCE, pgmpi_convert_type_count_2_bytes(count, datatype), size, &alg_id);
  }

  switch (alg_id) {
  case ALLREDUCE_AS_REDUCE_BCAST:
    ret_status = MPI_Allreduce_as_Reduce_Bcast(sendbuf, recvbuf, count, datatype, op, comm);
    break;
  case ALLREDUCE_AS_REDUCESCATTERBLOCK_ALLGATHER:
    ret_status = MPI_Allreduce_as_Reduce_scatter_block_Allgather(sendbuf, recvbuf, count, datatype, op, comm);
    break;
  case ALLREDUCE_AS_REDUCESCATTER_ALLGATHERV:
    ret_status = MPI_Allreduce_as_Reduce_scatter_Allgatherv(sendbuf, recvbuf, count, datatype, op, comm);
    break;
  case ALLREDUCE_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Allreduce");
    PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_ALLREDUCE, pgmpi_convert_type_count_2_bytes(count, datatype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

