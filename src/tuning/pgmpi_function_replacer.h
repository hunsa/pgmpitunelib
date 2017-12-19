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


#ifndef SRC_TUNING_PGMPI_FUNCTION_REPLACER_H_
#define SRC_TUNING_PGMPI_FUNCTION_REPLACER_H_

#include "pgmpi_tune.h"
#include "pgmpi_profile.h"

typedef struct {
  int num_collectives;
  pgmpi_profile_t **profile;
} alg_lookup_table_t;

int pgmpi_find_replacement_algorithm(const alg_lookup_table_t *tab, const pgmpi_collectives_t cid, const int msg_size,
    const int comm_size, int *alg_id);

int pgmpi_get_profile(const alg_lookup_table_t *tab, const pgmpi_collectives_t cid, pgmpi_profile_t **prof);

int pgmpi_allocate_replacement_table(alg_lookup_table_t *tab);

int pgmpi_free_replacement_table(alg_lookup_table_t *tab);

int pgmpi_add_profile_to_table(alg_lookup_table_t *tab, pgmpi_profile_t *profile);

#endif /* SRC_TUNING_PGMPI_FUNCTION_REPLACER_H_ */
