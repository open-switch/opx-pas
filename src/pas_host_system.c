/*
 * Copyright (c) 2017 Dell Inc.
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
 * @file pas_comm_dev.c
 *
 * @brief This file contains source code of host-system handling.
 **************************************************************************/


#include "private/pas_log.h"
#include "private/pas_comm_dev.h"
#include "private/pas_host_system.h"
#include "private/pald.h"
#include "private/dn_pas.h"
#include "private/pas_event.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "sdi_host_system.h"
#include "cps_api_operation.h"
#include "cps_api_service.h"
#include "cps_api_events.h"
#include "cps_class_map.h"
#include <stdlib.h>
#include "std_utils.h"

/* Table to hold the sdi handle */
static pas_host_system_t host_system;

/*
 * dn_host_system_rec_get is used to get host-system data
 */

pas_host_system_t * dn_host_system_rec_get()
{
    if (host_system.sdi_host_system_hdl == NULL) {
        return NULL;
    } else {
        return &host_system;
    }
}

/*
 * Call back function to learn the host system resource handles.
 */

static void dn_pas_host_system_resource_cb (sdi_resource_hdl_t hdl, void *user_data)
{
    STD_ASSERT(hdl != NULL);

    if (sdi_resource_type_get(hdl) == SDI_RESOURCE_HOST_SYSTEM) {
        host_system.sdi_host_system_hdl = hdl;
        host_system.present = true;
    }
    return;
}


/*
 * PAS host_system module initialization
 */

bool dn_pas_host_system_init (void)
{
    sdi_entity_hdl_t        entity_hdl;

    if((entity_hdl = sdi_entity_lookup(SDI_ENTITY_SYSTEM_BOARD, 1)) == NULL) {
        PAS_ERR("SDI Entity lookup failed for system board in host-system init");
        return false;
    }

    sdi_entity_for_each_resource(entity_hdl, dn_pas_host_system_resource_cb,
                NULL);

    return true;
}

/*
 * dn_pas_set_host_system_booted is used to set booted bit 
 * to inform the peer about the status of host-system
 */

bool dn_pas_set_host_system_booted(void)
{
    pas_host_system_t *host_system_rec = NULL;

    if ((host_system_rec = dn_host_system_rec_get()) == NULL) {
        PAS_ERR("Invalid fred ");
        return false;
    }

    if (sdi_host_system_booted_set(host_system_rec->sdi_host_system_hdl,
                true) != STD_ERR_OK) {
        PAS_ERR("Setting Host System booted failed");
        return false;
    }
    host_system_rec->booted = true;

    if (dn_pas_comm_dev_platform_info_get() == false) {
        PAS_ERR("Reading platform info failed");
        return false;
    }
    host_system_rec->valid = true;

    cps_api_object_t host_obj = cps_api_object_create();
    if (host_obj != NULL) {

        if (dn_pas_host_system_attr_add(host_obj) == false) {

            PAS_ERR("Adding host-system cps attr failed");
        } else {

            cps_api_key_from_attr_with_qual(cps_api_object_key(host_obj),
                    BASE_PAS_HOST_SYSTEM_OBJ, cps_api_qualifier_TARGET);

            dn_pas_cps_notify_qual(host_obj, cps_api_qualifier_TARGET);
        }
    } else {
        PAS_ERR("CPS object create failed to publish host-system object");
    }

    cps_api_object_t comm_obj = cps_api_object_create();
    if (comm_obj != NULL) {

        if (dn_pas_comm_dev_attr_add(comm_obj) == false) {

            PAS_ERR("Adding comm-dev cps-attr failed");
        } else {

            dn_pas_comm_dev_notify(comm_obj);
        }
    } else {
        PAS_ERR("CPS object create failed to publish comm-dev object");
    }

    return true;


}

/*
 * dn_pas_host_system_attr_add is used to add host-system cps attributes
 * to a object.
 */

bool dn_pas_host_system_attr_add (cps_api_object_t obj)
{
    STD_ASSERT(obj != NULL);

    pas_host_system_t *rec = dn_host_system_rec_get();
    if (rec == NULL) {
        PAS_ERR("Host system not initialized");
        return false;
    }

    if (cps_api_object_attr_add_u8(obj, BASE_PAS_HOST_SYSTEM_BOOTED,
                rec->booted) == false) {
        PAS_ERR("Adding BASE_PAS_HOST_SYSTEM_BOOTED attribute failed");
        return false;
    }

    if (cps_api_object_attr_add_u32(obj, BASE_PAS_HOST_SYSTEM_SLOT_NUMBER,
                rec->slot_occupation) == false) {
        PAS_ERR("Adding BASE_PAS_HOST_SYSTEM_SLOT_NUMBER attribute failed");
        return false;
    }

    if (cps_api_object_attr_add(obj, BASE_PAS_HOST_SYSTEM_SOFTWARE_REV,
                rec->software_rev, sizeof(rec->software_rev)) == false) {
        PAS_ERR("Adding BASE_PAS_HOST_SYSTEM_SOFTWARE_REV attribute failed");
        return false;
    }
    return true;
}
