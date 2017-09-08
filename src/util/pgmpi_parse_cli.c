/*
 * pgmpi_parse_cli.c
 *
 *  Created on: Feb 27, 2017
 *      Author: sascha
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pgmpi_tune.h"

#define ZF_LOG_LEVEL MY_ZF_LOG_LEVEL
#include "log/zf_log.h"


#include "pgmpi_parse_cli.h"
#include "collectives/collective_modules.h"
#include "util/keyvalue_store.h"



void get_key_val_from_env(char *arg, char **key, char **val) {
  char *work;
  int arg_l;
  int i, pos;

  if( arg == NULL ) {
    ZF_LOGE("argument is NULL, program error");
    return;
  }

  *key = NULL;
  *val = NULL;

  ZF_LOGV("arg is %s", arg);
  arg_l = strlen(arg);
  ZF_LOGV("arg_l is %d", arg_l);

  work = strdup(arg);

  pos = -1;
  for(i=0; i<arg_l; i++) {
    if( work[i] == '=' ) {
      pos = i;
      break;
    }
  }

  if( pos > 0 ) {
    int key_l, val_l;


    val_l = (arg_l - (pos+1)) + 1;  // + '\0'
    key_l = pos + 1; // (pos chars + '\0'  -> ignore "=" at pos

//    ZF_LOGV("val_l = %d, key_l=%d", val_l, key_l);

    *key = (char*)calloc(key_l, sizeof(char));
    *val = (char*)calloc(val_l, sizeof(char));

    memcpy(*key, &arg[0], key_l-1);
    (*key)[key_l-1] = '\0';

    memcpy(*val, &arg[pos+1], val_l-1);
    (*val)[val_l-1] = '\0';

//    ZF_LOGV("key:%s val:%s", *key, *val);
  }

  free(work);
}

// -m allgather=alg:1

void parse_cli_arguments(pgmpi_dictionary_t *dict, int *argc, char ***argv) {
  int i;

  for(i=0; i<*argc; i++) {
    // ZF_LOGV("argv[%i]=%s", i, (*argv)[i]);
    char *arg_key = NULL, *arg_val = NULL;
    char *arg;

    arg = strdup((*argv)[i]);
    get_key_val_from_env(arg, &arg_key, &arg_val);
    free(arg);

    if( arg_key == NULL || arg_val == NULL ) {
      continue;
    }

    if( strcmp(arg_key, "--module") == 0 ) {
      char *key = NULL, *val = NULL;
      //ZF_LOGV("param_str: %s", arg_val);

      key = strtok(arg_val, "=");
      if( key != NULL ) {
        val = strtok(NULL, "=");
      }

      if( key != NULL && val != NULL ) {
        ZF_LOGV("key=%s, val=%s", key, val);
        pgmpitune_add_element_to_dict(dict, key, val);
      } else {
        ZF_LOGW("cannot parse: %s", val);
      }

      free(arg_key);
      free(arg_val);

    } else if( strcmp(arg_key, "--config") == 0 ) {
      ZF_LOGV("adding config_file %s", arg_val);
      pgmpitune_add_element_to_dict(dict, "config_file", arg_val);
    } else if( strcmp(arg_key, "--ppath") == 0 ) {
      ZF_LOGV("adding profile_path %s", arg_val);
      pgmpitune_add_element_to_dict(dict, "profile_path", arg_val);
    }

  }

}


void parse_module_parameters(pgmpi_dictionary_t *dict, const char *options) {
  char* token;
  char* workargv;

  workargv = strdup(options);
  while( (token = strtok(workargv, ",")) != NULL ) {
    workargv = NULL;
    char *key, *val;

    ZF_LOGV("token=%s", token);
    key = strtok(token, ":");
    if( key != NULL ) {
      val = strtok(NULL, ":");
    }
    if( key != NULL && val != NULL ) {
      pgmpitune_add_element_to_dict(dict, key, val);
    } else {
      ZF_LOGW("token %s invalid", token);
    }
  }

  free(workargv);
}

int pgmpi_parse_module_params_get_cid(const module_alg_choices_t *module_choices, const char *argv) {
  pgmpi_dictionary_t hashmap;
  int alg_id = 0;
  char *alg_name = NULL;

  ZF_LOGV("parse arguments");
  if( argv == NULL ) {
    ZF_LOGW("argv is NULL");
    return alg_id;
  }

  pgmpitune_init_dictionary(&hashmap);
  parse_module_parameters(&hashmap, argv);

  if( pgmpitune_dict_has_key(&hashmap, "alg") ) {
    alg_name = pgmpitune_get_value_from_dict(&hashmap, "alg");
    if( alg_name != NULL ) {
      alg_id = pgmpi_modules_get_algid_by_algname(module_choices, alg_name);
      if( alg_id < 0 ) {
        ZF_LOGE("cannot find algorithm name \"%s\", reset alg to 0", alg_name);
        alg_id = 0;
      }
      free(alg_name);
    } else {
      ZF_LOGW("alg_name from dict is NULL");
    }
  }
  pgmpitune_cleanup_dictionary(&hashmap);

  return alg_id;
}

