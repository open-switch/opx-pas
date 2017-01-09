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
 * filename: pas_temp_threshold_handler.c
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

#include <stdlib.h>

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_temp_thresh_get1(
    cps_api_get_params_t     *param,
    cps_api_qualifier_t      qual,
    pas_temperature_sensor_t *rec,
    uint_t                   thresh_idx
                                           )
{
    cps_api_object_t resp_obj;

    if (thresh_idx < 1 || thresh_idx > rec->num_thresh) {
        return (STD_ERR(PAS, NEXIST, 0));
    }

    /* Compose respose object */

    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_temp_thresh_set(resp_obj,
                                   qual,
                                   true, rec->parent->entity_type,
                                   true, rec->parent->slot,
                                   true, rec->name,
                                   true, thresh_idx
                                   );

    --thresh_idx;

    if (rec->thresholds[thresh_idx].valid) {
        cps_api_object_attr_add_u16(resp_obj,
                                    BASE_PAS_TEMP_THRESHOLD_HI,
                                    rec->thresholds[thresh_idx].hi
                                    );
        
        cps_api_object_attr_add_u16(resp_obj,
                                    BASE_PAS_TEMP_THRESHOLD_LO,
                                    rec->thresholds[thresh_idx].lo
                                    );
    }
    
    /* Add response object to get response */

    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given temperature sensor instances */

t_std_error dn_pas_temp_threshold_get(cps_api_get_params_t * param, size_t key_idx)
{
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, sensor_name_valid, thresh_idx_valid;
    uint_t                   entity_type, slot, thresh_idx, i, sensor_idx;
    uint_t                   slot_start, slot_limit, sensor_idx_start, sensor_idx_limit;
    uint_t                   thresh_idx_start, thresh_idx_limit;
    char                     sensor_name[PAS_NAME_LEN_MAX];
    struct pas_config_entity *e;
    pas_entity_t             *entity_rec;
    pas_temperature_sensor_t *temp_rec;

    dn_pas_obj_key_temp_thresh_get(req_obj,
                                   &qual,
                                   &entity_type_valid, &entity_type,
                                   &slot_valid, &slot,
                                   &sensor_name_valid,
                                   sensor_name, sizeof(sensor_name),
                                   &thresh_idx_valid, &thresh_idx
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
                temp_rec = dn_pas_temperature_rec_get_name(e->entity_type, slot, sensor_name);
                if (temp_rec == 0)  continue;

                sensor_idx_start = sensor_idx_limit = temp_rec->sensor_idx;
            } else {
                entity_rec = dn_pas_entity_rec_get(e->entity_type, slot);
                if (entity_rec == 0)  continue;
                
                sensor_idx_start = 1;
                sensor_idx_limit = entity_rec->num_temp_sensors;
            }

            for (sensor_idx = sensor_idx_start; sensor_idx <= sensor_idx_limit; ++sensor_idx) {
                temp_rec = dn_pas_temperature_rec_get_idx(e->entity_type, slot, sensor_idx);
                if (temp_rec == 0)  continue;
                
                if (thresh_idx_valid) {
                    thresh_idx_start = thresh_idx_limit = thresh_idx;
                } else {
                    thresh_idx_start = 1;
                    thresh_idx_limit = temp_rec->num_thresh;
                }

                for (thresh_idx = thresh_idx_start; thresh_idx <= thresh_idx_limit; ++thresh_idx) {
                    dn_pas_temp_thresh_get1(param, qual, temp_rec, thresh_idx);
                }
            }
        }
    }

    return (STD_ERR_OK);
}

/* Delete one threshold */

static void  dn_pas_temp_thresh_del1(
    pas_temperature_sensor_t *rec,
    uint_t                   thresh_idx
                                     )
{
    if (thresh_idx < 1 || thresh_idx > rec->num_thresh)  return;

    rec->thresholds[thresh_idx - 1].valid = false;
}

/* Set one threshold */

