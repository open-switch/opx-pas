/*
 * Copyright (c) 2016 Dell Inc.
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
 * filename: pas_temperature_handler.c
 */ 
     
#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_entity.h"
#include "private/pas_temp_sensor.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"

#include "std_error_codes.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_temperature_get1(
    cps_api_get_params_t     *param,
    cps_api_qualifier_t      qual,
    pas_temperature_sensor_t *rec
                                   )
{
    cps_api_object_t resp_obj;
        
    /* Compose respose object */

    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_temperature_set(resp_obj,
                                   qual,
                                   true, rec->parent->entity_type,
                                   true, rec->parent->slot,
                                   true, rec->name
                                   );

    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_TEMPERATURE_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_TEMPERATURE_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );
    
    cps_api_object_attr_add_u16(resp_obj,
                                BASE_PAS_TEMPERATURE_TEMPERATURE,
                                rec->cur
                                );
    
    cps_api_object_attr_add_u16(resp_obj,
                                BASE_PAS_TEMPERATURE_SHUTDOWN_THRESHOLD,
                                rec->shutdown_threshold
                                );
    
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_TEMPERATURE_THRESH_ENABLE,
                               rec->thresh_en
                               );

    cps_api_object_attr_add_u16(resp_obj,
                                BASE_PAS_TEMPERATURE_LAST_THRESH,
                                rec->last_thresh_crossed->temperature
                               );

    /* Add response object to get response */

    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given temperature sensor instances */

t_std_error dn_pas_temperature_get(cps_api_get_params_t * param, size_t key_idx)
{
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, sensor_name_valid;
    uint_t                   entity_type, slot, i, sensor_idx;
    uint_t                   slot_start, slot_limit;
    char                     sensor_name[PAS_NAME_LEN_MAX];
    struct pas_config_entity *e;
    pas_entity_t             *entity_rec;
    pas_temperature_sensor_t *temp_rec;

    dn_pas_obj_key_temperature_get(req_obj,
                                   &qual,
                                   &entity_type_valid, &entity_type,
                                   &slot_valid, &slot,
                                   &sensor_name_valid,
                                   sensor_name, sizeof(sensor_name)
                                   );
    
    for (i = 0; (e = dn_pas_config_entity_get_idx(i)) != 0; ++i) {
        if (entity_type_valid && e->entity_type != entity_type)  continue;

        if (slot_valid) {
            slot_start = slot_limit = slot;
        } else {
            slot_start = 1;
            slot_limit = e->num_slots;
        }

        for (slot = slot_start; slot <= slot_limit; ++slot) {
            if (sensor_name_valid) {
                if (dn_pas_timedlock() != STD_ERR_OK) {
                    PAS_ERR("Not able to acquire the mutex (timeout)");
                    return (STD_ERR(PAS, FAIL, 0));
                }

                temp_rec = dn_pas_temperature_rec_get_name(e->entity_type, slot, sensor_name);
                if (temp_rec != 0)  dn_pas_temperature_get1(param, qual, temp_rec);

                dn_pas_unlock();
                
                continue;
            }

            entity_rec = dn_pas_entity_rec_get(e->entity_type, slot);
            if (entity_rec == 0)  continue;

            if (dn_pas_timedlock() != STD_ERR_OK) {
                PAS_ERR("Not able to acquire the mutex (timeout)");
                return (STD_ERR(PAS, FAIL, 0));
            }
            for (sensor_idx = 1; sensor_idx <= entity_rec->num_temp_sensors; ++sensor_idx) {
                temp_rec = dn_pas_temperature_rec_get_idx(e->entity_type, slot, sensor_idx);
                if (temp_rec == 0)  continue;

                dn_pas_temperature_get1(param, qual, temp_rec);
            }

            dn_pas_unlock();
        }
    }

    return (STD_ERR_OK);
}

/* Set one temperature sensor */

