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
#include "collective_modules.h"
#include "pgmpi_algid_store.h"
#include "util/pgmpi_parse_cli.h"
#include "all_guideline_collectives.h"

/********************************/

static void parse_arguments(const char *argv);
static void set_algid(const int algid);

/********************************/

enum mockups {
  SCATTER_DEFAULT = 0,
  SCATTER_AS_BCAST = 1,
  SCATTER_AS_SCATTERV = 2
#ifdef HAVE_LANE_COLL
  ,
  SCATTER_AS_SCATTER_HIER = 3,
  SCATTER_AS_SCATTER_LANE = 4
#endif
};

static alg_choice_t module_algs[] = {
    { SCATTER_DEFAULT, "default" },
    { SCATTER_AS_BCAST, "scatter_as_bcast" },
    { SCATTER_AS_SCATTERV, "scatter_as_scatterv" }
#ifdef HAVE_LANE_COLL
    ,
    { SCATTER_AS_SCATTER_HIER, "scatter_as_scatter_hier" },
    { SCATTER_AS_SCATTER_LANE, "scatter_as_scatter_lane" }
#endif
};

static module_alg_choices_t module_choices = {
    sizeof(module_algs)/sizeof(alg_choice_t),
    module_algs
};

static int alg_id = SCATTER_DEFAULT;

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

void register_module_scatter(module_t *module) {
  module->cli_prefix  = "scatter";
  module->mpiname     = "MPI_Scatter";
  module->cid         = CID_MPI_SCATTER;
  module->parse       = &parse_arguments;
  module->alg_choices = &module_choices;
  module->set_algid   = &set_algid;
  module->is_rooted   = 1;
}

void deregister_module_scatter(module_t *module) {
}


int MPI_Scatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
    MPI_Datatype recvtype, int root, MPI_Comm comm) {
  int ret_status = MPI_SUCCESS;
  int call_default = 0;
  int size;

  ZF_LOGV("Intercepting MPI_Scatter");

  MPI_Comm_size(comm, &size);

  if( get_pgmpi_context() == CONTEXT_TUNED ) {
    (void)pgtune_get_algorithm(CID_MPI_SCATTER, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), size, &alg_id);
  }

  switch (alg_id) {
  case SCATTER_AS_BCAST:
    ret_status = MPI_Scatter_as_Bcast(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case SCATTER_AS_SCATTERV:
    ret_status = MPI_Scatter_as_Scatterv(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
#ifdef HAVE_LANE_COLL
  case SCATTER_AS_SCATTER_HIER:
    ret_status = Scatter_hier(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
  case SCATTER_AS_SCATTER_LANE:
    ret_status = Scatter_lane(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    break;
#endif
  case SCATTER_DEFAULT:
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
    ZF_LOGV("Calling PMPI_Scatter");
    PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
  }

  if (PGMPI_ENABLE_ALGID_STORING) {
    pgmpi_save_algid_for_msg_size(CID_MPI_SCATTER, pgmpi_convert_type_count_2_bytes(sendcount, sendtype), alg_id,
        call_default);
  }

  return MPI_SUCCESS;
}

