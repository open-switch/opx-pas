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
 * filename: pas_fan_handler.c
 */ 
     
#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_entity.h"
#include "private/pas_fan.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"

#include "std_error_codes.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"
#include "sdi_fan.h"

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_fan_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    uint_t               entity_type,
    uint_t               slot,
    uint_t               fan_idx
                                   )
{
    pas_fan_t        *rec;
    cps_api_object_t resp_obj;

    /* Look up object in cache */
    
    char res_key[PAS_RES_KEY_SIZE];

    rec = (pas_fan_t *) dn_pas_res_getc(dn_pas_res_key_fan(res_key,
                                                           sizeof(res_key),
                                                           entity_type,
                                                           slot,
                                                           fan_idx
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

    dn_pas_obj_key_fan_set(resp_obj,
                           qual,
                           true, entity_type,
                           true, slot,
                           true, fan_idx
                           );

    dn_pas_lock();
    
    if ((!rec->valid || qual == cps_api_qualifier_REALTIME) && !dn_pald_diag_mode_get()) {
        /* Cache not valid or realtime object requested
           => Update cache from hardware
        */
        
        dn_entity_poll(rec->parent, true);
    }
    
    if (rec->parent->present) {
        /* Add result attributes to response object */    
        
        if (qual == cps_api_qualifier_TARGET) {
            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_FAN_SPEED,
                                        rec->targ_speed
                                        );
           
            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_FAN_SPEED_PCT,
                                        (rec->max_speed != 0)
                                        ? (100 * rec->targ_speed / rec->max_speed)
                                        : 0
                                        ); 
        } else {
            cps_api_object_attr_add_u8(resp_obj,
                                       BASE_PAS_FAN_OPER_STATUS,
                                       rec->oper_fault_state->oper_status
                                       );
            
            cps_api_object_attr_add_u8(resp_obj,
                                       BASE_PAS_FAN_FAULT_TYPE,
                                       rec->oper_fault_state->fault_type
                                       );
            
            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_FAN_SPEED,
                                        rec->obs_speed
                                        );
            
            cps_api_object_attr_add_u8(resp_obj,
                                       BASE_PAS_FAN_SPEED_PCT,
                                       rec->max_speed != 0
                                       ? (100 * rec->obs_speed) / rec->max_speed
                                       : 0
                                       );
            
            cps_api_object_attr_add_u16(resp_obj,
                                        BASE_PAS_FAN_MAX_SPEED,
                                        rec->max_speed
                                        );
        }
    }
    
    dn_pas_unlock();
    
    /* Add response object to get response */

    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given fan instance */

