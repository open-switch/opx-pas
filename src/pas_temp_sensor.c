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

#include "private/pas_comm_dev.h"
#include "private/pas_temp_sensor.h"
#include "private/pas_entity.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_event.h"
#include "private/pas_config.h"
#include "private/pas_utils.h"
#include "private/pas_data_store.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "std_time_tools.h"
#include "sdi_entity.h"
#include "sdi_thermal.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>

#define ARRAY_SIZE(a)        (sizeof(a) / sizeof((a)[0]))
#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))


/* Send a temperature notification */

bool dn_temp_sensor_notify(pas_temperature_sensor_t *rec)
{
    cps_api_object_t obj;

    obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {
        return (false);
    }

    dn_pas_obj_key_temperature_set(obj,
                                   cps_api_qualifier_OBSERVED,
                                   true, rec->parent->entity_type,
                                   true, rec->parent->slot,
                                   true, rec->name
                                   );

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_TEMPERATURE_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_TEMPERATURE_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );
    cps_api_object_attr_add_u16(obj,
                                BASE_PAS_TEMPERATURE_TEMPERATURE,
                                rec->cur
                                );
    cps_api_object_attr_add_u16(obj,
                                BASE_PAS_TEMPERATURE_LAST_THRESH,
                                rec->last_thresh_crossed->temperature
                                );
    return (dn_pas_cps_notify(obj));
}

/* Allocate a temperature sensor cache record */

pas_temperature_sensor_t *pas_temperature_new(void)
{
    pas_temperature_sensor_t      *result;
    struct pas_config_temperature *cfg;

    cfg = dn_pas_config_temperature_get();

    result = CALLOC_T(pas_temperature_sensor_t, 1);
    if (result != 0) {
        result->num_thresh = cfg->num_thresholds;
        result->thresholds = CALLOC_T(pas_temp_threshold_t, result->num_thresh);
        if (result->thresholds == 0) {
            free(result);

            result = 0;
        }
    }

    return (result);
}

/* Free a temperature sensor cache record */

void pas_temperature_del(pas_temperature_sensor_t *p)
{
    free(p->thresholds);
    free(p);
}

/* Create a temperature sensor cache record */

void dn_cache_init_temp_sensor(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       )
{
    pas_entity_t             *parent = (pas_entity_t *) data;
    const char               *sensor_name;
    char                     res_key_name[PAS_RES_KEY_SIZE];
    char                     res_key_idx[PAS_RES_KEY_SIZE];
    pas_temperature_sensor_t *rec;

    ++parent->num_temp_sensors;

    rec = pas_temperature_new();
    if (rec == 0)  return;

    sensor_name = sdi_resource_alias_get(sdi_resource_hdl);

    rec->parent           = parent;
    rec->sensor_idx       = parent->num_temp_sensors;
    STRLCPY(rec->name, sensor_name);
    rec->sdi_resource_hdl = sdi_resource_hdl;
    rec->last_thresh_crossed->temperature = -9999;
    rec->last_thresh_crossed->dir         = 1;

    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    if (!dn_pas_res_insertc(dn_pas_res_key_temp_sensor_name(res_key_name,
                                                            sizeof(res_key_name),
                                                            parent->entity_type,
                                                            parent->slot,
                                                            rec->name
                                                            ),
                            rec
                            )
        ) {
        pas_temperature_del(rec);

        return;
    }

    if (!dn_pas_res_insertc(dn_pas_res_key_temp_sensor_idx(res_key_idx,
                                                           sizeof(res_key_idx),
                                                           parent->entity_type,
                                                           parent->slot,
                                                           rec->sensor_idx
                                                           ),
                            rec
                            )
        ) {
        dn_pas_res_removec(res_key_name);
        pas_temperature_del(rec);
    }
}

pas_temperature_sensor_t *dn_pas_temperature_rec_get_name(
    uint_t entity_type,
    uint_t slot,
    char   *sensor_name
)
{
    char res_key[PAS_RES_KEY_SIZE];

    return ((pas_temperature_sensor_t *)
            dn_pas_res_getc(dn_pas_res_key_temp_sensor_name(res_key,
                                                            sizeof(res_key),
                                                            entity_type,
                                                            slot,
                                                            sensor_name
                                                            )
                            )
            );
}

pas_temperature_sensor_t *dn_pas_temperature_rec_get_idx(
    uint_t entity_type,
    uint_t slot,
    uint_t sensor_idx
                                                         )
{
    char res_key[PAS_RES_KEY_SIZE];

    return ((pas_temperature_sensor_t *)
            dn_pas_res_getc(dn_pas_res_key_temp_sensor_idx(res_key,
                                                           sizeof(res_key),
                                                           entity_type,
                                                           slot,
                                                           sensor_idx
                                                           )
                            )
            );
}

