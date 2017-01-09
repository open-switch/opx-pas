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

#include "private/pas_entity.h"
#include "private/pas_fan_tray.h"
#include "private/pas_fan.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_utils.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_service.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>

#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))


bool dn_cache_init_fan_tray(void)
{
    uint_t         slot, n;
    pas_entity_t   *parent;
    pas_fan_tray_t *data;

    for (slot = 1, n = sdi_entity_count_get(SDI_ENTITY_FAN_TRAY);
         n;
         --n, ++slot
         ) {
        char res_key[PAS_RES_KEY_SIZE];

        parent = dn_pas_res_getc(dn_pas_res_key_entity(res_key, 
                                                       sizeof(res_key),
                                                       PLATFORM_ENTITY_TYPE_FAN_TRAY,
                                                       slot
                                                       )
                                 );
        if (parent == 0) {
            return (false);
        }

        data = CALLOC_T(pas_fan_tray_t, 1);
        if (data == 0) {
            return (false);
        }

        data->parent = parent;

        if (!dn_pas_res_insertc(dn_pas_res_key_fan_tray(res_key,
                                                        sizeof(res_key),
                                                        slot
                                                        ),
                                data
                                )
            ) {
            free(data);

            return (false);
        }
    }

    return (true);
}


bool dn_fan_tray_poll(pas_fan_tray_t *rec, bool update_allf)
{
    enum { FAULT_LIMIT = 3 };

    sdi_entity_info_t entity_info[1];
    pas_entity_t      *parent = rec->parent;
    bool              fault_status;

    if (!rec->valid || update_allf) {
        if (STD_IS_ERR(sdi_entity_info_read(parent->sdi_entity_info_hdl,
                                            entity_info
                                            )
                       )
            ) {
            dn_pas_entity_fault_state_set(parent,
                                          PLATFORM_FAULT_TYPE_ECOMM
                                          );
            
            return (false);
        }
        
        switch (entity_info->air_flow) {
        case SDI_PWR_AIR_FLOW_NORMAL:
            rec->fan_airflow_type = PLATFORM_FAN_AIRFLOW_TYPE_NORMAL;
            break;
        case SDI_PWR_AIR_FLOW_REVERSE:
            rec->fan_airflow_type = PLATFORM_FAN_AIRFLOW_TYPE_REVERSE;
            break;

        default:
            rec->fan_airflow_type = PLATFORM_FAN_AIRFLOW_TYPE_UNKNOWN;
        }
        
        rec->valid = true;
    }

    if (STD_IS_ERR(sdi_entity_fault_status_get(parent->sdi_entity_hdl,
                                               &fault_status
                                               )
                   )
        ) {
        dn_pas_entity_fault_state_set(parent,
                                      PLATFORM_FAULT_TYPE_ECOMM
                                      );
    } else if (fault_status) {
        if (parent->fault_cnt < FAULT_LIMIT)  ++parent->fault_cnt;
        if (parent->fault_cnt >= FAULT_LIMIT) {
            dn_pas_entity_fault_state_set(parent,
                                          PLATFORM_FAULT_TYPE_EHW
                                          );
        }
    } else {
        parent->fault_cnt = 0;
    }

    return (true);
}
