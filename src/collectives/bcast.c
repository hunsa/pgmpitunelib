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
  BCAST_DEFAULT = 0,
  BCAST_AS_ALLGATHERV = 1,
  BCAST_AS_SCATTER_ALLGATHER = 2
};


static alg_choice_t module_algs[3] = {
    { BCAST_DEFAULT, "default" },
    { BCAST_AS_ALLGATHERV, "bcast_as_allgatherv" },
    { BCAST_AS_SCATTER_ALLGATHER, "bcast_as_scatter_allgather" }
};

static module_alg_choices_t module_choices = { 3, module_algs };

static int alg_id = BCAST_DEFAULT;

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


void register_module_bcast(module_t *module) {
  module->cli_prefix  = "bcast";
  module->mpiname     = "MPI_Bcast";
  module->cid         = CID_MPI_BCAST;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;
}

void deregister_module_bcast(module_t *module) {
}

int MPI_Bcast_as_Allgatherv(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  int i;
  int *recvcounts;
  int *displs;
  int rank, size;
  size_t fake_int_buf_size;
  int *aux_int_buf1, *aux_int_buf2;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);

  ZF_LOGV("Calling MPI_Bcast_as_Allgatherv");

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

  recvcounts = aux_int_buf1;
  displs = aux_int_buf2;

  for (i = 0; i < size; i++) {
    if (i == root) {
      recvcounts[i] = count;
      displs[i] = 0;
    } else {
      if (i < root) {
        recvcounts[i] = 0;
        displs[i] = 0;
      } else { // i > root
        recvcounts[i] = 0;
        displs[i] = count;
      }
    }
  }

  PGMPI(MPI_Allgatherv(MPI_IN_PLACE, 0, datatype, buffer, recvcounts, displs, datatype, comm));

  release_int_buffers();
  return MPI_SUCCESS;
}

int MPI_Bcast_as_Scatter_Allgather(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  int n;
  int n_padding_elems;
  void *sendbuf1;
  void *recvbuf1;
  void *sendbuf2;
  void *recvbuf2;
  int scattered_count;
  int rank, size;
  MPI_Aint type_extent, lb;
  size_t fake_buf_size1, fake_buf_size2;
  void *aux_buf1, *aux_buf2;
  int buf_status = BUF_NO_ERROR;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &size);
  MPI_Type_get_extent(datatype, &lb, &type_extent);

  ZF_LOGV("Calling MPI_Bcast_as_Scatter_Allgather");

  // we need two fake buffers: aux_buf1 with (n + padding) elements
  //    and aux_buf2 with (n + padding)/size elements
  n = count;            // buffer size broadcasted to all processes
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
  scattered_count = fake_buf_size2 / type_extent;

  // copy the send buffer to the beginning of the padded temporary buffer
  if (rank == root) { // buffer is only filled in at the root
    memcpy(sendbuf1, buffer, n * type_extent);
  }

  PGMPI(MPI_Scatter(sendbuf1, scattered_count, datatype, recvbuf1, scattered_count, datatype, root, comm));

  sendbuf2 = recvbuf1;   // send buffer per process (reuse previous receive buffer)
  recvbuf2 = aux_buf1;   // padded receive buffer to hold the final result

  PGMPI(MPI_Allgather(sendbuf2, scattered_count, datatype, recvbuf2, scattered_count, datatype, comm));

  // copy only the needed data (n elements) to the final receive buffer
  if (rank != root) {
    memcpy(buffer, recvbuf2, n * type_extent);
  }

  release_msg_buffers();
  return MPI_SUCCESS;
}

int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Bcast");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_BCAST, pgmpi_convert_type_count_2_bytes(count, datatype), size, &alg_id);
  }

  switch (alg_id) {
  case BCAST_AS_ALLGATHERV:
    ret_status = MPI_Bcast_as_Allgatherv(buffer, count, datatype, root, comm);
    break;
  case BCAST_AS_SCATTER_ALLGATHER:
    ret_status = MPI_Bcast_as_Scatter_Allgather(buffer, count, datatype, root, comm);
    break;
  case BCAST_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Bcast");
    PMPI_Bcast(buffer, count, datatype, root, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_BCAST, pgmpi_convert_type_count_2_bytes(count, datatype), alg_id,
        call_default);
  }


  return MPI_SUCCESS;
}