/* Delete a temperature sensor cache record */

void dn_cache_del_temp_sensor(pas_entity_t *parent)
{
    uint_t                   sensor_idx;
    pas_temperature_sensor_t *rec;

    for (sensor_idx = 1; sensor_idx <= parent->num_temp_sensors; ++sensor_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        rec = dn_pas_res_removec(dn_pas_res_key_temp_sensor_idx(res_key,
                                                                sizeof(res_key),
                                                                parent->entity_type,
                                                                parent->slot,
                                                                sensor_idx
                                                                )
                                 );

        dn_pas_res_removec(dn_pas_res_key_temp_sensor_name(res_key,
                                                           sizeof(res_key),
                                                           parent->entity_type,
                                                           parent->slot,
                                                           rec->name
                                                           )
                           );

        pas_temperature_del(rec);
    }
}

/* Check temperature readings against thresholds; return true
   iff threshold state changed
 */

bool dn_temp_sensor_thresh_chk(pas_temperature_sensor_t *rec)
{
    bool                 result = false;
    uint_t               n;
    pas_temp_threshold_t *thresh;
    bool                 above_valid = false, below_valid = false;
    int                  above = 0, below = 0;
    int                  thresh_crossed = rec->last_thresh_crossed->temperature, dir = rec->last_thresh_crossed->dir;

    if (!rec->thresh_en
        || rec->nsamples == 0
        || rec->num_thresh == 0
        || (rec->nsamples >= 2 && rec->cur == rec->prev)
        ) {
        return (false);
    }

    for (thresh = rec->thresholds, n = rec->num_thresh;
         n > 0;
         --n, ++thresh
         ) {
        if (!thresh->valid)  continue;

        if (rec->nsamples < 2) {
            if (rec->cur >= thresh->hi
                && (!above_valid || thresh->hi > above)
                ) {
                above       = thresh->hi;
                above_valid = true;
            }

            continue;
        }

        if (rec->cur >= thresh->hi
            && thresh->hi > rec->prev
            && (!above_valid || thresh->hi > above)
            ) {
            above       = thresh->hi;
            above_valid = true;
        }
        if (rec->cur < thresh->lo
            && thresh->lo <= rec->prev
            && (!below_valid || thresh->lo < below)
            ) {
            below       = thresh->lo;
            below_valid = true;
        }
    }

    if (above_valid) {
        thresh_crossed       = above;
        dir                  = 1;
    } else if (below_valid) {
        thresh_crossed       = below;
        dir                  = -1;
    }

    if ( rec->last_thresh_crossed->dir != dir
         || rec->last_thresh_crossed->temperature != thresh_crossed
        ) {
        rec->last_thresh_crossed->dir         = dir;
        rec->last_thresh_crossed->temperature = thresh_crossed;
        result = true;
    }

    return (result);
}

/* Poll a "local" temperature sensor, by making sdi calls.
   Generate notifications if required
 */

bool dn_temp_sensor_poll(
    pas_temperature_sensor_t *rec,
    bool                     update_allf,
    bool                     *parent_notif
                         )
{
    pas_oper_fault_state_t oper_fault_state[1];
    int                    temp;
    bool                   notif = false;
    bool                   fault_status = false;

    /* To avoid polling a temperature sensor not handled by SDI (NPU temp sensor)*/
    if(rec->sdi_resource_hdl == NULL)  return (true);

    dn_pas_oper_fault_state_init(oper_fault_state);

    do {
        if (STD_IS_ERR(sdi_temperature_status_get(rec->sdi_resource_hdl, &fault_status))) {
            dn_pas_oper_fault_state_update(oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );

            break;
        }

        if (fault_status) {
            dn_pas_oper_fault_state_update(oper_fault_state,
                                           PLATFORM_FAULT_TYPE_EHW
                                           );

            break;
        }

        if (STD_IS_ERR(sdi_temperature_get(rec->sdi_resource_hdl, &temp))) {
            dn_pas_oper_fault_state_update(oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );

            break;
        }

        rec->prev = rec->cur;
        rec->cur  = temp;
        if (rec->nsamples < 2)  ++rec->nsamples;

        if (dn_temp_sensor_thresh_chk(rec))  notif = true;
    } while (0);

    if (rec->oper_fault_state->oper_status != oper_fault_state->oper_status) {
        /* Operational status changed => Send notification */

        notif = true;
    }

    *rec->oper_fault_state = *oper_fault_state;

    rec->valid = true;

    if (notif)  dn_temp_sensor_notify(rec);

    rec->polltime_from_epoch = std_time_get_current_from_epoch_in_nanoseconds();

    return (true);
}
