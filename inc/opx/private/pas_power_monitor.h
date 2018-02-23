/*
 * Copyright (c) 2017 Dell Inc.
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
 * filename: pas_power_monitor.h
 *
 */

#ifndef __PAS_POWER_MONITOR__H
#define __PAS_POWER_MONITOR__H

#include "std_type_defs.h"     
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_key.h"
#include "pas_res_structs.h"

/* Initialize power-monitor record in cache for given entity */

void dn_cache_init_power_monitor(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       );

/* Delete all power-monitor records in cache for given entity */

void dn_cache_del_power_monitor(pas_entity_t *parent);

/* Poll a Power Monitor */

bool dn_power_monitor_poll(pas_power_monitor_t *rec);


#endif /* !defined(__PAS_POWER_MONITOR_H) */
