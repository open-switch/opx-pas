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
 * filename: pas_nvram.h
 *
 */

#ifndef __PAS_NVRAM_H
#define __PAS_NVRAM_H

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_key.h"
#include "pas_res_structs.h"
#include "std_llist.h"

typedef struct _pas_nvram_entry_t {
    std_dll     list_pointers;

    uint64_t    tag;
    uint64_t    length;
    uint8_t     data[256]; /* Max length of data is 256 bytes */
} pas_nvram_entry_t;

/* Initialize NVRAM record in cache for given entity */

void dn_cache_init_nvram(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       );

/* Delete all NVRAM record in cache for given entity */

void dn_cache_del_nvram(pas_entity_t *parent);

/* Return the NVRAM cache record */
pas_nvram_t *dn_pas_nvram_rec_get(void);

/* Parse NVRAM data */
void dn_nvram_parse(void);

/* Write NVRAM to disk */
void dn_nvram_write(void);

/* Initialize the NVRAM structure */
void dn_nvram_init(void);

#endif /* !defined(__PAS_NVRAM_H) */