static t_std_error dn_pas_temperature_set1(
    cps_api_transaction_params_t *param,
    cps_api_qualifier_t          qual,
    pas_temperature_sensor_t     *rec,
    bool                         shutdown_thresh_valid,
    int                          shutdown_thresh,
    bool                         thresh_en_valid,
    bool                         thresh_en
                                   )
{
    cps_api_object_t         old_obj;
    bool                     old_thresh_en;

    /* Add old values, for rollback */

    old_obj = cps_api_object_create();
    if (old_obj == CPS_API_OBJECT_NULL)  return (STD_ERR(PAS, NOMEM, 0));

    dn_pas_obj_key_temperature_set(old_obj,
                                   qual,
                                   true, rec->parent->entity_type,
                                   true, rec->parent->slot,
                                   true, rec->name
                                   );

    cps_api_object_attr_add_u16(old_obj,
                                BASE_PAS_TEMPERATURE_SHUTDOWN_THRESHOLD,
                                (uint16_t) rec->shutdown_threshold
                                );
    
    cps_api_object_attr_add_u8(old_obj,
                               BASE_PAS_TEMPERATURE_THRESH_ENABLE,
                               rec->thresh_en
                               );

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    if (shutdown_thresh_valid) {
        rec->shutdown_threshold = shutdown_thresh;
    }
    
    if (thresh_en_valid) {
        old_thresh_en  = rec->thresh_en;
        rec->thresh_en = thresh_en;
        
        if (rec->thresh_en && !old_thresh_en) {
            rec->last_thresh_crossed->temperature = -9999;
            rec->last_thresh_crossed->dir         = 1;
            if (rec->nsamples >= 2)  rec->nsamples = 1;
            
            dn_temp_sensor_thresh_chk(rec);
            dn_temp_sensor_notify(rec);
        }
    }

    return (STD_ERR_OK);
}

t_std_error dn_pas_temperature_set(cps_api_transaction_params_t *param, cps_api_object_t obj)
{
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, sensor_name_valid;
    uint_t                   entity_type, slot, sensor_idx, i;
    uint_t                   slot_start, slot_limit;
    char                     sensor_name[PAS_NAME_LEN_MAX];
    cps_api_object_attr_t    a;
    bool                     shutdown_thresh_valid;
    int                      shutdown_thresh = 0;
    bool                     thresh_en_valid;
    bool                     thresh_en = false;
    struct pas_config_entity *e;
    pas_entity_t             *entity_rec;
    pas_temperature_sensor_t *temp_rec;

    if (cps_api_object_type_operation(cps_api_object_key(obj)) != cps_api_oper_SET) {
        return cps_api_ret_code_ERR;
    }

    a = cps_api_object_attr_get(obj, BASE_PAS_TEMPERATURE_SHUTDOWN_THRESHOLD);
    if ((shutdown_thresh_valid = (a != CPS_API_ATTR_NULL))) {
        shutdown_thresh = cps_api_object_attr_data_uint(a);
    }

    a = cps_api_object_attr_get(obj, BASE_PAS_TEMPERATURE_THRESH_ENABLE);
    if ((thresh_en_valid = (a != CPS_API_ATTR_NULL))) {
        thresh_en = cps_api_object_attr_data_uint(a);
    }

    dn_pas_obj_key_temperature_get(obj,
                                   &qual,
                                   &entity_type_valid, &entity_type,
                                   &slot_valid, &slot,
                                   &sensor_name_valid, sensor_name, sizeof(sensor_name)
                                   );

    for (i = 0; (e = dn_pas_config_entity_get_idx(i)) != 0; ++i) {
        if (entity_type_valid && e->entity_type != entity_type)  continue;

        if (slot_valid) {
            slot_start = slot_limit = slot;
        } else {
            slot_start = 1;
            slot_limit = e->num_slots;
        }

        for (slot = slot_start; slot <= slot_limit; ++slot) {
            if (sensor_name_valid) {
                if (dn_pas_timedlock() != STD_ERR_OK) {
                    PAS_ERR("Not able to acquire the mutex (timeout)");
                    return (STD_ERR(PAS, FAIL, 0));
                }

                temp_rec = dn_pas_temperature_rec_get_name(e->entity_type,
                                                           slot,
                                                           sensor_name
                                                           );
                if (temp_rec != 0) {
                    dn_pas_temperature_set1(param,
                                            qual,
                                            temp_rec,
                                            shutdown_thresh_valid, shutdown_thresh,
                                            thresh_en_valid, thresh_en
                                            );
                }

                dn_pas_unlock();
                
                continue;
            }

            entity_rec = dn_pas_entity_rec_get(e->entity_type, slot);
            if (entity_rec == 0)  continue;

            if (dn_pas_timedlock() != STD_ERR_OK) {
                PAS_ERR("Not able to acquire the mutex (timeout)");
                return (STD_ERR(PAS, FAIL, 0));
            }
            for (sensor_idx = 1; sensor_idx <= entity_rec->num_temp_sensors; ++sensor_idx) {
                temp_rec = dn_pas_temperature_rec_get_idx(e->entity_type, slot, sensor_idx);
                if (temp_rec == 0)  continue;

                dn_pas_temperature_set1(param,
                                        qual,
                                        temp_rec,
                                        shutdown_thresh_valid, shutdown_thresh,
                                        thresh_en_valid, thresh_en
                                        );
            }

            dn_pas_unlock();
        }
    }

    return (STD_ERR_OK);
}
