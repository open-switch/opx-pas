/*
 * Copyright (c) 2017 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/**************************************************************************
 * @file pas_host_system_handler.c
 * 
 * @brief This file contains source code of host-system CPS set/get handlers.
 **************************************************************************/


/*
 * filename: pas_host_system_handler.c
 */

#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_res_structs.h"
#include "private/pas_comm_dev.h"
#include "private/pas_host_system.h"
#include "dell-base-pas.h"
#include "cps_class_map.h"

#include <stdlib.h>


/*
 * CPS get handler for host-system object
 */

t_std_error dn_pas_host_system_get(cps_api_get_params_t *param, size_t key_idx)
{
    pas_host_system_t    *rec = NULL;
    cps_api_object_t     req_obj = cps_api_object_list_get(param->filters,
                                                           key_idx);

    STD_ASSERT(req_obj != NULL);
    cps_api_key_t  *key = cps_api_object_key(req_obj);

    if ((rec = dn_host_system_rec_get()) == NULL) {
        return STD_ERR(PAS, FAIL, 0);
    }

    cps_api_object_t obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {
        return STD_ERR(PAS, NOMEM, 0);
    }
    cps_api_object_set_key(obj, key);

    if (dn_pas_host_system_attr_add(obj) == false) {
        cps_api_object_delete(obj);
        return STD_ERR(PAS, NOMEM, 0);
    }

    if (!cps_api_object_list_append(param->list, obj)) {
        cps_api_object_delete(obj);
        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/*
 * CPS set handler for host-system object
 */

t_std_error dn_pas_host_system_set(cps_api_transaction_params_t *param,
                                   cps_api_object_t             obj)
{
    cps_api_object_t  cloned;
    t_std_error       ret = STD_ERR_OK;

    STD_ASSERT(param != NULL);
    STD_ASSERT(obj != NULL);

    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_qualifier_t   qual = cps_api_key_get_qual(key);

    if (dn_pas_timedlock() != STD_ERR_OK) {
        PAS_ERR("Not able to acquire the mutex (timeout)");
        return (STD_ERR(PAS, FAIL, 0));
    }

    if(dn_pald_diag_mode_get()) {
         dn_pas_unlock();
         return STD_ERR(PAS, FAIL, 0);
    }

    if ((cloned = cps_api_object_create()) == NULL) {
        dn_pas_unlock();
        return STD_ERR(PAS, NOMEM, 0);
    }
    if (dn_pas_host_system_attr_add(cloned) == false) {
        cps_api_object_delete(cloned);
        dn_pas_unlock();
        return STD_ERR(PAS, NOMEM, 0);
    }
    if (cps_api_object_list_append(param->prev, cloned) == false) {
        cps_api_object_delete(cloned);
        dn_pas_unlock();
        return STD_ERR(PAS, FAIL, 0);
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(cloned),
            BASE_PAS_HOST_SYSTEM_OBJ, qual);

    cps_api_object_attr_t attr = cps_api_object_attr_get(obj,
                                             BASE_PAS_HOST_SYSTEM_BOOTED);
    if (attr != NULL) {
        if (dn_pas_set_host_system_booted() == false) {
            dn_pas_unlock();
            return STD_ERR(PAS, FAIL, 0);
        }
    }

    attr = cps_api_object_attr_get(obj,
                          BASE_PAS_HOST_SYSTEM_SOFTWARE_REV);
    if (attr != NULL) {
        if (dn_pas_comm_dev_host_sw_version_set(
                    cps_api_object_attr_data_bin(attr)) == false) {
            dn_pas_unlock();
            return STD_ERR(PAS, FAIL, 0);
        }
    }

    dn_pas_unlock();
    return ret;
}
