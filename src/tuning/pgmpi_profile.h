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

