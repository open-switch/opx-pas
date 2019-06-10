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

/*
 * filename: pas_pld_handler.c
 */ 
     
#include "private/pas_main.h"
#include "private/pas_log.h"
#include "private/pas_utils.h"

#include "std_error_codes.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"
#include "cps_api_key.h"
#include "cps_api_object_key.h"
#include "cps_api_object.h"
#include "cps_class_map.h"

#include <stdio.h>

static const char version_file[] = "/var/log/firmware_versions";

t_std_error dn_pas_pld_get(cps_api_get_params_t * param, size_t key_idx)
{
    cps_api_object_t    req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_key_t       *key = cps_api_object_key(req_obj);    
    cps_api_qualifier_t qual = cps_api_key_get_qual(key);
    switch (qual) {
    case cps_api_qualifier_OBSERVED:
    case cps_api_qualifier_REALTIME:
        break;
    default:
        return (STD_ERR_OK);
    }

    /* Extract key fields in request */

    bool       entity_type_valid = false, slot_valid = false, name_valid = false;
    uint_t     entity_type = 0, slot = 0;
    const char *name = NULL;

    cps_api_object_attr_t a;
    a = cps_api_get_key_data(req_obj, BASE_PAS_PLD_ENTITY_TYPE);
    if ((entity_type_valid = (a != 0))) {
        entity_type = cps_api_object_attr_data_u8(a);
    }
    a = cps_api_get_key_data(req_obj, BASE_PAS_PLD_SLOT);
    if ((slot_valid = (a != 0))) {
        slot = cps_api_object_attr_data_u8(a);
    }
    a = cps_api_get_key_data(req_obj, BASE_PAS_PLD_NAME);
    if ((name_valid = (a != 0))) {
        name = (const char *) cps_api_object_attr_data_bin(a);
    }

    /* Only valid for my card */
    uint_t my_slot;
    if (!dn_pas_myslot_get(&my_slot)) {
        PAS_ERR("Failed to get my slot");
        return (STD_ERR_OK);
    }
    if ((entity_type_valid && entity_type != PLATFORM_ENTITY_TYPE_CARD)
        || (slot_valid && slot != my_slot)
        ) {
        return (STD_ERR_OK);
    }

    /* Search version file for given name */

    FILE *fp = fopen(version_file, "r");
    if (fp == 0) {
        PAS_ERR("Failed to open PLD version file");
        return (STD_ERR_OK);
    }

    while (!feof(fp)) {
        char buf[80];
        if (fgets(buf, sizeof(buf), fp) == 0) {
            PAS_ERR("Read of PLD version file failed");
            break;
        }
        char *p = index(buf, ':');
        if (p == 0) {
            PAS_ERR("Bad format of PLD version file");
            break;
        }
        
        /* Skip if name given and no match */
        
        *p = 0;
        if (name_valid && strcmp(name, buf) != 0)  continue;
        
        /* Create response obect, and add to response */
        
        cps_api_object_t resp_obj = cps_api_object_create();
        if (resp_obj == CPS_API_OBJECT_NULL) {
            PAS_ERR("Failed to allocate CPS API object for response");
            break;
        }
        if (!cps_api_object_list_append(param->list, resp_obj)) {
            PAS_ERR("Failed to add CPS API object to response");
            cps_api_object_delete(resp_obj);
            break;
        }
        
        /* Add response attributes */
        
        uint8_t temp = PLATFORM_ENTITY_TYPE_CARD;
        cps_api_object_attr_add_u8(resp_obj, BASE_PAS_PLD_ENTITY_TYPE, temp);
        cps_api_set_key_data(resp_obj,
                             BASE_PAS_PLD_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
        temp = my_slot;
        cps_api_object_attr_add_u8(resp_obj, BASE_PAS_PLD_SLOT, temp);
        cps_api_set_key_data(resp_obj,
                             BASE_PAS_PLD_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
        cps_api_object_attr_add_str(resp_obj, BASE_PAS_PLD_NAME, buf);
        cps_api_set_key_data(resp_obj,
                             BASE_PAS_PLD_NAME,
                             cps_api_object_ATTR_T_BIN,
                             buf, strlen(buf) + 1
                             );
        p += 2;
        char *q = index(p, '\n');
        if (q != 0)  *q = 0;
        cps_api_object_attr_add_str(resp_obj, BASE_PAS_PLD_VERSION, p);
    }

    fclose(fp);

    return (STD_ERR_OK);
}


t_std_error dn_pas_pld_set(cps_api_key_t *key, cps_api_object_t obj)
{
    return (STD_ERR_OK);
}
