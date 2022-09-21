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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_profile.h"
#include "include/collective_modules.h"

int pgmpi_profile_allocate(pgmpi_profile_t *profile, const char *mpiname, const int nb_procs,
      const int n_ranges) {
  pgmpi_collectives_t cid;

  assert(profile != NULL);

  if( pgmpi_modules_get_id_by_mpiname(mpiname, &cid) != 0 ) {
    ZF_LOGE("cannot find CID for mpiname \"%s\"", mpiname);
    exit(1);
  }
  profile->cid = cid;
  profile->n_ranges = n_ranges;
  profile->nb_procs = nb_procs;
  profile->range = (pgmpi_range_t *)malloc(n_ranges * sizeof(pgmpi_range_t));
  return 0;
}

int pgmpi_profile_free(pgmpi_profile_t *profile) {

  if( profile == NULL ) {
    ZF_LOGV("error: profile is already NULL");
    return 1;
  }

  if( profile->range == NULL) {
    return 0;
  }

  //ZF_LOGV("free range");
  free(profile->range);

  return 0;
}

int pgmpi_profile_set_alg_for_range(pgmpi_profile_t *profile, const int range_idx, const int msize_begin,
    const int msize_end, const char *algname) {
  int algid;
  module_t *mod;

  assert(profile != NULL);
  assert(algname != NULL);
  assert(msize_begin >= 0 && msize_begin <= msize_end);
  assert(range_idx >= 0 && range_idx < profile->n_ranges);

  mod = pgmpi_modules_get(profile->cid);
  if( mod == NULL ) {
    ZF_LOGE("cannot find module with id %d", profile->cid);
    exit(1);
  }
  algid = pgmpi_modules_get_algid_by_algname(mod->alg_choices, algname);
  if( algid < 0 ) {
    ZF_LOGE("cannot find alg id for algname: %s", algname);
    exit(1);
  }

  profile->range[range_idx].msg_size_start = msize_begin;
  profile->range[range_idx].msg_size_end   = msize_end;
  profile->range[range_idx].alg_id = algid;

  return 0;
}

int pgmpi_profile_find_alg(pgmpi_profile_t *profile, const int msize, int *algid) {
  int i;
  int ret = -1;

  // TODO: to bisection!
  for(i=0; i<profile->n_ranges; i++) {
    if( profile->range[i].msg_size_start <= msize &&
        profile->range[i].msg_size_end   >= msize) {
      *algid = profile->range[i].alg_id;
      ret = 0;
      break;
    }
  }

  return ret;
}

char *pgmpi_get_profile_path() {
  char *rpath;
  rpath = getenv ("PGMPI_PROFILE_PATH");
  if(rpath != NULL) {
    rpath = strdup(rpath);
  }
  return rpath;
}

char *pgmpi_get_profile_file_suffix() {
  return "prf";
}


