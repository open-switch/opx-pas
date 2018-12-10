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
 * filename: pas_display.h
 *
 */

#ifndef __PAS_DISPLAY_H
#define __PAS_DISPLAY_H

#include "std_type_defs.h"     
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_key.h"
#include "pas_res_structs.h"

/* Get cache record for display by name */

pas_display_t *dn_pas_disp_rec_get_name(
    uint_t entity_type,
    uint_t slot,
    char   *disp_name
                                     );

/* Get cache record for display by index */

pas_display_t *dn_pas_disp_rec_get_idx(
    uint_t entity_type,
    uint_t slot,
    uint_t disp_idx
                                  );

/* Initialize display record in cache for given entity */

void dn_cache_init_disp(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       );

/* Delete all display record in cache for given entity */

void dn_cache_del_disp(pas_entity_t *parent);

#endif /* !defined(__PAS_DISPLAY_H) */
