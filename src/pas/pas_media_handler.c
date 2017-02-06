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
 * @file pas_media_handler.c
 *
 * @brief This file contains source code of physical media handling.
 **************************************************************************/

#include "private/pas_media.h"
#include "private/pas_log.h"
#include "private/pald.h"
#include "private/pas_main.h"
#include "private/pas_utils.h"
#include "private/dn_pas.h"
#include "cps_api_operation.h"
#include "cps_api_service.h"
#include "cps_api_events.h"
#include <stdio.h>
#include <stdlib.h>

t_std_error dn_pas_media_get(cps_api_get_params_t * param,
        size_t key_ix)
{
    cps_api_object_t        obj =  CPS_API_OBJECT_NULL;
    cps_api_qualifier_t     qualifier;
    phy_media_tbl_t         *mtbl = NULL;
    uint32_t                slot, port, port_module;
    bool                    slot_valid, port_module_valid, port_valid;
    uint32_t                start, end;
    cps_api_object_t        req_obj;


    req_obj = cps_api_object_list_get(param->filters, key_ix);

    STD_ASSERT(req_obj != NULL);

    dn_pas_obj_key_media_get(req_obj, &qualifier, &slot_valid, &slot,
            &port_module_valid, &port_module, &port_valid, &port);

    if (slot_valid == false) {
        //TODO hadle chassis case when slot number is not part of
        //CPS key, CP should initiate collecting from all line cards
        dn_pas_myslot_get(&slot);
    }

    if (port_valid == false) {
        start = dn_media_id_to_port(PAS_MEDIA_START_PORT);
        end = dn_media_id_to_port(dn_phy_media_count_get());
    } else {
        start = end = port;
    }

    dn_pas_lock();

    for ( ; (start <= end); start++) {

        mtbl = dn_phy_media_entry_get(start);
        if ((mtbl == NULL) || (mtbl->res_data == NULL)) {
            PAS_ERR("Invalid media, port %u", start);

            continue;
        }

        if (((qualifier == cps_api_qualifier_REALTIME)
                   || (!mtbl->res_data->valid))
                && !dn_pald_diag_mode_get()) {
            //featch from hard ware
            dn_pas_phy_media_poll(start, true);

            if (!mtbl->res_data->valid) mtbl->res_data->valid = true;
        }

        if (dn_pas_is_media_obj_empty(req_obj, BASE_PAS_MEDIA_OBJ) == true) {

            if ((obj = dn_pas_media_data_publish(start, NULL, 0, true)) == NULL) {
                PAS_ERR("Failed to publish media event, port %u",
                        start
                        );

                dn_pas_unlock();
                return STD_ERR(PAS, FAIL, 0);
            }
        } else {

            if ((obj = cps_api_object_create()) == NULL) {
                dn_pas_unlock();
                return STD_ERR(PAS, NOMEM, 0);
            }

            if (dn_pas_media_populate_current_data(BASE_PAS_MEDIA_OBJ, req_obj,
                        obj, start, PAS_MEDIA_INVALID_ID) == false) {
                PAS_ERR("Failed to populate media object, port %u",
                        start
                        );

                cps_api_object_delete(obj);
                dn_pas_unlock();
                return STD_ERR(PAS, FAIL, 0);
            }
        }

        dn_pas_obj_key_media_set(obj, qualifier, true, slot, false,
                PAS_MEDIA_INVALID_PORT_MODULE, true, start);

        if (!cps_api_object_list_append(param->list, obj)) {

            cps_api_object_delete(obj);

            PAS_ERR("Failed to append response object, port %u",
                    start
                    );

            dn_pas_unlock();
            return STD_ERR(PAS, FAIL, 0);
        }
    }

    dn_pas_unlock();
    return STD_ERR_OK;
}

