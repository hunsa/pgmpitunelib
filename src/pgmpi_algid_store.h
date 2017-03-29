/*
 * pgmpi_algid_store.h
 *
 *  Created on: Mar 8, 2017
 *      Author: sascha
 */

#ifndef SRC_PGMPI_ALGID_STORE_H_
#define SRC_PGMPI_ALGID_STORE_H_

#include "pgmpi_tune.h"

void pgmpi_init_algid_maps();

void pgmpi_free_algid_maps();

void pgmpi_save_algid_for_msg_size(const pgmpi_collectives_t cid, const int msg_size, const int alg_id,
    const int called_default);

void pgmpi_print_algids(FILE *fp);

#endif /* SRC_PGMPI_ALGID_STORE_H_ */
