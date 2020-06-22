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

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  REDUCESCATTERBLOCK_DEFAULT = 0,
  REDUCESCATTERBLOCK_AS_REDUCE_SCATTER = 1,
  REDUCESCATTERBLOCK_AS_REDUCESCATTER = 2,
  REDUCESCATTERBLOCK_AS_ALLREDUCE = 3,
  REDUCESCATTERBLOCK_AS_REDUCESCATTERBLOCK_HIER = 4,
  REDUCESCATTERBLOCK_AS_REDUCESCATTERBLOCK_LANE = 5,
};

static alg_choice_t module_algs[] = {
    { REDUCESCATTERBLOCK_DEFAULT, "default" },
    { REDUCESCATTERBLOCK_AS_REDUCE_SCATTER, "reducescatterblock_as_reduce_scatter" },
    { REDUCESCATTERBLOCK_AS_REDUCESCATTER, "reducescatterblock_as_reducescatter" },
    { REDUCESCATTERBLOCK_AS_ALLREDUCE, "reducescatterblock_as_allreduce" },
    { REDUCESCATTERBLOCK_AS_REDUCESCATTERBLOCK_HIER, "reducescatterblock_as_reducescatterblock_hier" },
    { REDUCESCATTERBLOCK_AS_REDUCESCATTERBLOCK_LANE, "reducescatterblock_as_reducescatterblock_lane" },
};

static module_alg_choices_t module_choices = {
    sizeof(module_algs)/sizeof(alg_choice_t),
    module_algs };

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
  case REDUCESCATTERBLOCK_AS_REDUCESCATTERBLOCK_HIER:
    ret_status = Reduce_scatter_block_hier(sendbuf, recvbuf, recvcount, datatype, op, comm);
    break;
  case REDUCESCATTERBLOCK_AS_REDUCESCATTERBLOCK_LANE:
    ret_status = Reduce_scatter_block_lane(sendbuf, recvbuf, recvcount, datatype, op, comm);
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

