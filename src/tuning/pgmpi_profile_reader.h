/*
 * pgmpi_profile_reader.h
 *
 *  Created on: Mar 2, 2017
 *      Author: sascha
 */

#ifndef SRC_TUNING_PGMPI_PROFILE_READER_H_
#define SRC_TUNING_PGMPI_PROFILE_READER_H_


#include "pgmpi_profile.h"

int pgmpi_profile_read(const char *fname, pgmpi_profile_t *profile);

int pgmpi_profile_read_many(const char *path, pgmpi_profile_t **profiles, int *num_profiles);


#endif /* SRC_TUNING_PGMPI_PROFILE_READER_H_ */
