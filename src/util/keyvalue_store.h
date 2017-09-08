/*  ReproMPI Benchmark
 *
 *  Copyright 2015 Alexandra Carpen-Amarie, Sascha Hunold
    Research Group for Parallel Computing
    Faculty of Informatics
    Vienna University of Technology, Austria

<license>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
</license>
*/

#ifndef PGMPI_KEYVALUE_STORE_H_
#define PGMPI_KEYVALUE_STORE_H_

#include <stdio.h>

typedef struct pgmpitune_params_keyval {
    char* key;
    char* value;
} pgmpi_dict_keyval_t;


typedef struct pgmpitune_dictionary {
    pgmpi_dict_keyval_t* data;
    int n_elems;
    int size;
} pgmpi_dictionary_t;

typedef enum {
    DICT_SUCCESS = 0,
    DICT_KEY_ERROR,
    DICT_ERROR_NULL_KEY,
    DICT_ERROR_NULL_VALUE
} pgmpitune_dict_error_t;



void pgmpitune_init_dictionary(pgmpi_dictionary_t* dict);
void pgmpitune_cleanup_dictionary(pgmpi_dictionary_t* dict);
pgmpitune_dict_error_t pgmpitune_add_element_to_dict(pgmpi_dictionary_t* dict, const char* key, const char* val);
char* pgmpitune_get_value_from_dict(const pgmpi_dictionary_t* dict, const char* key);
pgmpitune_dict_error_t pgmpitune_remove_element_from_dict(pgmpi_dictionary_t* dict, const char* key);
pgmpitune_dict_error_t pgmpitune_get_keys_from_dict(const pgmpi_dictionary_t* dict, char ***keys, int *length);
int pgmpitune_dict_is_empty(const pgmpi_dictionary_t* dict);
int pgmpitune_dict_get_length(const pgmpi_dictionary_t* dict);
int pgmpitune_dict_has_key(const pgmpi_dictionary_t* dict, const char *key);

void pgmpitune_print_dictionary(const pgmpi_dictionary_t* dict, FILE* f);


#endif /* PGMPI_KEYVALUE_STORE_H_ */
