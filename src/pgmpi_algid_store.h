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


#ifndef SRC_PGMPI_ALGID_STORE_H_
#define SRC_PGMPI_ALGID_STORE_H_

#include "pgmpi_tune.h"

void pgmpi_init_algid_maps();

void pgmpi_free_algid_maps();

void pgmpi_save_algid_for_msg_size(const pgmpi_collectives_t cid, const int msg_size, const int alg_id,
    const int called_default);

void pgmpi_print_algids(FILE *fp);

int pgmpi_convert_type_count_2_bytes(const int count, const MPI_Datatype type);

#endif /* SRC_PGMPI_ALGID_STORE_H_ */
