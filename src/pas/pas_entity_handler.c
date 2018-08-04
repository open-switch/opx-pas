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
 * filename: pas_entity_handler.c
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

#include <stdlib.h>

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_entity_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    uint_t               entity_type,
    uint_t               slot
                                      )
{
    pas_entity_t        *rec;
    cps_api_object_t    resp_obj;

    /* Look up object in cache */
    
    char res_key[PAS_RES_KEY_SIZE];

    rec = (pas_entity_t *) dn_pas_res_getc(dn_pas_res_key_entity(res_key,
                                                                 sizeof(res_key),
                                                                 entity_type,
                                                                 slot)
                                           );
    if (rec == 0) {
        /* Not found */

        return (STD_ERR(PAS, NEXIST, 0));
    }
        
    /* Compose respose object */

    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to create CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_entity_set(resp_obj, qual, true, entity_type, true, slot);

    if (dn_pas_timedlock() != STD_ERR_OK) {
        PAS_ERR("Not able to acquire the mutex (timeout)");
        return (STD_ERR(PAS, FAIL, 0));
    }

    if ((!rec->valid || qual == cps_api_qualifier_REALTIME) && !dn_pald_diag_mode_get()) {
        /* Cache not valid or realtime object requested
           => Update cache from hardware
        */
        
        dn_entity_poll(rec, true);
    }
    
    /* Add result attributes to response object */
    
    cps_api_object_attr_add_str(resp_obj,
                                BASE_PAS_ENTITY_NAME,
                                rec->name
                                );
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_ENTITY_PRESENT,
                               rec->present
                               );
    cps_api_object_attr_add_u64(resp_obj,
                                BASE_PAS_ENTITY_INSERTION_CNT,
                                rec->insertion_cnt
                                );
    cps_api_object_attr_add_u64(resp_obj,
                                BASE_PAS_ENTITY_INSERTION_TIMESTAMP,
                                rec->insertion_timestamp
                                );
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_ENTITY_ADMIN_STATUS,
                               rec->admin_status
                               );
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_ENTITY_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_ENTITY_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );

    if (rec->eeprom_valid) {
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_VENDOR_NAME,
                                    rec->eeprom->vendor_name
                                    );
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_PRODUCT_NAME,
                                    rec->eeprom->product_name
                                    );
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_HW_VERSION,
                                    rec->eeprom->hw_version
                                    );
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_PLATFORM_NAME,
                                    rec->eeprom->platform_name
                                    );
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_PPID,
                                    rec->eeprom->ppid
                                    );
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_PART_NUMBER,
                                    rec->eeprom->part_number
                                    );
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_SERVICE_TAG,
                                    rec->eeprom->service_tag
                                    );
        char buf[15];
        dn_pas_service_tag_to_express_service_code(buf, sizeof(buf), rec->eeprom->service_tag);
        cps_api_object_attr_add_str(resp_obj,
                                    BASE_PAS_ENTITY_SERVICE_CODE,
                                    buf
                                    );
    }

    if (entity_type == PLATFORM_ENTITY_TYPE_CARD) {
        cps_api_object_attr_add_u8(
            resp_obj,
            BASE_PAS_ENTITY_ENTITY_TYPE_CHOICE_CARD_POWER_ON,
            rec->power_on
                                   );

        cps_api_object_attr_add_u8(
            resp_obj,
            BASE_PAS_ENTITY_ENTITY_TYPE_CHOICE_CARD_REBOOT,
            rec->reboot_type
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

/* Append get response objects to given get params for given entity instance */

t_std_error dn_pas_entity_get(cps_api_get_params_t * param, size_t key_idx)
{
    t_std_error              result  = STD_ERR_OK, ret;
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid;
    uint_t                   entity_type, slot, slot_start, slot_limit, i;
    struct pas_config_entity *e;

    dn_pas_obj_key_entity_get(req_obj,
                              &qual,
                              &entity_type_valid,
                              &entity_type,
                              &slot_valid,
                              &slot
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

        for (slot = slot_start; slot <= slot_limit; ++slot) {
            ret = dn_pas_entity_get1(param, qual, e->entity_type, slot);

            if (ret == STD_ERR(PAS, NEXIST, 0)) {
                if (entity_type_valid && slot_valid) {
                    result = STD_ERR(PAS, NEXIST, 0);
                }

                break;
            } else if (STD_IS_ERR(ret)) {
                result = STD_ERR(PAS, FAIL, 0);
            }
        }
    }

    return (result);
}

static t_std_error dn_pas_entity_set1( 
                     cps_api_transaction_params_t *param,
                     cps_api_qualifier_t           qual,
                     uint_t                        entity_type,
                     uint_t                        slot,
                     uint8_t                       reboot_type)
{

    cps_api_object_t old_obj;
    pas_entity_t     *rec;
    char             res_key[PAS_RES_KEY_SIZE];

    rec = (pas_entity_t *) dn_pas_res_getc(dn_pas_res_key_entity(res_key,
                                                                 sizeof(res_key),
                                                                 entity_type,
                                                                 slot
                                                                 )
                                           );
    if (rec == 0) {
        /* Not found */

        return (STD_ERR(PAS, NEXIST, 0));
    }

    /* Add old values, for rollback */

    old_obj = cps_api_object_create();

    if (old_obj == CPS_API_OBJECT_NULL) {
        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_entity_set(old_obj, qual, true, entity_type, true, slot);

    cps_api_object_attr_add_u8(old_obj,
                               BASE_PAS_ENTITY_ENTITY_TYPE_CHOICE_CARD_REBOOT, 
                               rec->reboot_type
                               );

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    rec->reboot_type = reboot_type;

    switch (reboot_type) {
        case PLATFORM_REBOOT_TYPE_COLD:  
                /* Resets all components of the entity except the Data-plane */

            if (STD_IS_ERR(sdi_entity_reset(rec->sdi_entity_hdl, 
                                            COLD_RESET))) {

                  return (STD_ERR(PAS, FAIL, 0));
            }
            break;

        case PLATFORM_REBOOT_TYPE_WARM:   
                /* Resets all the components of the entity which includes Control
                   plane and Data Plane */

            if (STD_IS_ERR(sdi_entity_reset(rec->sdi_entity_hdl, 
                                            WARM_RESET))) {

                  return (STD_ERR(PAS, FAIL, 0));
            }
            break;

        default: 
                break;
    }

    return (STD_ERR_OK);
}

/************************************************************************
 *
 * Name: dn_pas_entity_set
 *
 *      This function is to handle pas entity set request
 *
 * Input: cps_api_transaction_params_t
 *        cps object
 *
 * Return Values: error code or STD_ERR_OK 
 * -------------
 *
 ************************************************************************/

t_std_error dn_pas_entity_set(cps_api_transaction_params_t *param, 
                              cps_api_object_t             obj)
{
    t_std_error              ret = STD_ERR_OK;
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid;
    uint_t                   entity_type, slot, slot_start, slot_limit, i;
    uint8_t                  reboot_type;
    cps_api_object_attr_t    a;
    struct pas_config_entity *e;

    if (cps_api_object_type_operation(cps_api_object_key(obj)) != cps_api_oper_SET) {
        return cps_api_ret_code_ERR;
    }

    dn_pas_obj_key_entity_get(obj,
                              &qual,
                              &entity_type_valid,
                              &entity_type,
                              &slot_valid,
                              &slot
                              );

    /* set reboot type only on Card entity */ 
    if (entity_type        != PLATFORM_ENTITY_TYPE_CARD || 
        entity_type_valid  == false) {

           return STD_ERR(PAS, FAIL, 0);
    }

    a = cps_api_object_attr_get(obj, 
                                BASE_PAS_ENTITY_ENTITY_TYPE_CHOICE_CARD_REBOOT);

    if (a == CPS_API_ATTR_NULL) {
        return (STD_ERR(PAS, FAIL, 0));
    }

    reboot_type = cps_api_object_attr_data_u8(a);

    if (reboot_type != PLATFORM_REBOOT_TYPE_COLD &&
        reboot_type != PLATFORM_REBOOT_TYPE_WARM ) {
        PAS_WARN("Invalid reboot type (%u)", reboot_type);

        return (STD_ERR(PAS, FAIL, 0));
    }

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

        for (slot = slot_start; slot <= slot_limit; ++slot) {
            ret = dn_pas_entity_set1(param, 
                                     qual, 
                                     entity_type, 
                                     slot, 
                                     reboot_type);

        }
    }
    return ret;
}
