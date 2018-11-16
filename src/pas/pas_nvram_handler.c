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
 * filename: pas_nvram_handler.c
 */

#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_entity.h"
#include "private/pas_nvram.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "private/pas_event.h"
#include "private/dn_pas.h"

#include "cps_api_errors.h"
#include "cps_api_events.h"
#include "cps_api_object.h"
#include "cps_api_object_key.h"
#include "cps_api_object_tools.h"
#include "cps_api_operation.h"
#include "cps_api_operation_tools.h"
#include "cps_class_map.h"

#include "std_error_codes.h"
#include "std_llist.h"
#include "sdi_nvram.h"
#include "dell-base-common.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/* Append get response object to given get params for given fully-qualified key */

static t_std_error dn_pas_nvram_get1(
    cps_api_get_params_t *param,
    cps_api_qualifier_t  qual,
    pas_nvram_entry_t    *entry
                                   )
{
    cps_api_object_t resp_obj;

    /* Compose response object */
    resp_obj = cps_api_object_create();
    if (resp_obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("Failed to allocate CPS API object");

        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_nvram_set(resp_obj,
                           qual,
                           true, entry->tag
                           );


    cps_api_object_attr_add(resp_obj, BASE_PAS_NVRAM_VALUE,
                            entry->data, entry->length);

    /* Add response object to get response */
    if (!cps_api_object_list_append(param->list, resp_obj)) {
        cps_api_object_delete(resp_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

/* Append get response objects to given get params for given NVRAM instances */

t_std_error dn_pas_nvram_get(cps_api_get_params_t * param, size_t key_idx)
{
    cps_api_object_t         req_obj = cps_api_object_list_get(param->filters, key_idx);
    cps_api_qualifier_t      qual;
    bool                     tag_valid;
    uint64_t                 tag;
    pas_nvram_t              *nvram_rec;

    dn_pas_obj_key_nvram_get(req_obj,
                           &qual,
                           &tag_valid, &tag
                           );

    dn_nvram_init();
    nvram_rec = dn_pas_nvram_rec_get();
    pas_nvram_entry_t *entry;
    std_dll *dll_entry;

    if (!nvram_rec->initialized) {
        return cps_api_ret_code_ERR;
    }

    for (dll_entry = std_dll_getfirst(&nvram_rec->nvram_data); dll_entry;
         dll_entry = std_dll_getnext(&nvram_rec->nvram_data, dll_entry)) {

        entry = (pas_nvram_entry_t *)dll_entry;
        if (tag_valid) {
            if (tag == entry->tag) {
                dn_pas_nvram_get1(param, qual, entry);
                break;
            }
        } else {
            dn_pas_nvram_get1(param, qual, entry);
        }
    }

    return (STD_ERR_OK);
}

/* Set one NVRAM Tag */

static t_std_error dn_pas_nvram_set1(
    cps_api_transaction_params_t *param,
    cps_api_qualifier_t          qual,
    pas_nvram_t                  *rec,
    pas_nvram_entry_t            *entry,
    bool                         tag_valid,
    uint64_t                     tag,
    uint64_t                     length,
    bool                         data_valid,
    uint8_t                      *data
                                   )
{
    cps_api_object_t old_obj;

    /* Add old values, for rollback */

    old_obj = cps_api_object_create();
    if (old_obj == CPS_API_OBJECT_NULL)  return (STD_ERR(PAS, NOMEM, 0));

    dn_pas_obj_key_nvram_set(old_obj,
                           qual,
                           true, entry->tag
                           );

    cps_api_object_attr_add_u64(old_obj,
                               BASE_PAS_NVRAM_TAG,
                               entry->tag
                               );

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    if (!tag_valid)  return (STD_ERR(PAS, FAIL, 0));

    if (!data_valid) {
        std_dll_remove(&rec->nvram_data, (std_dll *)entry);
    } else {
        /* Size limit the data to the maximum capacity for each entry */
        if (length > sizeof(entry->data)) {
            length = sizeof(entry->data);
        }
        entry->length = length;
        memcpy(entry->data, data, length);
    }
    return (STD_ERR_OK);
}

t_std_error dn_pas_nvram_set(cps_api_transaction_params_t *param, cps_api_object_t obj)
{
    cps_api_qualifier_t     qual;
    bool                    tag_valid, data_valid;
    uint64_t                tag, length;
    uint8_t                 *data;
    cps_api_object_attr_t   a;
    pas_nvram_t             *nvram_rec;
    pas_nvram_entry_t       *entry;
    std_dll                 *dll_entry;
    bool                    notif = false;

    if (cps_api_object_type_operation(cps_api_object_key(obj)) != cps_api_oper_SET) {
        return cps_api_ret_code_ERR;
    }

    dn_pas_obj_key_nvram_get(obj,
                           &qual,
                           &tag_valid, &tag
                           );

    if (!tag_valid) {
        return cps_api_ret_code_ERR;
    }

    a = cps_api_object_attr_get(obj, BASE_PAS_NVRAM_VALUE);
    if ((data_valid = (a != CPS_API_ATTR_NULL))) {
        data = cps_api_object_attr_data_bin(a);
        length = cps_api_object_attr_len(a);
    } else {
        data = NULL;
        length = 0;
    }

    dn_nvram_init();
    nvram_rec = dn_pas_nvram_rec_get();
    if (!nvram_rec->initialized) {
        return cps_api_ret_code_ERR;
    }

    entry = NULL;
    for (dll_entry = std_dll_getfirst(&nvram_rec->nvram_data); dll_entry;
         dll_entry = std_dll_getnext(&nvram_rec->nvram_data, dll_entry)) {

        if (((pas_nvram_entry_t *)dll_entry)->tag == tag) {
            entry = (pas_nvram_entry_t *)dll_entry;
            break;
        }
    }

    if (!entry) {
        if (data_valid) {
            /* Allocate a new entry */
            entry = calloc(1, sizeof(*entry));
            if (!entry) {
                return cps_api_ret_code_ERR;
            }

            entry->tag = tag;
            if (length > sizeof(entry->data)) {
                length = sizeof(entry->data);
            }
            entry->length = length;
            memcpy(entry->data, data, length);

            std_dll_insertatback(&nvram_rec->nvram_data, (std_dll *)entry);

            notif = true;
        }
    } else {
        /* Check if data is valid, otherwise, delete the entry */
        notif = (dn_pas_nvram_set1(param, qual, nvram_rec, entry,
                      tag_valid, tag, length, data_valid, data) == STD_ERR_OK);
    }

    /* Write NVRAM */
    dn_nvram_write();

    /* Send a notification */
    if (notif) {
        cps_api_object_t obj;

        obj = cps_api_object_create();
        if (obj == CPS_API_OBJECT_NULL) {
            PAS_ERR("Failed to allocate CPS Event object");

            return (STD_ERR(PAS, NOMEM, 0));
        }

        dn_pas_obj_key_nvram_set(obj, cps_api_qualifier_OBSERVED,
                                 true, tag);
        cps_api_object_attr_add(obj, BASE_PAS_NVRAM_VALUE, data, length);

        /* Object is deleted in dn_pas_cps_notify */
        if (!dn_pas_cps_notify(obj)) {
            PAS_ERR("Failed to send NVRAM notification");

            return (STD_ERR(PAS, FAIL, 0));
        }
    }

    return (STD_ERR_OK);
}

