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
 * filename: pas_remote_poller.c
 */
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_temp_sensor.h"
#include "private/pas_utils.h"
#include "private/pas_data_store.h"
#include "private/pas_comm_dev.h"

#include "cps_api_key.h"
#include "cps_api_object_key.h"
#include "cps_api_object.h"
#include "cps_class_map.h"
#include "cps_api_operation.h"
#include "cps_api_service.h"

#include "dell-base-pas.h"
#include "dell-base-common.h"   /** \todo Simplify */
#include "dell-base-platform-common.h"
#include "dell-base-switch-element.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

static volatile bool remote_npu_temp_sensor_init_flag = false;

/* initiaize cacheDB entry for remote NPU temp sensor */
void dn_cache_init_remote_temp_sensor(void)
{
    /* slot_no and sensor_name are hard-coded for pizza-boxes */
    uint_t                    slot_no      = 1;
    const char                *sensor_name = "NPU temp sensor";
    pas_entity_t              *parent;
    pas_temperature_sensor_t  *rec;

    /* get parent from cache */

    char res_key[PAS_RES_KEY_SIZE];

    parent = (pas_entity_t *) dn_pas_res_getc(dn_pas_res_key_entity(res_key,
                                                                    sizeof(res_key),
                                                                    PLATFORM_ENTITY_TYPE_CARD,
                                                                    slot_no
                                                                    )
                                              );
    if (NULL == parent) return;

    rec = pas_temperature_new();
    if (rec == 0)  return;

    ++parent->num_temp_sensors;
    STRLCPY(rec->name, sensor_name);
    rec->parent           = parent;
    rec->sensor_idx       = parent->num_temp_sensors;
    rec->sdi_resource_hdl = NULL; /* not handled by SDI, hence resource_hdl is NULL */
    rec->last_thresh_crossed->temperature = -9999;
    rec->last_thresh_crossed->dir         = 1;

    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    /* inserting rec in cacheDB by name */
    char res_key_name[PAS_RES_KEY_SIZE];

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

    /* inserting same rec in cacheDB by index */
    if (!dn_pas_res_insertc(dn_pas_res_key_temp_sensor_idx(res_key,
                                                           sizeof(res_key),
                                                           parent->entity_type,
                                                           parent->slot,
                                                           rec->sensor_idx
                                                           ),
                            rec
                            )
        ) {
    	/* remove the corresponding "name" entry from cacheDB */
        dn_pas_res_removec(res_key_name);
        pas_temperature_del(rec);
        return;
    }

    remote_npu_temp_sensor_init_flag = true;
}

/* Get temperature value on a NPU Temperature Sensor by NAS CPS get */
static t_std_error dn_pas_npu_temperature_get(uint_t *temp)
{
    int                  switch_id = 0;
    cps_api_key_t        key;
    cps_api_get_params_t gp;
    bool                 err_flag  = false;

    /* Initialize a CPS get request */
    cps_api_get_request_init(&gp);
    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);

    do {
        if (NULL == obj) {
            PAS_ERR("Failed to create CPS API object");

            err_flag = true;
            break;
        }

        /* make a CPS key and attach to request */
        cps_api_key_from_attr_with_qual(&key,
                                        BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY,
                                        cps_api_qualifier_TARGET);
        cps_api_object_set_key(obj, &key);

        cps_api_set_key_data(obj,
                             BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_ID,
                             cps_api_object_ATTR_T_U32,
                             &switch_id,
                             sizeof(switch_id));

        cps_api_object_attr_add_u32(obj,
                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_TEMPERATURE,
                0);


        /* getting a list of CPS response objects */
        if (cps_api_get(&gp) == cps_api_ret_code_OK) {

            size_t mx = cps_api_object_list_size(gp.list);
            size_t ix = 0;

            for (ix = 0 ; ix < mx ; ++ix ) {

                cps_api_object_it_t   it;
                cps_api_object_attr_t a;
                cps_api_object_t      obj = cps_api_object_list_get(gp.list, ix);

                cps_api_object_it_begin(obj, &it);
                a = cps_api_object_attr_get(obj,
                                            BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_TEMPERATURE);

                if(a != CPS_API_ATTR_NULL) {

            	    *temp = cps_api_object_attr_data_uint(a);

                } else {
                    PAS_ERR("Temperature attribute not found");

            	    err_flag = true;
            	    break;
            	}
            } /* for loop - closing brace */
        } else {

    	    err_flag = true;
    	    break;
        }
    } while(0);

    cps_api_get_request_close(&gp);
    if (err_flag) {

        return STD_ERR(PAS, FAIL, 0);
    } else {

        return STD_ERR_OK;
    }
}

/* Poll a remote temperature sensor on the NPU */
static bool dn_remote_temp_sensor_poll(void)
{
    /* slot_no and sensor_name are hard-coded for pizza-boxes */
    uint_t                   slot_no       = 1;
    char                     *sensor_name  = "NPU temp sensor";
    bool                     notif         = false;
    bool                     ret           = true;
    uint_t                   temp = 0;
    pas_oper_fault_state_t   oper_fault_state[1];
    pas_temperature_sensor_t *temp_rec;
    pas_temperature_sensor_t *rec;

    /* get parent from cache */

    char res_key[PAS_RES_KEY_SIZE];

    pas_entity_t *parent = (pas_entity_t *)
        dn_pas_res_getc(dn_pas_res_key_entity(res_key,
                                              sizeof(res_key),
                                              PLATFORM_ENTITY_TYPE_CARD,
                                              slot_no
                                              )
                        );
    if (NULL == parent) return false;

    /* retrieve temp record from the cache based on name */
    temp_rec = dn_pas_temperature_rec_get_name(parent->entity_type,
                                               parent->slot,
                                               sensor_name
                                               );
    if (NULL == temp_rec) return false;

    /* retrieve temp record from the cache based on index */
    rec = dn_pas_temperature_rec_get_idx(parent->entity_type,
                                         parent->slot,
                                         temp_rec->sensor_idx);
    if (NULL == rec) return false;

    dn_pas_oper_fault_state_init(oper_fault_state);

    do {
        /* get the realtime temperature value from NAS and if nothing is returned say error */
        if (STD_IS_ERR(dn_pas_npu_temperature_get(&temp))) {
            PAS_ERR("CPS API get failed");

            dn_pas_oper_fault_state_update(oper_fault_state,
                                           PLATFORM_FAULT_TYPE_ECOMM);

            ret = false;
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

    return ret;
}

/* NPU temperature sensor poller thread initialization */
t_std_error dn_pas_remote_poller_thread(void)
{
    /* hard-coded for now; will use config file in future depending on design */
    int polling_freq = 5;

    /* keep trying till remote npu temp sensor's cacheDB entry gets initialized */
    while(!remote_npu_temp_sensor_init_flag) {

    	sleep(1);
    }

    PAS_NOTICE("Remote poller initialized");

    for (;;) {

        sleep(polling_freq);

        dn_pas_lock();
        if (false == dn_remote_temp_sensor_poll()) {
            PAS_ERR("Poll cycle failed");
        }
        dn_pas_unlock();
    }

    PAS_NOTICE("Remote poller exiting");

    return (STD_ERR_OK);        /* Should never return */
}
