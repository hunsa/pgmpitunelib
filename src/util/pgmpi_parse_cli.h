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


#ifndef SRC_PGMPI_PARSE_CLI_H_
#define SRC_PGMPI_PARSE_CLI_H_

#include "util/keyvalue_store.h"

void parse_cli_arguments(pgmpi_dictionary_t *dict, int *argc, char ***argv);

void parse_module_parameters(pgmpi_dictionary_t *dict, const char *options);

/*!
  \param module_choices pointer to algorithmic choice of a specific module
  \param argv string which is the name of the specific algorithm
  \return alg id, if alg id is not found, it returns 0 (default implementation)
*/
int pgmpi_parse_module_params_get_cid(const module_alg_choices_t *module_choices, const char *argv);

#endif /* SRC_PGMPI_PARSE_CLI_H_ */
