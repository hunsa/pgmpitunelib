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

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "bufmanager/pgmpi_buf.h"
#include "pgmpi_algid_store.h"
#include "collectives/collective_modules.h"
#include "util/pgmpi_parse_cli.h"
#include "all_guideline_collectives.h"

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  ALLTOALL_DEFAULT = 0,
  ALLTOALL_AS_ALLTOALLV = 1
#ifdef HAVE_LANE_COLL
  ,
  ALLTOALL_AS_ALLTOALL_LANE = 2
#endif
};

static alg_choice_t module_algs[] = {
    { ALLTOALL_DEFAULT, "default" },
    { ALLTOALL_AS_ALLTOALLV, "alltoall_as_alltoallv" }
#ifdef HAVE_LANE_COLL
    ,
    { ALLTOALL_AS_ALLTOALL_LANE, "alltoall_as_alltoall_lane" }
#endif
};

static module_alg_choices_t module_choices = {
    sizeof(module_algs)/sizeof(alg_choice_t),
    module_algs };

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


int MPI_Alltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Alltoall");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void) pgtune_get_algorithm(CID_MPI_ALLTOALL, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), size, &alg_id);
  }

  switch (alg_id) {
  case ALLTOALL_AS_ALLTOALLV:
    ret_status = MPI_Alltoall_as_Alltoallv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    break;
#ifdef HAVE_LANE_COLL
  case ALLTOALL_AS_ALLTOALL_LANE:
    ret_status = Alltoall_lane(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    break;
#endif
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
    pgmpi_save_algid_for_msg_size(CID_MPI_ALLTOALL, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

