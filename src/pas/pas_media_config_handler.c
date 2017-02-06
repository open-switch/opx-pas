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

/**************************************************************************
 * @file pas_media_config_handler.c
 *
 * @brief This file contains source code of physical media handling.
 **************************************************************************/

#include "private/pas_media.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_main.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"
#include "cps_api_operation.h"
#include "cps_api_service.h"
#include "cps_api_events.h"
#include <stdio.h>
#include <stdlib.h>

t_std_error dn_pas_media_config_get(cps_api_get_params_t * param, 
        size_t key_ix)
{
    cps_api_object_t        obj =  CPS_API_OBJECT_NULL;
    cps_api_qualifier_t     qualifier;
    uint32_t                slot, node_id;
    bool                    slot_valid, node_id_valid;
    cps_api_object_t        req_obj;
    
    
    req_obj = cps_api_object_list_get(param->filters, key_ix);

    STD_ASSERT(req_obj != NULL);

    dn_pas_obj_key_media_config_get(req_obj, &qualifier, &node_id_valid,
            &node_id, &slot_valid, &slot);


    if (slot_valid == false) {
        dn_pas_slot(&slot);
    }


    if((obj = cps_api_object_create()) == NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_media_config_set(obj, qualifier, true, node_id, true, slot);

    struct pas_config_media *cfg = dn_pas_config_media_get();

    if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CONFIG_LOCK_DOWN_STATUS,
                &cfg->lockdown, sizeof(cfg->lockdown)) == false) {
        cps_api_object_delete(obj);

        return STD_ERR(PAS, FAIL, 0);
    }

    if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CONFIG_LED_CONTROL,
                &cfg->led_control, sizeof(cfg->led_control)) == false) {
        cps_api_object_delete(obj);

        return STD_ERR(PAS, FAIL, 0);
    }

    if (!cps_api_object_list_append(param->list, obj)) {

        cps_api_object_delete(obj);

        PAS_ERR("Failed to append response object");

        return STD_ERR(PAS, FAIL, 0);
    }

    return STD_ERR_OK;
}

t_std_error dn_pas_media_config_set(cps_api_transaction_params_t * param,
        cps_api_object_t obj)
{
    uint32_t                node_id, slot;
    bool                    slot_valid, node_id_valid;
    t_std_error             ret = STD_ERR_OK;
    cps_api_object_it_t     it;
    cps_api_attr_id_t       attr_id;
    cps_api_qualifier_t     qualifier;
    cps_api_object_t        cloned;

    dn_pas_lock();

    if(dn_pald_diag_mode_get()) {

        dn_pas_unlock();
        return STD_ERR(PAS, FAIL, 0);
    }

    if ((param == NULL) || (obj == NULL)) {
        PAS_ERR("Invalid parameter");

        dn_pas_unlock();
        return STD_ERR(PAS, FAIL, 0);
    }

    dn_pas_obj_key_media_config_get(obj, &qualifier, &node_id_valid, &node_id,
            &slot_valid, &slot);


    if (slot_valid == false) {
        dn_pas_slot(&slot);
    }


    if ((cloned = cps_api_object_create()) == NULL) {
        PAS_ERR("Failed to create CPS API object");

        dn_pas_unlock();
        return STD_ERR(PAS, NOMEM, 0);
    }

    dn_pas_obj_key_media_config_set(cloned, qualifier, true, node_id, true, slot);

    struct pas_config_media *cfg = dn_pas_config_media_get();

    if (cps_api_object_attr_add(cloned, BASE_PAS_MEDIA_CONFIG_LOCK_DOWN_STATUS,
                &cfg->lockdown, sizeof(cfg->lockdown)) == false) {

        cps_api_object_delete(cloned);
        dn_pas_unlock();
        return STD_ERR(PAS, FAIL, 0);
    }

    if (cps_api_object_list_append(param->prev, cloned) == false) {
        PAS_ERR("Failed to append response object");

        cps_api_object_delete(cloned);
        dn_pas_unlock();
        return STD_ERR(PAS, FAIL, 0);
    }

    cps_api_object_it_begin(obj, &it);

    while (cps_api_object_it_valid (&it)) {

        attr_id = cps_api_object_attr_id (it.attr);

        switch (attr_id) {
            case BASE_PAS_MEDIA_CONFIG_LOCK_DOWN_STATUS:
                {
                    bool status;

                    memcpy(&status, cps_api_object_attr_data_bin(it.attr),
                            sizeof(status));

                    if (dn_pas_media_lockdown_handle(status) == false) {
                        PAS_ERR("Media lockdown failed");

                        ret = STD_ERR(PAS, FAIL, 0);
                    }

                }
                break;
            default:
                {
                    PAS_WARN("Invalid attribute (%u)", attr_id);
                }
                break;
        }
        cps_api_object_it_next (&it);
    }
    dn_pas_unlock();
    return ret;
}
