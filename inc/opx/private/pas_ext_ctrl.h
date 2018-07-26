/*
 * Copyright (c) 2018 Dell EMC..
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
 * filename: pas_ext_ctrl.h
 *
 */

#ifndef __PAS_EXT_CTRL_H
#define __PAS_EXT_CTRL_H

#include "std_type_defs.h"     
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_key.h"
#include "pas_res_structs.h"

/* Get cache record for extctrl by name */

pas_extctrl_t *dn_pas_extctrl_rec_get_name(pas_entity_t *parent, char *extctrl_name);

/* Get cache record for extctrl by index */

pas_extctrl_t *dn_pas_extctrl_rec_get_idx (uint_t extctrl_idx);


/* Initialize extctrl record in cache for given entity */

void dn_cache_init_extctrl_db (pas_entity_t *cd_rec);

void dn_cache_init_extctrl (sdi_resource_hdl_t sdi_resource_hdl, void *data);

/* Delete all extctrl record in cache for given entity */

bool dn_cache_del_extctrl (pas_entity_t *cd_rec);

/*
 * dn_extctrl_poll is used to poll ext ctrl for messages
 */
bool dn_entity_extctrl_poll (pas_entity_t *rec, bool update_allf, bool *notif);

#endif /* !defined(__PAS_EXT_CTRL_H) */
