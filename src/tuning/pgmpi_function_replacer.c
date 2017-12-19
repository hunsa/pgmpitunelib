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


#ifndef SRC_TUNING_PGMPI_FUNCTION_REPLACER_C_
#define SRC_TUNING_PGMPI_FUNCTION_REPLACER_C_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"

#include "pgmpi_function_replacer.h"

int pgmpi_find_replacement_algorithm(const alg_lookup_table_t *tab, const pgmpi_collectives_t cid, const int msg_size,
    const int comm_size, int *alg_id) {

  pgmpi_profile_t *profile = NULL;

  if (tab == NULL) {
    ZF_LOGE("tab is NULL");
    return -1;
  }

  if (cid < 0 || cid >= tab->num_collectives) {
    ZF_LOGE("cid invalid");
    return -1;
  }

  if (msg_size <= 0) {
    ZF_LOGE("msg size <= 0, invalid!");
    return -1;
  }

  if( pgmpi_get_profile(tab, cid, &profile) != 0 ) {
    ZF_LOGE("cannot find correct profile for cid=%d", cid);
    return -1;
  }
  if( profile == NULL ) {
    // there is no profile for that collective
    ZF_LOGV("no profile for collective with id %u", cid);
    return -1;
  }

  if( profile->nb_procs != comm_size ) {
    ZF_LOGW("profile does not match number of processes");
    return -1;
  }

  return pgmpi_profile_find_alg(profile, msg_size, alg_id);

}

int pgmpi_get_profile(const alg_lookup_table_t *tab, const pgmpi_collectives_t cid, pgmpi_profile_t **prof) {
  assert(cid >= 0 || cid >= tab->num_collectives );
  //ZF_LOGV("tab profile at %u is %p", cid, tab->profile[cid]);
  *prof = tab->profile[cid];
  return 0;
}


int pgmpi_allocate_replacement_table(alg_lookup_table_t *tab) {
  if (tab == NULL) {
    ZF_LOGE("tab is NULL");
    return -1;
  }
  tab->num_collectives = NUM_COLLECTIVES;
  tab->profile = (pgmpi_profile_t **) calloc(NUM_COLLECTIVES, sizeof(pgmpi_profile_t*));
  return 0;
}

int pgmpi_add_profile_to_table(alg_lookup_table_t *tab, pgmpi_profile_t *profile) {
  if (tab == NULL) {
    ZF_LOGE("tab is NULL");
    return -1;
  }
  if (profile == NULL) {
    ZF_LOGE("cannot add profile that is NULL");
    return -1;
  }
  if (profile->cid < 0 || profile->cid >= tab->num_collectives) {
    ZF_LOGE("cid invalid");
    return -1;
  }

  ZF_LOGV("add profile at %u (address %p)", profile->cid, profile);
  tab->profile[profile->cid] = profile;

  return 0;
}

int pgmpi_free_replacement_table(alg_lookup_table_t *tab) {
  int i;

  if (tab == NULL) {
    ZF_LOGE("tab is NULL");
    return -1;
  }

  for(i=0; i<tab->num_collectives; i++) {
    if( tab->profile[i] != NULL ) {
      //ZF_LOGV("free profile at %i (%p)", i, tab->profile[i]);
      pgmpi_profile_free(tab->profile[i]);
    }
  }

  free(tab->profile);

  return 0;
}

#endif /* SRC_TUNING_PGMPI_FUNCTION_REPLACER_C_ */

