/*
 * pgmpi_parse_cli.h
 *
 *  Created on: Feb 27, 2017
 *      Author: sascha
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
