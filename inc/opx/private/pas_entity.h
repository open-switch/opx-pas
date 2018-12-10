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

#ifndef __PAS_ENTITY_H
#define __PAS_ENTITY_H

#include "std_type_defs.h"
#include "cps_api_key.h"
#include "pas_res_structs.h"


/* Initialize entity records in the cache */

bool dn_cache_init_entity(void);

/* Get cache record for entity */

pas_entity_t *dn_pas_entity_rec_get(uint_t entity_type, uint_t slot);

/* Poll an entity */

bool dn_entity_poll(pas_entity_t *rec, bool update_allf);

/* Set the fault status of an entity */

bool dn_pas_entity_fault_state_set(pas_entity_t *rec, uint_t fault_state);

/* Poll an CPU lpc bus access*/

bool dn_entity_lpc_bus_poll(pas_entity_t *rec,bool *lpc_test_status);

/* generic api to set led in pas */
t_std_error dn_pas_generic_led_set(uint_t entity_type,uint_t slot,
                                            char  *led_name,bool  led_state);

#endif /* !defined(__PAS_ENTITY_H) */
