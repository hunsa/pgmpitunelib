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
  SCAN_DEFAULT = 0,
  SCAN_AS_EXSCAN_REDUCELOCAL = 1
#ifdef HAVE_LANE_COLL
  ,
  SCAN_AS_SCAN_HIER = 2,
  SCAN_AS_SCAN_LANE = 3
#endif
};

static alg_choice_t module_algs[] = {
    { SCAN_DEFAULT, "default" },
    { SCAN_AS_EXSCAN_REDUCELOCAL, "scan_as_exscan_reducelocal" }
#ifdef HAVE_LANE_COLL
    ,
    { SCAN_AS_SCAN_HIER, "scan_as_scan_hier" },
    { SCAN_AS_SCAN_LANE, "scan_as_scan_lane" }
#endif
};

static module_alg_choices_t module_choices = {
    sizeof(module_algs)/sizeof(alg_choice_t),
    module_algs
};


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
#ifdef HAVE_LANE_COLL
  case SCAN_AS_SCAN_HIER:
    ret_status = Scan_hier(sendbuf, recvbuf, count, datatype, op, comm);
    break;
  case SCAN_AS_SCAN_LANE:
    ret_status = Scan_lane(sendbuf, recvbuf, count, datatype, op, comm);
    break;
#endif
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

