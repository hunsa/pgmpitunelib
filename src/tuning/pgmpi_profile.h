/*
 * pgmpi_profile.h
 *
 *  Created on: Mar 2, 2017
 *      Author: sascha
 */


/*
 * a profile contains several algselections
 */

#ifndef SRC_TUNING_PGMPI_PROFILE_H_
#define SRC_TUNING_PGMPI_PROFILE_H_


#include "pgmpi_tune.h"

typedef struct {
  int msg_size_start;
  int msg_size_end; /* communication amount in bytes */
  int alg_id;       /* selected alg (should be != 0) */
} pgmpi_range_t;

typedef struct {
  pgmpi_collectives_t cid;
  int nb_procs;
  int n_ranges;   /* number of ranges of message sizes in selection */
  pgmpi_range_t *range;
} pgmpi_profile_t;

int pgmpi_profile_allocate(pgmpi_profile_t *profile, const char *mpiname, const int nb_procs,
      const int n_ranges);

int pgmpi_profile_free(pgmpi_profile_t *profile);

int pgmpi_profile_set_alg_for_range(pgmpi_profile_t *profile, const int range_idx, const int msize_begin,
    const int msize_end, const char *algname);

int pgmpi_profile_find_alg(pgmpi_profile_t *profile, const int msize, int *algid);

char *pgmpi_get_profile_path();
char *pgmpi_get_profile_file_suffix();

#endif

