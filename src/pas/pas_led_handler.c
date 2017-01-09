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
 * filename: pas_led_handler.c
 */ 
     
#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_entity.h"
#include "private/pas_led.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"

#include "std_error_codes.h"
#include "sdi_led.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_led_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    pas_led_t            *rec
                                   )
{
    cps_api_object_t resp_obj;
        
    /* Compose respose object */

    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_led_set(resp_obj,
                           qual,
                           true, rec->parent->entity_type,
                           true, rec->parent->slot,
                           true, rec->name
                           );

    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_LED_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_LED_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );
    
    cps_api_object_attr_add_u8(resp_obj,
                               BASE_PAS_LED_ON,
                               rec->req_on != 0
                               );
    
    /* Add response object to get response */

    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given LED instances */

t_std_error dn_pas_led_get(cps_api_get_params_t * param, size_t key_idx)
{
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, led_name_valid;
    uint_t                   entity_type, slot, i, led_idx;
    uint_t                   slot_start, slot_limit;
    char                     led_name[PAS_NAME_LEN_MAX];
    struct pas_config_entity *e;
    pas_entity_t             *entity_rec;
    pas_led_t                *led_rec;

    dn_pas_obj_key_led_get(req_obj,
                           &qual,
                           &entity_type_valid, &entity_type,
                           &slot_valid, &slot,
                           &led_name_valid,
                           led_name, sizeof(led_name)
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
            if (led_name_valid) {
                led_rec = dn_pas_led_rec_get_name(e->entity_type, slot, led_name);
                if (led_rec == 0)  continue;

                dn_pas_led_get1(param, qual, led_rec);
                
                continue;
            }

            entity_rec = dn_pas_entity_rec_get(e->entity_type, slot);
            if (entity_rec == 0)  continue;

            for (led_idx = 1; led_idx <= entity_rec->num_leds; ++led_idx) {
                led_rec = dn_pas_led_rec_get_idx(e->entity_type, slot, led_idx);
                if (led_rec == 0)  continue;

                dn_pas_led_get1(param, qual, led_rec);
            }
        }
    }

    return (STD_ERR_OK);
}

/* Set one LED */

static t_std_error dn_pas_led_set1(
    cps_api_transaction_params_t *param,
    cps_api_qualifier_t          qual,
    pas_led_t                    *rec,
    bool                         on_valid,
    bool                         on
                                   )
{
    cps_api_object_t old_obj;
    void             *iter;
    bool             on_foundf;

    /* Add old values, for rollback */

    old_obj = cps_api_object_create();
    if (old_obj == CPS_API_OBJECT_NULL)  return (STD_ERR(PAS, NOMEM, 0));

    dn_pas_obj_key_led_set(old_obj,
                           qual,
                           true, rec->parent->entity_type,
                           true, rec->parent->slot,
                           true, rec->name
                           );

    cps_api_object_attr_add_u8(old_obj,
                               BASE_PAS_LED_ON,
                               rec->req_on != 0
                               );

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    if (!on_valid)  return (STD_ERR_OK);

    rec->req_on = on;

    /* Set LEDs in group, highest to lowest priority */

    on_foundf = false;
    for (iter = dn_pas_config_led_group_iter_first(rec->parent->entity_type,
                                                   rec->name
                                                   );
         iter != 0;
         iter = dn_pas_config_led_group_iter_next(iter)
         ) {
        pas_led_t *grec;
        char      *gname;
        bool      sdi_on;
        
        gname = dn_pas_config_led_group_iter_name(iter);

        grec = dn_pas_led_rec_get_name(rec->parent->entity_type,
                                       rec->parent->slot,
                                       gname
                                       );
        if (grec == 0)  continue;

        /* If higher pri LED on, this SDI LED is off, else is requested state */

        sdi_on = on_foundf ? false : grec->req_on;

        if (!grec->sdi_on_valid || grec->sdi_on != sdi_on) {
            /* New SDI state != current SDI state */

            grec->sdi_on       = sdi_on; /* Update SDI state */
            grec->sdi_on_valid = true;

            if (!on_foundf) {
                /* Haven't higher pri LED on => Set state in SDI */
                dn_pas_lock();
    
                if(!dn_pald_diag_mode_get()) {
        
                    (*(grec->sdi_on ? sdi_led_on : sdi_led_off))(grec->sdi_resource_hdl);
                }
                dn_pas_unlock();
            }
        }

        /* Found an LED request to be on */
        if (grec->req_on)  on_foundf = true;
    }

    return (STD_ERR_OK);
}

t_std_error dn_pas_led_set(cps_api_transaction_params_t *param, cps_api_object_t obj)
{
    cps_api_qualifier_t      qual;
    bool                     entity_type_valid, slot_valid, led_name_valid;
    uint_t                   entity_type, slot, led_idx, i;
    uint_t                   slot_start, slot_limit;
    char                     led_name[PAS_NAME_LEN_MAX];
    cps_api_object_attr_t    a;
    bool                     on_valid;
    bool                     on;
    struct pas_config_entity *e;
    pas_entity_t             *entity_rec;
    pas_led_t                *led_rec;

    if (cps_api_object_type_operation(cps_api_object_key(obj)) != cps_api_oper_SET) {
        return cps_api_ret_code_ERR;
    }

    a = cps_api_object_attr_get(obj, BASE_PAS_LED_ON);
    if ((on_valid = (a != CPS_API_ATTR_NULL))) {
        on = (cps_api_object_attr_data_uint(a) != 0);
    } else {
        return cps_api_ret_code_ERR;
    }

    dn_pas_obj_key_led_get(obj,
                           &qual,
                           &entity_type_valid, &entity_type,
                           &slot_valid, &slot,
                           &led_name_valid,
                           led_name, sizeof(led_name)
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
            if (led_name_valid) {
                led_rec = dn_pas_led_rec_get_name(e->entity_type,
                                                  slot,
                                                  led_name
                                                  );
                if (led_rec == 0)  continue;

                dn_pas_led_set1(param, qual, led_rec, on_valid, on);
                
                continue;
            }

            entity_rec = dn_pas_entity_rec_get(e->entity_type, slot);
            if (entity_rec == 0)  continue;

            for (led_idx = 1; led_idx <= entity_rec->num_leds; ++led_idx) {
                led_rec = dn_pas_led_rec_get_idx(e->entity_type, slot, led_idx);
                if (led_rec == 0)  continue;

                dn_pas_led_set1(param, qual, led_rec, on_valid, on);
            }
        }
    }

    return (STD_ERR_OK);
}
