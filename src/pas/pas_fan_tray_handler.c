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
 * filename: pas_fan_tray_handler.c
 */ 
     
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_main.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_entity.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"

#include "std_error_codes.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_fan_tray_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    uint_t               slot
                                   )
{
    pas_fan_tray_t   *rec;
    cps_api_object_t resp_obj;

    /* Look up object in cache */
    
    char res_key[PAS_RES_KEY_SIZE];

    rec = (pas_fan_tray_t *) dn_pas_res_getc(dn_pas_res_key_fan_tray(res_key,
                                                                    sizeof(res_key),
                                                                    slot
                                                                    )
                                             );
    if (rec == 0) {
        /* Not found */

        return (STD_ERR(PAS, NEXIST, 0));
    }
        
    /* Compose respose object */

    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_fan_tray_set(resp_obj, qual, true, slot);

    if (dn_pas_timedlock() != STD_ERR_OK) {
        PAS_ERR("Not able to acquire the mutex (timeout)");
        return (STD_ERR(PAS, FAIL, 0));
    }
    if ((!rec->valid || qual == cps_api_qualifier_REALTIME) && !dn_pald_diag_mode_get()) {
        /* Cache not valid or realtime object requested
           => Update cache from hardware
        */
        
        dn_entity_poll(rec->parent, true);
    }
    
    if (rec->parent->present) {
        /* Add result attributes to response object */    
        if ((rec->fan_airflow_type != PLATFORM_FAN_AIRFLOW_TYPE_NORMAL) &&
            (rec->fan_airflow_type != PLATFORM_FAN_AIRFLOW_TYPE_REVERSE) &&
            (rec->fan_airflow_type != PLATFORM_FAN_AIRFLOW_TYPE_NOT_APPLICABLE)) {

            rec->fan_airflow_type = PLATFORM_FAN_AIRFLOW_TYPE_UNKNOWN;
        }

        cps_api_object_attr_add_u8(resp_obj,
                                   BASE_PAS_FAN_TRAY_FAN_AIRFLOW_TYPE,
                                   rec->fan_airflow_type
                                   );
    }
    
    dn_pas_unlock();
    
    /* Add response object to get response */

    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given fan tray instance */

t_std_error dn_pas_fan_tray_get(cps_api_get_params_t * param, size_t key_idx)
{
    t_std_error         result  = STD_ERR_OK, ret;
    cps_api_object_t    req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t qual;
    bool                slot_valid;
    uint_t              slot, slot_start, slot_limit;

    dn_pas_obj_key_fan_tray_get(req_obj, &qual, &slot_valid, &slot);

    if (slot_valid) {
        slot_start = slot_limit = slot;
    } else {
        struct pas_config_entity *e;
        
        e = dn_pas_config_entity_get_type(PLATFORM_ENTITY_TYPE_FAN_TRAY);
        if (e == 0)  return (STD_ERR(PAS, FAIL, 0));
        
        slot_start = 1;
        slot_limit = e->num_slots;
    }

    for (slot = slot_start; slot <= slot_limit; ++slot) {
        ret = dn_pas_fan_tray_get1(param, qual, slot);
        
        if (ret == STD_ERR(PAS, NEXIST, 0)) {
            if (slot_valid) {
                result = STD_ERR(PAS, NEXIST, 0);
            }
            
            break;
        } else if (STD_IS_ERR(ret)) {
            result = STD_ERR(PAS, FAIL, 0);
        }
    }

    return (result);
}

t_std_error dn_pas_fan_tray_set(cps_api_key_t *key, cps_api_object_t obj)
{
   return STD_ERR_OK;
}
