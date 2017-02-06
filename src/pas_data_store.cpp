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

/**************************************************************************
 * @file pas_data_store.cpp
 *
 * @brief This file contains the API's for accessing the Data store
 **************************************************************************/
#include <map>
#include <string>

#include "private/pas_data_store.h"
#include "private/pas_log.h"

#include "cps_api_key.h"


typedef std::map<std::string, void *> pas_res_map_t;
typedef std::pair<std::string, void *> pas_res_pair_t;
typedef std::pair<pas_res_map_t::iterator,bool> pas_res_return_t;


static pas_res_map_t res_map;

enum {
    /* Offset to ignore qualifier (single digit plus period)
       in the printable-string of an OID.
    */
    IGNORE_QUALIFIER_OFS = 2
};

/*
 * dn_pas_res_insertc is to insert the data in data store.
 *
 * INPUT: 1. key of the resource instance
 *        2. Pointer to an resource data structure.
 *
 * Return value: true on success and false on failure.
 */

bool dn_pas_res_insertc (const char *key, void *p_res_obj)
{
    pas_res_map_t::iterator it;

    it = res_map.find(key);

    if (it != res_map.end()) {
        PAS_ERR("Already present, key %s", key);

        return false;
    }

    pas_res_return_t ret;

    ret = res_map.insert (pas_res_pair_t(key, p_res_obj));

    if (ret.second == false) {
        PAS_ERR("Insert failed, key %s", key);

        return false;
    }

    return true;
}

/*
 * dn_pas_res_removec function is to remove resource data and the key
 * from data store
 *
 * INPUT: 1. key of the resource instance
 *
 * Return value: Returns pointer to resource data on success 
 *               otherwise NULL.
 */

void *dn_pas_res_removec (const char *key)
{
    pas_res_map_t::iterator it;
    void                    *p_res_obj;

    it = res_map.find(key);

    if (it == res_map.end()) {
        return NULL;
    }

    p_res_obj = it->second;

    res_map.erase(it);

    return p_res_obj;
}

/*
 * dn_pas_res_getc function is to get resource data from data store
 *
 * INPUT: key of the resource instance
 *
 * Return value: Pointer to resource data record, or 0 if not found
 *
 */

void *dn_pas_res_getc (const char *key)
{
    pas_res_map_t::iterator it = res_map.find(key);
    
    return (it == res_map.end() ? 0 : it->second);
}