static t_std_error dn_pas_temp_thresh_set1(
    cps_api_transaction_params_t *param,
    cps_api_qualifier_t          qual,
    pas_temperature_sensor_t     *rec,
    uint_t                       thresh_idx,
    int                          hi,
    int                          lo
                                   )
{
    cps_api_object_t old_obj;

    if (thresh_idx < 1 || thresh_idx > rec->num_thresh) {
        return (STD_ERR(PAS, NEXIST, 0));
    }

    /* Append old value, for rollback */

    old_obj = cps_api_object_create();
    if (old_obj == CPS_API_OBJECT_NULL)  return (STD_ERR(PAS, NOMEM, 0));

    dn_pas_obj_key_temp_thresh_set(old_obj,
                                   qual,
                                   true, rec->parent->entity_type,
                                   true, rec->parent->slot,
                                   true, rec->name,
                                   true, thresh_idx
                                   );

    --thresh_idx;

    if (rec->thresholds[thresh_idx].valid) {
        cps_api_object_attr_add_u16(old_obj,
                                    BASE_PAS_TEMP_THRESHOLD_HI,
                                    rec->thresholds[thresh_idx].hi
                                    );
        cps_api_object_attr_add_u16(old_obj,
                                    BASE_PAS_TEMP_THRESHOLD_LO,
                                    rec->thresholds[thresh_idx].lo
                                    );
    }

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    /* Set new value */

    rec->thresholds[thresh_idx].hi    = hi;
    rec->thresholds[thresh_idx].lo    = lo;
    rec->thresholds[thresh_idx].valid = true;

    return (STD_ERR_OK);
}

/* Set thresholds */

t_std_error dn_pas_temp_threshold_set(
    cps_api_transaction_params_t *param,
    cps_api_object_t             obj
                                      )
{
    cps_api_operation_types_t oper;
    cps_api_qualifier_t       qual;
    bool                      entity_type_valid, slot_valid;
    bool                      sensor_name_valid, thresh_idx_valid;
    uint_t                    i, entity_type, slot, sensor_idx, thresh_idx;
    char                      sensor_name[PAS_NAME_LEN_MAX];
    uint_t                    slot_start, slot_limit, sensor_idx_start, sensor_idx_limit;
    uint_t                    thresh_idx_start, thresh_idx_limit;
    struct pas_config_entity  *e;
    pas_entity_t              *entity_rec;
    pas_temperature_sensor_t  *temp_rec;
    cps_api_object_attr_t     a;
    int                       hi = 0, lo = 0;

    oper = cps_api_object_type_operation(cps_api_object_key(obj));
    switch (oper) {
    case cps_api_oper_SET:
        a = cps_api_object_attr_get(obj, BASE_PAS_TEMP_THRESHOLD_HI);
        if (a == CPS_API_ATTR_NULL) {
            return (STD_ERR(PAS, FAIL, 0));
        }
        hi = cps_api_object_attr_data_int(a);
        
        a = cps_api_object_attr_get(obj, BASE_PAS_TEMP_THRESHOLD_LO);
        if (a == CPS_API_ATTR_NULL) {
            return (STD_ERR(PAS, FAIL, 0));
        }
        lo = cps_api_object_attr_data_int(a);

        break;
        
    case cps_api_oper_DELETE:
        break;
        
    default:
        return cps_api_ret_code_ERR;
    }

    dn_pas_obj_key_temp_thresh_get(obj,
                                   &qual,
                                   &entity_type_valid, &entity_type,
                                   &slot_valid, &slot,
                                   &sensor_name_valid,
                                   sensor_name, sizeof(sensor_name),
                                   &thresh_idx_valid, &thresh_idx
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
                temp_rec = dn_pas_temperature_rec_get_name(e->entity_type, slot, sensor_name);
                if (temp_rec == 0)  continue;

                sensor_idx_start = sensor_idx_limit = temp_rec->sensor_idx;
            } else {
                entity_rec = dn_pas_entity_rec_get(e->entity_type, slot);
                if (entity_rec == 0)  continue;
                
                sensor_idx_start = 1;
                sensor_idx_limit = entity_rec->num_temp_sensors;
            }

            for (sensor_idx = sensor_idx_start; sensor_idx <= sensor_idx_limit; ++sensor_idx) {
                temp_rec = dn_pas_temperature_rec_get_idx(e->entity_type, slot, sensor_idx);
                if (temp_rec == 0)  continue;
                
                if (thresh_idx_valid) {
                    thresh_idx_start = thresh_idx_limit = thresh_idx;
                } else {
                    thresh_idx_start = 1;
                    thresh_idx_limit = temp_rec->num_thresh;
                }

                for (thresh_idx = thresh_idx_start; thresh_idx <= thresh_idx_limit; ++thresh_idx) {
                    switch (oper) {
                    case cps_api_oper_SET:
                        dn_pas_temp_thresh_set1(param, qual, temp_rec, thresh_idx, hi, lo);
                        break;

                    case cps_api_oper_DELETE:
                        dn_pas_temp_thresh_del1(temp_rec, thresh_idx);
                        break;

                    default:
                        ;
                    }
                }
            }
        }
    }

    return (STD_ERR_OK);
}
