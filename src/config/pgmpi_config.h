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


#ifndef SRC_CONFIG_PGMPI_CONFIG_H_
#define SRC_CONFIG_PGMPI_CONFIG_H_

#include <stdio.h>

#include "util/keyvalue_store.h"

typedef struct {
  pgmpi_dictionary_t* config_dict;
} pgmpi_config_t;


void pgmpi_config_init();

void pgmpi_config_free();

void pgmpi_config_add_key_value(const char *key, const char *val);

void pgmpi_config_get_keys(char ***keys, int *nkeys);

int pgmpi_config_get_long_value(const char *key, unsigned long *val);

int pgmpi_config_get_string_value(const char *key, char **val);

void pgmpi_config_print(FILE *fp);

char *pgmpi_config_get_filename_from_env();

#endif /* SRC_CONFIG_PGMPI_CONFIG_H_ */
