/*
 * pgmpi_function_replacer.h
 *
 *  Created on: Mar 2, 2017
 *      Author: sascha
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