t_std_error dn_pas_media_set(cps_api_transaction_params_t * param,
        cps_api_object_t obj)
{
    uint32_t                port, port_module, slot;
    bool                    slot_valid, port_module_valid, port_valid;
    t_std_error             ret = STD_ERR_OK;
    cps_api_object_it_t     it;
    cps_api_attr_id_t       attr_id;
    cps_api_qualifier_t     qualifier;
    uint32_t                start, end;
    cps_api_object_t        cloned;
    cps_api_operation_types_t operation;

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

    operation = cps_api_object_type_operation(cps_api_object_key(obj));

    dn_pas_obj_key_media_get(obj, &qualifier, &slot_valid, &slot,
            &port_module_valid, &port_module, &port_valid, &port);

    if (slot_valid == false) {
        dn_pas_myslot_get(&slot);
    }

    if (port_valid == false) {
        start = dn_media_id_to_port(PAS_MEDIA_START_PORT);
        end = dn_media_id_to_port(dn_phy_media_count_get());
    } else {
        start = end = port;
    }

    for ( ; (start <= end); start++) {
        uint_t              count = 0;
        BASE_IF_SPEED_t     supported_speed[MAX_SUPPORTED_SPEEDS];

        memset(supported_speed, 0, sizeof(supported_speed));

        if ((cloned = cps_api_object_create()) == NULL) {
            PAS_ERR("Failed to create CPS API object, port %u",
                    start
                    );

            dn_pas_unlock();
            return STD_ERR(PAS, NOMEM, 0);
        }

        dn_pas_obj_key_media_set(cloned, qualifier, true, slot,
                port_module_valid, port_module, true, start);

        if (dn_pas_media_populate_current_data(BASE_PAS_MEDIA_OBJ, obj,
                    cloned, start, PAS_MEDIA_INVALID_ID) == false) {
            PAS_ERR("Failed to populate object, port %u",
                    start
                    );

            cps_api_object_delete(cloned);
            dn_pas_unlock();
            return STD_ERR(PAS, FAIL, 0);
        }

        if (cps_api_object_list_append(param->prev, cloned) == false) {
            PAS_ERR("Failed to append response object, port %u",
                    start
                    );

            cps_api_object_delete(cloned);
            dn_pas_unlock();
            return STD_ERR(PAS, FAIL, 0);
        }


        cps_api_object_it_begin(obj,&it);

        while (cps_api_object_it_valid (&it)) {

            attr_id = cps_api_object_attr_id (it.attr);

            switch (attr_id) {
                case BASE_PAS_MEDIA_HIGH_POWER_MODE:
                    {
                        bool status;

                        memcpy(&status, cps_api_object_attr_data_bin(it.attr),
                                sizeof(status));

                        if (dn_pas_media_high_power_mode_set(start, status)
                                == false) {
                            PAS_ERR("Failed to set media high-power mode, port %u",
                                    start
                                    );

                            ret = STD_ERR(PAS, FAIL, 0);
                        }
                    }
                    break;
                case BASE_PAS_MEDIA_AUTONEG:
                    {
                        bool autoneg;
                        memcpy(&autoneg, cps_api_object_attr_data_bin(it.attr),
                                 sizeof(autoneg));

                        if (dn_pas_media_phy_autoneg_config_set(start, PAS_MEDIA_CH_START,
                                    autoneg, operation) == false) {
                            ret = STD_ERR(PAS, FAIL, 0);
                        }
                    }
                    break;
                case BASE_PAS_MEDIA_SUPPORTED_SPEED:
                    {

                        supported_speed[count++] = cps_api_object_attr_data_uint(it.attr);
                    }
                    break;
                case BASE_PAS_MEDIA_TARGET_WAVELENGTH:
                    {
                        float value;

                        memcpy(&value, cps_api_object_attr_data_bin(it.attr),
                                sizeof(value));

                        if (dn_pas_media_wavelength_config_set(start, value,
                                    operation) == false) {
                            ret = STD_ERR(PAS, FAIL, 0);
                        }
                    }
                    break;
                default:
                    {
                        if (dn_pas_media_is_key_attr(BASE_PAS_MEDIA_OBJ, attr_id)
                                != true) {
                            PAS_WARN("Invalid attribute, port %u, attribute %u",
                                     start, attr_id
                                     );

                            ret = STD_ERR(PAS, FAIL, 0);
                        }
                    }
            }
            cps_api_object_it_next (&it);
        }

        if (count > 0) {
            if (dn_pas_media_phy_supported_speed_config_set(start, PAS_MEDIA_CH_START,
                        supported_speed, count, operation) == false) {
                ret = STD_ERR(PAS, FAIL, 0);
            }
        }
    }
    dn_pas_unlock();
    return ret;
}
