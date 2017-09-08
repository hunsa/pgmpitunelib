/*
 * pgmpi_config.h
 *
 *  Created on: Mar 9, 2017
 *      Author: sascha
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
