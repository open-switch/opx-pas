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
 * filename: pas_status_handler.c
 */ 
     
#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_utils.h"
#include "private/dn_pas.h"

#include "std_error_codes.h"
#include "cps_class_map.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_status_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    uint_t               slot
    )
{
    cps_api_object_t resp_obj;

    /* Compose respose object */
    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_pas_status_set(resp_obj,
                                  qual,
                                  true, 
                                  slot
                                 );
        
    cps_api_object_attr_add_u8( resp_obj,
                                BASE_PAS_READY_STATUS,
                                dn_pald_status_get()
                                );
    
    /* Add response object to get response */
    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given status instance */

t_std_error dn_pas_status_get(cps_api_get_params_t * param, size_t key_idx)
{
    t_std_error         result  = STD_ERR_OK, ret;
    cps_api_object_t    req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t qual;
    bool                slot_valid;
    uint_t              slot;

    dn_pas_obj_key_pas_status_get(req_obj,
                                  &qual,
                                  &slot_valid, 
                                  &slot
                                 );
    uint_t my_slot = 0;
    dn_pas_myslot_get(&my_slot);

    if(slot_valid && (my_slot != slot)) {
        
        return STD_ERR(PAS, FAIL, 0);
    }

    ret = dn_pas_status_get1(param, qual, slot);

    if (ret == STD_ERR(PAS, FAIL, 0) || ret == STD_ERR(PAS, NOMEM, 0)) {

        result = STD_ERR(PAS, NEXIST, 0);
    }

    return (result);
}

/* Register with CPS */

void dn_pas_status_init(cps_api_operation_handle_t cps_hdl) 
{
    static const uint_t cps_api_qualifiers[] = {
        cps_api_qualifier_OBSERVED,
        cps_api_qualifier_REALTIME
    };

    uint_t                           i;
    cps_api_registration_functions_t reg[1];

    for (i = 0; i < ARRAY_SIZE(cps_api_qualifiers); ++i) {
        reg->handle = cps_hdl;
        cps_api_key_from_attr_with_qual(&reg->key,
                                        BASE_PAS_READY_OBJ,
                                        cps_api_qualifiers[i]
                                        );
        reg->_read_function     = dn_pas_read_function;
        reg->_write_function    = NULL;
        reg->_rollback_function = NULL;

        if (cps_api_register(reg) != cps_api_ret_code_OK)  return;
    }
}
