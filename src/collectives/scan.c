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
  SCAN_DEFAULT = 0,
  SCAN_AS_EXSCAN_REDUCELOCAL = 1
};

static alg_choice_t module_algs[4] = {
    { SCAN_DEFAULT, "default" },
    { SCAN_AS_EXSCAN_REDUCELOCAL, "scan_as_exscan_reducelocal" },
};

static module_alg_choices_t module_choices = { 2, module_algs };


static int alg_id = SCAN_DEFAULT;

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

void register_module_scan(module_t *module) {
  module->cli_prefix  = "scan";
  module->mpiname     = "MPI_Scan";
  module->cid         = CID_MPI_SCAN;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;
}

void deregister_module_scan(module_t *module) {
}

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

int MPI_Scan(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Scan");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_SCAN, pgmpi_convert_type_count_2_bytes(count, datatype), size, &alg_id);
  }

  switch (alg_id) {
  case SCAN_AS_EXSCAN_REDUCELOCAL:
    ret_status = MPI_Scan_as_Exscan_Reduce_local(sendbuf, recvbuf, count, datatype, op, comm);
    break;
  case SCAN_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Scan");
    PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_SCAN, pgmpi_convert_type_count_2_bytes(count, datatype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

