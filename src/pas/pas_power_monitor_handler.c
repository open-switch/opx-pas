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
 * filename: pas_power_monitor_handler.c
 */ 
     
#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
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

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_pm_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    uint_t               entity_type,
    uint_t               slot,
    uint_t               pm_idx
                                 )
{
    pas_power_monitor_t    *rec;
    cps_api_object_t       resp_obj;

    /* Look up object in cache */
    
    char res_key[PAS_RES_KEY_SIZE];

    rec = (pas_power_monitor_t *) dn_pas_res_getc(dn_pas_res_key_pm(res_key,
                                                          sizeof(res_key),
                                                          entity_type,
                                                          slot,
                                                          pm_idx));
    if (rec == 0) {
        /* Not found */

        return (STD_ERR(PAS, NEXIST, 0));
    }
        
    /* Compose response object */

    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_pm_set(resp_obj,
                           qual,
                           true, entity_type,
                           true, slot,
                           true, pm_idx
                           );
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
        
            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_POWER_MONITOR_VOLTAGE,
                                        rec->obs_pm_voltage_volt
                                        );
            
            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_POWER_MONITOR_CURRENT,
                                        rec->obs_pm_current_amp
                                        );

            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_POWER_MONITOR_POWER,
                                        rec->obs_pm_power_watt
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

/* Append get response objects to given get params for given power monitor instance */

t_std_error dn_pas_power_monitor_get(cps_api_get_params_t * param, size_t key_idx)
{
    t_std_error              result  = STD_ERR_OK, ret;
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, pm_idx_valid;
    uint_t                   entity_type, i, slot, slot_idx, slot_start, slot_limit;
    uint_t                   pm_idx, idx, pm_idx_start, pm_idx_limit;
    struct pas_config_entity *e;

    dn_pas_obj_key_pm_get(req_obj,
                           &qual,
                           &entity_type_valid, &entity_type,
                           &slot_valid, &slot,
                           &pm_idx_valid, &pm_idx
                           );

    for (i = 0; ; ++i) {
        e = dn_pas_config_entity_get_idx(i);
        if (e == 0)  break;

        if (entity_type_valid && e->entity_type != entity_type)  continue;

        if (slot_valid) {
            slot_start = slot_limit = slot;
        } else {
            slot_start = 1;
            slot_limit = e->num_slots;
        }

        for (slot_idx = slot_start; slot_idx <= slot_limit; ++slot_idx) {
            if (pm_idx_valid) {
                pm_idx_start = pm_idx_limit = pm_idx;
            } else {
                pas_entity_t *entity_rec;

                entity_rec = dn_pas_entity_rec_get(e->entity_type, slot_idx);
                if (entity_rec == 0)  continue;

                pm_idx_start = 1;
                pm_idx_limit = entity_rec->num_power_monitors; 
            }

            for (idx = pm_idx_start; idx <= pm_idx_limit; ++idx) {
                ret = dn_pas_pm_get1(param, qual, e->entity_type, slot_idx, idx);
                
                if (ret == STD_ERR(PAS, NEXIST, 0)) {
                    if (entity_type_valid && slot_valid && pm_idx_valid) {
                        result = STD_ERR(PAS, NEXIST, 0);
                    }
                    
                    break;
                } else if (STD_IS_ERR(ret)) {
                    result = STD_ERR(PAS, FAIL, 0);
                }
            }
        }
    }

    return (result);
}


