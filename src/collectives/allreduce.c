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

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_tune.h"
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
  ALLREDUCE_DEFAULT = 0,
  ALLREDUCE_AS_REDUCE_BCAST = 1,
  ALLREDUCE_AS_REDUCESCATTERBLOCK_ALLGATHER = 2,
  ALLREDUCE_AS_REDUCESCATTER_ALLGATHERV = 3
#ifdef HAVE_LANE_COLL
  ,
  ALLREDUCE_AS_ALLREDUCE_LANE = 4,
  ALLREDUCE_AS_ALLREDUCE_HIER = 5
#endif
#ifdef HAVE_CIRCULANTS
  ,
  ALLREDUCE_AS_ALLREDUCE_CIRCULANT = 6,
#endif
};

static alg_choice_t module_algs[] = {
    { ALLREDUCE_DEFAULT, "default" },
    { ALLREDUCE_AS_REDUCE_BCAST, "allreduce_as_reduce_bcast" },
    { ALLREDUCE_AS_REDUCESCATTERBLOCK_ALLGATHER, "allreduce_as_reducescatterblock_allgather" },
    { ALLREDUCE_AS_REDUCESCATTER_ALLGATHERV, "allreduce_as_reducescatter_allgatherv" }
#ifdef HAVE_LANE_COLL
,
    { ALLREDUCE_AS_ALLREDUCE_LANE, "allreduce_as_allreduce_lane" },
    { ALLREDUCE_AS_ALLREDUCE_HIER, "allreduce_as_allreduce_hier" }
#endif
#ifdef HAVE_CIRCULANTS
    ,
    { ALLREDUCE_AS_ALLREDUCE_CIRCULANT, "allreduce_as_allreduce_circulant" },
#endif
};

static module_alg_choices_t module_choices = {
    sizeof(module_algs)/sizeof(alg_choice_t),
    module_algs };

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
#ifdef HAVE_LANE_COLL
  case ALLREDUCE_AS_ALLREDUCE_LANE:
    ret_status = Allreduce_lane(sendbuf, recvbuf, count, datatype, op, comm);
    break;
  case ALLREDUCE_AS_ALLREDUCE_HIER:
    ret_status = Allreduce_hier(sendbuf, recvbuf, count, datatype, op, comm);
    break;
#endif
#ifdef HAVE_CIRCULANTS
  case ALLREDUCE_AS_ALLREDUCE_CIRCULANT:
    ret_status = Allreduce_circulant(sendbuf, recvbuf, count, datatype, op, comm);
    break;
#endif
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