t_std_error dn_pas_fan_get(cps_api_get_params_t * param, size_t key_idx)
{
    t_std_error              result  = STD_ERR_OK, ret;
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, fan_idx_valid;
    uint_t                   entity_type, i, slot, slot_idx, slot_start, slot_limit;
    uint_t                   fan_idx, idx, fan_idx_start, fan_idx_limit;
    struct pas_config_entity *e;

    dn_pas_obj_key_fan_get(req_obj,
                           &qual,
                           &entity_type_valid, &entity_type,
                           &slot_valid, &slot,
                           &fan_idx_valid, &fan_idx
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
            if (fan_idx_valid) {
                fan_idx_start = fan_idx_limit = fan_idx;
            } else {
                pas_entity_t *entity_rec;

                entity_rec = dn_pas_entity_rec_get(e->entity_type, slot_idx);
                if (entity_rec == 0)  continue;

                fan_idx_start = 1;
                fan_idx_limit = entity_rec->num_fans;
            }

            for (idx = fan_idx_start; idx <= fan_idx_limit; ++idx) {
                ret = dn_pas_fan_get1(param, qual, e->entity_type, slot_idx, idx);
                
                if (ret == STD_ERR(PAS, NEXIST, 0)) {
                    if (entity_type_valid && slot_valid && fan_idx_valid) {
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

/************************************************************************
 *
 * Name: dn_pas_fan_set
 *
 *      This function is to handle pas fan set request
 *
 * Input: cps api key
 *        cps object
 *
 * Return Values: cps_api_return_code_t
 * -------------
 *
 ************************************************************************/

static t_std_error dn_pas_fan_set1(
    cps_api_transaction_params_t *param,
    cps_api_qualifier_t          qual,
    uint_t                       entity_type,
    uint_t                       slot,
    uint_t                       fan_idx,
    bool                         speed_valid,
    uint_t                       speed,
    bool                         speed_pct_valid,
    uint_t                       speed_pct
                                   )
{
    pas_fan_t              *rec;
    cps_api_object_t       old_obj;
    pas_oper_fault_state_t prev_oper_fault_state[1];
    t_std_error            rc = STD_ERR_OK;
    uint_t                 targ_speed = 0;
    bool                   notif = false;

    /* Look up object in cache */
    
    char res_key[PAS_RES_KEY_SIZE];

    rec = (pas_fan_t *) dn_pas_res_getc(dn_pas_res_key_fan(res_key,
                                                           sizeof(res_key),
                                                           entity_type,
                                                           slot,
                                                           fan_idx
                                                           )
                                        );
    if (rec == 0) {
        /* Not found */

        return (STD_ERR(PAS, NEXIST, 0));
    }

    *prev_oper_fault_state = *rec->oper_fault_state;

    old_obj = cps_api_object_create();
    if (old_obj == CPS_API_OBJECT_NULL)  return (STD_ERR(PAS, NOMEM, 0));

    dn_pas_obj_key_fan_set(old_obj,
                           qual,
                           true, entity_type,
                           true, slot,
                           true, fan_idx
                           );

    cps_api_object_attr_add_u16(old_obj,
                                BASE_PAS_FAN_SPEED,
                                rec->targ_speed
                                );

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    targ_speed = rec->targ_speed;

    if (speed_pct_valid) {
        targ_speed = (speed_pct * rec->max_speed) / 100;
    }

    if (speed_valid)  targ_speed = speed;

    if (targ_speed == rec->targ_speed) return rc;
        
    dn_pas_lock();

    if(!dn_pald_diag_mode_get()) {
        
        rc = sdi_fan_speed_set(rec->sdi_resource_hdl, targ_speed);
    }
    
    dn_pas_unlock();

    if (STD_IS_ERR(rc)) {
        dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                       PLATFORM_FAULT_TYPE_EHW
                                       );
    } else {
        rec->targ_speed = targ_speed;
    }
    
    if (rec->oper_fault_state->oper_status != prev_oper_fault_state->oper_status) {
        notif = true;
    }

    if (notif)  dn_fan_notify(rec);

    return (rc);
}

t_std_error dn_pas_fan_set(cps_api_transaction_params_t *param, cps_api_object_t obj)
{
    t_std_error              result  = STD_ERR_OK, ret;
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, fan_idx_valid;
    bool                     speed_valid, speed_pct_valid;
    uint_t                   entity_type, slot, slot_idx, slot_start, slot_limit;
    uint_t                   fan_idx, idx, fan_idx_start, fan_idx_limit;
    uint_t                   speed = 0, speed_pct = 0, i;
    cps_api_object_attr_t    a;
    struct pas_config_entity *e;

    if (cps_api_object_type_operation(cps_api_object_key(obj)) != cps_api_oper_SET) {
        return cps_api_ret_code_ERR;
    }

    speed_valid = ((a = cps_api_object_attr_get(obj, BASE_PAS_FAN_SPEED))
                   != CPS_API_ATTR_NULL
                   );
    if (speed_valid)  speed = cps_api_object_attr_data_uint(a);

    speed_pct_valid = ((a = cps_api_object_attr_get(obj, BASE_PAS_FAN_SPEED_PCT))
                       != CPS_API_ATTR_NULL
                       );
    if (speed_pct_valid)  speed_pct = cps_api_object_attr_data_uint(a);

    if (!speed_valid && !speed_pct_valid)  return (result);

    dn_pas_obj_key_fan_get(obj,
                           &qual,
                           &entity_type_valid, &entity_type,
                           &slot_valid, &slot,
                           &fan_idx_valid, &fan_idx
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
            if (fan_idx_valid) {
                fan_idx_start = fan_idx_limit = fan_idx;
            } else {
                pas_entity_t *entity_rec;

                entity_rec = dn_pas_entity_rec_get(e->entity_type, slot_idx);
                if (entity_rec == 0)  continue;
                fan_idx_start = 1;
                fan_idx_limit = entity_rec->num_fans;
            }

            for (idx = fan_idx_start; idx <= fan_idx_limit; ++idx) {
                ret = dn_pas_fan_set1(param,
                                      qual,
                                      e->entity_type,
                                      slot_idx,
                                      idx, 
                                      speed_valid,
                                      speed,
                                      speed_pct_valid,
                                      speed_pct
                                      );
                
                if (ret == STD_ERR(PAS, NEXIST, 0)) {
                    if (entity_type_valid && slot_valid && fan_idx_valid) {
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
