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

/**
 * filename: pas_temp_sensor.h
 *
 */

#ifndef __PAS_TEMP_SENSOR__H
#define __PAS_TEMP_SENSOR__H

#include "std_type_defs.h"     
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_key.h"
#include "pas_res_structs.h"

/* Get cache record for sensor by name */

pas_temperature_sensor_t *dn_pas_temperature_rec_get_name(
    uint_t entity_type,
    uint_t slot,
    char   *sensor_name
                                                          );
/* Get cache record for sensor by index */

pas_temperature_sensor_t *dn_pas_temperature_rec_get_idx(
    uint_t entity_type,
    uint_t slot,
    uint_t sensor_idx
                                                         );

/* Initialize temp_sensor record in cache for given entity */

void dn_cache_init_temp_sensor(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       );

/* Initialize remote (NPU) temp_sensor record in cache */
void dn_cache_init_remote_temp_sensor(void);

/* Allocate a temperature sensor cache record */

pas_temperature_sensor_t *pas_temperature_new(void);

/* Free a temperature sensor cache record */

void pas_temperature_del(pas_temperature_sensor_t *p);

/* Delete all temp_sensor records in cache for given entity */

void dn_cache_del_temp_sensor(pas_entity_t *parent);

/* Check temperature against thresholds */

bool dn_temp_sensor_thresh_chk(pas_temperature_sensor_t *rec);

/* Poll a temp_sensor */

bool dn_temp_sensor_poll(pas_temperature_sensor_t *rec, bool update_allf, bool *parent_notif);

/* Send a notification about a temp_sensor */

bool dn_temp_sensor_notify(pas_temperature_sensor_t *rec);

#endif /* !defined(__PAS_TEMP_SENSOR_H) */
