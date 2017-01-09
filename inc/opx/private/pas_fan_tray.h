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

#ifndef __PAS_FAN_TRAY_H
#define __PAS_FAN_TRAY_H

#include "std_type_defs.h"     
#include "cps_api_key.h"
#include "pas_res_structs.h"


/* Initialize fan tray records in cache */

bool dn_cache_init_fan_tray(void);

/* Poll a FAN_TRAY */

bool dn_fan_tray_poll(pas_fan_tray_t *rec, bool update_allf);

#endif /* !defined(__PAS_FAN_TRAY_H) */
