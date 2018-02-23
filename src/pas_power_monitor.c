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
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_event.h"
#include "private/pas_config.h"
#include "private/pas_utils.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "sdi_power_monitor.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>


#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))

void dn_cache_init_power_monitor(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       )
{
    pas_entity_t             *parent = (pas_entity_t *) data;
    pas_power_monitor_t                *rec;
    struct pas_config_entity *e;

    e = dn_pas_config_entity_get_type(parent->entity_type);
    if (e == 0)  return;

    ++parent->num_power_monitors;

    rec = CALLOC_T(pas_power_monitor_t, 1);
    if (rec == 0)  return;

    rec->parent           = parent;
    rec->pm_idx           = parent->num_power_monitors;
    rec->sdi_resource_hdl = sdi_resource_hdl;

    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    char res_key[PAS_RES_KEY_SIZE];

    if (!dn_pas_res_insertc(dn_pas_res_key_pm(res_key,
                                               sizeof(res_key),
                                               parent->entity_type,
                                               parent->slot,
                                               parent->num_power_monitors
                                               ),
                            rec
                            )
        ) {
        free(rec);
    }
}

void dn_cache_del_power_monitor(pas_entity_t *parent)
    
{
    uint_t        pm_idx;

    for (pm_idx = 1; pm_idx <= parent->num_power_monitors; ++pm_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        free(dn_pas_res_removec(dn_pas_res_key_pm(res_key,
                                                   sizeof(res_key),
                                                   parent->entity_type,
                                                   parent->slot,
                                                   pm_idx
                                                   )
                               )
             );
    }
}

bool dn_power_monitor_poll(pas_power_monitor_t *rec)
{
    float                  voltage_volt = 0, current_amp = 0, power_watt = 0;

    do {
        if (STD_IS_ERR(sdi_power_monitor_current_amp_get(rec->sdi_resource_hdl, 
                                                         &current_amp))) {
            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );
            rec->valid = false; 
            break;
        }

        rec->obs_pm_current_amp = current_amp;

        if (STD_IS_ERR(sdi_power_monitor_voltage_volt_get(rec->sdi_resource_hdl, 
                                                          &voltage_volt))) {
            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );
            rec->valid = false; 
            break;
        }

        rec->obs_pm_voltage_volt = voltage_volt;

        if (STD_IS_ERR(sdi_power_monitor_power_watt_get(rec->sdi_resource_hdl, 
                                                        &power_watt))) {
            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );
            rec->valid = false; 
            break;
        }

        rec->obs_pm_power_watt = power_watt;

        rec->valid = true;

    } while (0);

    return (true);
}
