/*
 * Copyright (c) 2018 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: pas_main.h
 */

#ifndef __PAS_MAIN_H__
#define __PAS_MAIN_H__

#include "std_error_codes.h"
#include "cps_api_operation.h"

/* define PAS error codes */
enum e_pas_error_codes {
     PAS_CPS_WRITE_OPERATION_INVALID     = 100,
     PAS_CPS_OPERATION_SUBSYS_INIT_ERR   = 101,
     PAS_OBJECT_CRREATE_ERR              = 102,
     PAS_OBJECT_LIST_APPEND_ERR          = 103,
};

/* Top-level CPS API handler functions */

cps_api_return_code_t dn_pas_read_function (
                          void *context, 
                          cps_api_get_params_t *param, 
                          size_t key_ix
                                            );

cps_api_return_code_t dn_pas_write_function(
                          void *context, 
                          cps_api_transaction_params_t *param, 
                          size_t index_of_element_being_updated
                                            );

cps_api_return_code_t dn_pas_rollback_function(
                          void *context, 
                          cps_api_transaction_params_t *param, 
                          size_t index_of_element_being_updated
                                               );


/* cps get and set  handlers */

/* CPS get handler for comm-dev CPS object */

t_std_error dn_pas_comm_dev_get(cps_api_get_params_t *param, size_t key);

/* CPS set handler for comm-dev CPS object */

t_std_error dn_pas_comm_dev_set(cps_api_transaction_params_t *param,
                                cps_api_object_t obj);

/* CPS get handler for host-system CPS object */

t_std_error dn_pas_host_system_get(cps_api_get_params_t * param,
                                   size_t key_idx);

/* CPS set handler for host-system CPS object */

t_std_error dn_pas_host_system_set(cps_api_transaction_params_t* param,
                                   cps_api_object_t obj);

t_std_error dn_pas_status_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_chassis_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_chassis_set(cps_api_transaction_params_t *param, 
                               cps_api_object_t obj);

t_std_error dn_pas_entity_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_entity_set(cps_api_transaction_params_t * param,
                              cps_api_object_t obj);

t_std_error dn_pas_psu_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_psu_set(cps_api_key_t *key, cps_api_object_t obj);

t_std_error dn_pas_fan_tray_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_fan_tray_set(cps_api_key_t *key, cps_api_object_t obj);

t_std_error dn_pas_card_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_card_set(cps_api_key_t *key, cps_api_object_t obj);

t_std_error dn_pas_fan_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_fan_set(cps_api_transaction_params_t * param,
                           cps_api_object_t obj
                           );

t_std_error dn_pas_led_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_led_set(cps_api_transaction_params_t *param,
                           cps_api_object_t obj
                           );

t_std_error dn_pas_display_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_display_set(cps_api_transaction_params_t *param,
                               cps_api_object_t obj
                               );

t_std_error dn_pas_temperature_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_temperature_set(cps_api_transaction_params_t * param,
                                   cps_api_object_t obj
                                   );

t_std_error dn_pas_temp_threshold_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_temp_threshold_set(cps_api_transaction_params_t * param,
                                      cps_api_object_t obj
                                      );

t_std_error dn_pas_pld_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_pld_set(cps_api_key_t *key, cps_api_object_t obj);

t_std_error dn_pas_port_module_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_port_module_set(cps_api_key_t *key, cps_api_object_t obj);

t_std_error dn_pas_media_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_media_set(cps_api_transaction_params_t * param,
                cps_api_object_t obj);
t_std_error dn_pas_media_channel_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_media_channel_set(cps_api_transaction_params_t * param,
                cps_api_object_t obj);

t_std_error dn_pas_media_config_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_media_config_set(cps_api_transaction_params_t * param,
                cps_api_object_t obj);

t_std_error dn_pas_phy_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_phy_set(cps_api_key_t *key, cps_api_object_t obj);

t_std_error dn_pas_power_monitor_get(cps_api_get_params_t * param, size_t key_idx);

t_std_error dn_pas_nvram_get(cps_api_get_params_t *param, size_t key);

t_std_error dn_pas_nvram_set(cps_api_transaction_params_t *param,
                             cps_api_object_t obj);

#endif


