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

/***************************************************************************
 * @file pas_utils.c
 *
 * @brief This file contains pas common utils definitions
 **************************************************************************/

#include "private/pas_utils.h"

#include "std_type_defs.h"


int cps_api_object_attr_data_int(cps_api_object_attr_t a)
{
    uint_t v = cps_api_object_attr_data_uint(a);

    switch (cps_api_object_attr_len(a)) {
    case 1:
        if (v & (1 << 7))  v |= (~((uint_t) 0) & ~((1 << 8) - 1));
        break;
    case 2:
        if (v & (1 << 15))  v |= (~((uint_t) 0) & ~((1 << 16) - 1));
        break;
    case 4:
        ;
    default:
        return (0);
    }

    return ((int) v);
}

/*
 * dn_pas_myslot_get gets my physical slot (unit) number from SDI and returns it
 * to the caller.
 */

bool dn_pas_myslot_get (uint_t *slot)
{
    //TODO call the SDI API and return the value
    //As of now returning 1, once sdi api is ready will replace with
    //actual function call

    if (slot == NULL) return false;

    *slot = 1;

    return true;
}

/* Get SDI entity info for given entity */

static void _dn_pas_entity_info_hdl(sdi_resource_hdl_t sdi_resource_hdl, void *p)
{
    if (sdi_resource_type_get(sdi_resource_hdl) == SDI_RESOURCE_ENTITY_INFO) {
        * (sdi_resource_hdl_t *) p = sdi_resource_hdl;
    }
}

sdi_resource_hdl_t dn_pas_entity_info_hdl(sdi_entity_hdl_t sdi_entity_hdl)
{
    sdi_resource_hdl_t result = 0;

    sdi_entity_for_each_resource(sdi_entity_hdl, _dn_pas_entity_info_hdl, &result);

    return (result);
}


static uint_t fault_type_pri[] = {
    [PLATFORM_FAULT_TYPE_OK]      = 0,
    [PLATFORM_FAULT_TYPE_UNKNOWN] = 1,
    [PLATFORM_FAULT_TYPE_ECOMM]   = 2,
    [PLATFORM_FAULT_TYPE_ECFG]    = 3,
    [PLATFORM_FAULT_TYPE_ECOMPAT] = 4,
    [PLATFORM_FAULT_TYPE_EPOWER]  = 5,
    [PLATFORM_FAULT_TYPE_EHW]     = 6
};

bool dn_pas_oper_fault_state_update(
    pas_oper_fault_state_t *p,
    uint_t                 fault_type
                                    )
{
    bool result = false;

    if (fault_type_pri[fault_type] > fault_type_pri[p->fault_type]) {
        p->fault_type = fault_type;

        result = (p->oper_status != BASE_CMN_OPER_STATUS_TYPE_FAIL);

        p->oper_status = BASE_CMN_OPER_STATUS_TYPE_FAIL;
    }

    return (result);
}

void
dn_pas_service_tag_to_express_service_code(char *buf, unsigned bufsize, char *service_tag)
{
    if (service_tag != 0 && service_tag[0] != 0) {
        /* Convert service tag as base-36 number to decimal */
        uint64_t val;
        char     c;
        for (val = 0; (c = *service_tag) != 0; ++service_tag) {
            val = 36 * val + (c <= '9' ? c - '0' : c - 'A' + 10);
        }
        char temp[12];
        snprintf(temp, sizeof(temp), "%lu", val);
        
        /* Insert a space as every third character */
        char     *p;
        unsigned i;
        for (i = 0, p = temp; bufsize > 1; ++buf, --bufsize) {
            if (i >= 3) {
                *buf = ' ';
                i = 0;
                continue;
            }
            if ((c = *p++) == 0)  break;
            *buf = c;
            ++i;
        }
    }
    *buf = 0;
}
