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

#ifndef __PAS_PSU_H
#define __PAS_PSU_H

#include "std_type_defs.h"     
#include "cps_api_key.h"
#include "pas_res_structs.h"


/* Initialize PSU records in cache */

bool dn_cache_init_psu(void);

/* Poll a PSU */

bool dn_psu_poll(pas_psu_t *rec, bool update_allf);

#endif /* !defined(__PAS_PSU_H) */
