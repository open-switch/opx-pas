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

#include "private/pas_fan.h"
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
#include "sdi_fan.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>


/* Send a fan notification */

bool dn_fan_notify(pas_fan_t *rec)
{
    cps_api_object_t obj;

    obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {
        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_fan_set(obj,
                           cps_api_qualifier_OBSERVED,
                           true,
                           rec->parent->entity_type,
                           true,
                           rec->parent->slot,
                           true,
                           rec->fan_idx
                           );

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_FAN_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_FAN_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );

    return (dn_pas_cps_notify(obj));
}


#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))

void dn_cache_init_fan(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       )
{
    pas_entity_t             *parent = (pas_entity_t *) data;
    pas_fan_t                *rec;
    struct pas_config_entity *e;

    e = dn_pas_config_entity_get_type(parent->entity_type);
    if (e == 0)  return;

    ++parent->num_fans;

    rec = CALLOC_T(pas_fan_t, 1);
    if (rec == 0)  return;

    rec->parent           = parent;
    rec->fan_idx          = parent->num_fans;
    rec->sdi_resource_hdl = sdi_resource_hdl;

    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    rec->speed_control_en      = e->fan.speed_control_en;
    rec->speed_err_margin      = e->fan.margin;
    rec->speed_err_integ.limit = e->fan.limit;
    rec->speed_err_integ.incr  = e->fan.incr;
    rec->speed_err_integ.decr  = e->fan.decr;

    char res_key[PAS_RES_KEY_SIZE];

    if (!dn_pas_res_insertc(dn_pas_res_key_fan(res_key,
                                               sizeof(res_key),
                                               parent->entity_type,
                                               parent->slot,
                                               parent->num_fans
                                               ),
                            rec
                            )
        ) {
        free(rec);
    }
}

void dn_cache_del_fan(pas_entity_t *parent)
{
    uint_t        fan_idx;

    for (fan_idx = 1; fan_idx <= parent->num_fans; ++fan_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        free(dn_pas_res_removec(dn_pas_res_key_fan(res_key,
                                                   sizeof(res_key),
                                                   parent->entity_type,
                                                   parent->slot,
                                                   fan_idx
                                                   )
                                )
             );
    }
}


bool dn_fan_poll(
    pas_fan_t *rec,
    bool      update_allf,
    bool      *parent_notif
                 )
{
    enum { FAULT_LIMIT = 3 };

    sdi_entity_info_t      entity_info[1];
    pas_entity_t           *parent = rec->parent;
    bool                   fault_status;
    uint_t                 speed, targ_speed;
    bool                   notif = false, power_status;

    pas_oper_fault_state_t prev_oper_fault_state[1];
    *prev_oper_fault_state = *rec->oper_fault_state;
    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    if (!rec->valid || update_allf) {
        if (STD_IS_ERR(sdi_entity_info_read(parent->sdi_entity_info_hdl,
                                            entity_info
                                            )
                       )
            ) {

            if (dn_pas_entity_fault_state_set(parent,
                                              PLATFORM_FAULT_TYPE_ECOMM
                                              )
                ) {
                *parent_notif = true;
            }

            return (false);
        }

        rec->max_speed = entity_info->max_speed;

        rec->valid = true;
    }

    /* Do not poll a PSU fan if it is not powered on */
    if (parent && parent->entity_type == PLATFORM_ENTITY_TYPE_PSU) {
        if (STD_IS_ERR(sdi_entity_psu_output_power_status_get(
                                parent->sdi_entity_hdl, &power_status))) {
                dn_pas_entity_fault_state_set(parent,
                                              PLATFORM_FAULT_TYPE_ECOMM
                                              );
                return false;
            } else if (!power_status) {
                dn_pas_entity_fault_state_set(parent,
                                              PLATFORM_FAULT_TYPE_EPOWER
                                              );
                return false;
            }
    }

    do {
        if (STD_IS_ERR(sdi_fan_status_get(rec->sdi_resource_hdl, &fault_status))) {
            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );

            break;
        }

        if (fault_status) {
            if (rec->fault_cnt < FAULT_LIMIT) ++rec->fault_cnt;
            if (rec->fault_cnt >= FAULT_LIMIT) {
                dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                               PLATFORM_FAULT_TYPE_EHW
                                               );
                break;
            }

            /*
             * If we haven't hit the fault limit yet, restore the
             * fault state and return early.
             */
            *rec->oper_fault_state = *prev_oper_fault_state;
            return (true);

        }
        rec->fault_cnt = 0;

        if (STD_IS_ERR(sdi_fan_speed_get(rec->sdi_resource_hdl, &speed))) {
            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM
                                           );

            break;
        }

        rec->obs_speed = speed;

        targ_speed = rec->targ_speed == 0 ? rec->max_speed : rec->targ_speed;

        if (targ_speed == 0) {
            /* Target speed is invalid */

            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECFG
                                           );

            break;
        }

        /* Skip speed monitoring, if so configured */
        if (!rec->speed_control_en)  break;

        if ( ((int) rec->obs_speed < (int) targ_speed) && ((100 * abs((int) rec->obs_speed - (int) targ_speed)) / targ_speed)
            >= rec->speed_err_margin
            ) {
            /* | Observed speed - target speed | exceeds margin */

            /* Try to set proper speed */

            if (rec->targ_speed != targ_speed) {

                if (STD_IS_ERR(sdi_fan_speed_set(rec->sdi_resource_hdl,
                                                 targ_speed))) {
                    dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                                   PLATFORM_FAULT_TYPE_ECOMM);
                } else {
                    rec->targ_speed = targ_speed;
                }

            }

            /* Pump up integrator; raise error if limit reached or exceeded */

            if (rec->speed_err_integ.sum < rec->speed_err_integ.limit) {
                rec->speed_err_integ.sum += rec->speed_err_integ.incr;

                if (rec->speed_err_integ.sum >= rec->speed_err_integ.limit) {
                    rec->speed_err = true;

                    rec->speed_err_integ.sum = rec->speed_err_integ.limit;
                }
            }
        } else {
            /* | Observed speed - target speed | within margin */

            /* Pump down integrator; clear error if 0 */

            if (rec->speed_err_integ.decr > rec->speed_err_integ.sum) {
                rec->speed_err_integ.sum = 0;
            } else {
                rec->speed_err_integ.sum -= rec->speed_err_integ.decr;
            }

            if (rec->speed_err_integ.sum == 0)  rec->speed_err = false;
        }

        if (rec->speed_err) {
            dn_pas_oper_fault_state_update(rec->oper_fault_state,
                                           PLATFORM_FAULT_TYPE_EHW
                                           );
        }
    } while (0);

    if (rec->oper_fault_state->oper_status != prev_oper_fault_state->oper_status) {
        /* Operationl status changed => Send notification */

        notif = true;
    }

    if (notif)  dn_fan_notify(rec);

    return (true);
}
