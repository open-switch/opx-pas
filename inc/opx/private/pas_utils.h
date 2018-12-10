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

#ifndef __PAS_UTILS_H
#define __PAS_UTILS_H

#include "private/pas_res_structs.h"

#include "std_type_defs.h"
#include "cps_api_key.h"
#include "cps_api_object.h"
#include "sdi_entity.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"

/* Return current slot number */

bool dn_pas_myslot_get (uint_t *slot);

/* Get a CPS uint8 attribute */

static inline uint_t cps_api_object_attr_data_u8(cps_api_object_attr_t attr)
{
    return (* (uint8_t *) cps_api_object_attr_data_bin(attr));
}

/* Set a CPS uint8 attribute */

static inline bool cps_api_object_attr_add_u8(
    cps_api_object_t  obj,
    cps_api_attr_id_t attr,
    uint_t            val
                                              )
{
    uint8_t temp = val;

    return (cps_api_object_attr_add(obj, attr, &temp, sizeof(temp)));
}

int cps_api_object_attr_data_int(cps_api_object_attr_t a);

/* Get SDI entity info for given entity */

sdi_resource_hdl_t dn_pas_entity_info_hdl(sdi_entity_hdl_t sdi_entity_hdl);

static inline void strlcpy(char *dst, const char *src, size_t dst_size)
{
    strncpy(dst, src, dst_size);
    dst[dst_size - 1] = 0;
}

#define STRLCPY(_dst, _src)  strlcpy((_dst), (_src), sizeof(_dst))

static inline bool cps_api_object_attr_add_str(
    cps_api_object_t  obj,
    cps_api_attr_id_t attr,
    const char        *val
                                              )
{
    return (cps_api_object_attr_add(obj, attr, val, strlen(val) + 1));
}

static inline void dn_pas_oper_fault_state_init(pas_oper_fault_state_t *p)
{
    p->oper_status = BASE_CMN_OPER_STATUS_TYPE_UP;
    p->fault_type  = PLATFORM_FAULT_TYPE_OK;
}

bool dn_pas_oper_fault_state_update(
    pas_oper_fault_state_t *p,
    uint_t                 fault_type
                                    );

void dn_pas_service_tag_to_express_service_code(
    char     *buf,
    unsigned bufsize,
    char     *service_tag
                                                );

#endif /* !defined(__PAS_UTILS_H) */
