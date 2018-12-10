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

/**************************************************************************
 * @file pas_comm_dev.c
 *
 * @brief This file contains source code of comm dev handling.
 **************************************************************************/


#include "private/pas_log.h"
#include "private/pas_comm_dev.h"
#include "private/pas_host_system.h"
#include "private/pald.h"
#include "private/dn_pas.h"
#include "private/pas_event.h"
#include "private/pas_utils.h"
#include "private/pas_config.h"
#include "cps_api_operation.h"
#include "cps_api_service.h"
#include "cps_api_events.h"
#include "cps_class_map.h"
#include <stdlib.h>
#include "std_utils.h"

/* Table to hold the sdi handle */

static pas_comm_dev_t comm_dev;

/*
 * dn_comm_dev_rec_get is used to get comm-dev data
 */

pas_comm_dev_t* dn_comm_dev_rec_get(void)
{
    return (comm_dev.sdi_comm_dev_hdl == NULL)
            ? NULL
            : &comm_dev;
}

/*
 * Call back function to learn comm dev resource handles.
 */

static void dn_pas_comm_dev_resource_cb (sdi_resource_hdl_t hdl, void *user_data)
{
    STD_ASSERT(hdl != NULL);

    if (sdi_resource_type_get(hdl) == SDI_RESOURCE_COMM_DEV) {
        comm_dev.sdi_comm_dev_hdl = hdl;
        comm_dev.present = true;
    }

    return;
}

/*
 * PAS comm dev module initialization function.
 */

bool dn_pas_comm_dev_init (void)
{
    sdi_entity_hdl_t        entity_hdl;

    if((entity_hdl = sdi_entity_lookup(SDI_ENTITY_SYSTEM_BOARD, 1)) == NULL) {
        PAS_ERR("SDI Entity lookup failed for system board in comm-dev init");
        return false;
    }

    sdi_entity_for_each_resource(entity_hdl, dn_pas_comm_dev_resource_cb, NULL);

    if (dn_pas_host_system_init() != true) {
        PAS_ERR("host system initialization failed");
        return false;
    }

    dn_pas_comm_dev_messaging_enable(false);
    dn_pas_comm_dev_platform_info_get();

    return true;
}

/*
 * dn_pas_comm_dev_comm_msg_write is used to write a response/notification
 * message to communication device
 */

bool dn_pas_comm_dev_comm_msg_write (uint8_t * data, uint_t size)
{
    pas_comm_dev_t     *cd_rec = dn_comm_dev_rec_get();
    pas_host_system_t  *hs_rec = dn_host_system_rec_get();

    if ((data == NULL) || (size == 0) || (size > SDI_COMM_DEV_BUFFER_MAX_SIZE)) {
        PAS_ERR("Invalid params, data (%u), size (%u)", data, size);
        return false;
    }

    if ((cd_rec == NULL) || (hs_rec == NULL)
            || (hs_rec->booted == false)) {
        PAS_ERR("comm-dev/host-system not initialized or host-system is not booted");
        return false;
    }

    if (sdi_comm_dev_msg_write(cd_rec->sdi_comm_dev_hdl, size, data) != STD_ERR_OK) {
         PAS_ERR("Writing to north bound mailbox failed, %s", data);
         return false;
    }
    PAS_TRACE("COMM-MSG writing to NB mailbox is Success, (%s)", data);
    return true;
}

/*
 * dn_comm_dev_mb_poll is used by pas poller thread to
 * read messages from comm-dev and publish the same.
 */

void dn_comm_dev_poll (void)
{
    pas_comm_dev_t     *comm_rec = dn_comm_dev_rec_get();
    pas_host_system_t  *host_rec = dn_host_system_rec_get();
    bool               present = false;

    if ((comm_rec == NULL) || (host_rec == NULL)
            || (host_rec->booted == false)) {
        PAS_TRACE("comm-dev/host-system not initialized or host-system is not booted");
        return;
    }

    if (sdi_comm_dev_is_msg_present(host_rec->sdi_host_system_hdl,
                &present) != STD_ERR_OK) {
        PAS_ERR("Checking Comm Dev message present failed");
        return;
    }

    if (present == false) return;

    cps_api_object_t   obj = cps_api_object_create();
    if (obj == NULL) {
        PAS_ERR("CPS object allocation failed");
        return;
    }

    char   buff[SDI_COMM_DEV_BUFFER_MAX_SIZE];

    if (sdi_comm_dev_msg_read(comm_rec->sdi_comm_dev_hdl, sizeof(buff),
                (uint8_t *)buff) == STD_ERR_OK) {
        if (cps_api_object_attr_add(obj, BASE_PAS_COMM_DEV_COMM_MSG, buff,
                    strlen(buff) + 1) == false) {
            cps_api_object_delete(obj);
            PAS_ERR("CPS attribute add failed to add BASE_PAS_COMM_DEV_COMM_MSG");
            return;
        }
        PAS_TRACE("COMM-MSG reading from SB mailbox is Success and publishing it, (%s)", buff);
        dn_pas_comm_dev_notify(obj);
    } else {
        cps_api_object_delete(obj);
        PAS_ERR("Comm Dev message read failed from SDI");
        return;
    }
}

/*
 * dn_pas_host_system_slot_get is used to convert slot position to slot number
 * which is provided by driver.
 */

static uint_t dn_pas_host_system_slot_get (uint8_t slot_pos)
{
    uint_t slot_num = 0;

    switch (slot_pos) {
        case 0x1:
            slot_num = 1;
            break;
        case 0x2:
            slot_num = 2;
            break;
        case 0x4:
            slot_num = 3;
            break;
        case 0x8:
            slot_num = 4;
            break;
        default :
            slot_num = 0;
            break;
    }
    return slot_num;
}

bool dn_pas_comm_dev_messaging_enable(bool enable)
{

    pas_comm_dev_t      *comm_dev_rec = dn_comm_dev_rec_get();
    if ((comm_dev_rec == NULL)) {
        return false;
    }

    if (sdi_comm_dev_messaging_enable(comm_dev_rec->sdi_comm_dev_hdl,
                enable) != STD_ERR_OK) {
        PAS_ERR(" Error reading platform comm dev info");
        return false;
    }

    return true;
}

/*
 * dn_pas_comm_dev_platform_info_get is used to get platform info
 * from comm dev device.
 */

bool dn_pas_comm_dev_platform_info_get(void)
{
    pas_comm_dev_t      *comm_dev_rec = dn_comm_dev_rec_get();
    pas_host_system_t   *host_system_rec = dn_host_system_rec_get();
    sdi_platform_info_t platform_info[1];
    char                buff[SDI_COMM_DEV_FW_VERSION_SIZE];

    memset(buff, 0, SDI_COMM_DEV_FW_VERSION_SIZE);
    memset(platform_info, 0, sizeof(platform_info));

    if ((comm_dev_rec == NULL) || (host_system_rec == NULL)) {
        return false;
    }

    if (sdi_comm_dev_platform_info_get(comm_dev_rec->sdi_comm_dev_hdl,
                platform_info) != STD_ERR_OK) {
        PAS_ERR(" Error reading platform comm dev info");
        return false;
    }

    if(!safestrncpy(comm_dev_rec->chasis_service_tag,
                platform_info->service_tag,
                SDI_COMM_DEV_SERVICE_TAG_SIZE)) {
         PAS_ERR( " Error in updating service tag\n");
        return false;
    }

    if(!safestrncpy(comm_dev_rec->comm_dev_firmware_rev,
                platform_info->comm_dev_fw_ver,
                SDI_COMM_DEV_FW_VERSION_SIZE)) {
         PAS_ERR( " Error in updating comm dev FW rev\n");
        return false;
    }

    comm_dev_rec->valid = true;

    host_system_rec->slot_occupation =
        dn_pas_host_system_slot_get(platform_info->slot_occupation);

    switch (host_system_rec->slot_occupation){
        case 1:
            safestrncpy(host_system_rec->fab_id,"A1",PAS_HOST_SYSTEM_FAB_ID_SIZE);
            break;
        case 2:
            safestrncpy(host_system_rec->fab_id,"A2",PAS_HOST_SYSTEM_FAB_ID_SIZE);
            break;
        case 3:
            safestrncpy(host_system_rec->fab_id,"B1",PAS_HOST_SYSTEM_FAB_ID_SIZE);
            break;
        case 4:
            safestrncpy(host_system_rec->fab_id,"B2",PAS_HOST_SYSTEM_FAB_ID_SIZE);
            break;
        default :
            break;
    }

    if (sdi_comm_dev_host_sw_version_get(comm_dev_rec->sdi_comm_dev_hdl,
                (uint8_t *)buff) != STD_ERR_OK) {
        PAS_ERR(" Error reading host_sw_version_get");
        return false;
    }

    if(!safestrncpy(host_system_rec->software_rev, buff,
                SDI_COMM_DEV_FW_VERSION_SIZE)) {
         PAS_ERR( " Error in updating host system sw rev\n");
        return false;
    }
    return true;
}

/*
 * dn_pas_comm_dev_notify is used to publish comm-dev CPS object
 */

bool dn_pas_comm_dev_notify(cps_api_object_t  obj)
{
    STD_ASSERT(obj != NULL);

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
            BASE_PAS_COMM_DEV_OBJ, cps_api_qualifier_TARGET);

    dn_pas_cps_notify_qual(obj,cps_api_qualifier_TARGET);

    return true;
}

/*
 * dn_pas_comm_dev_attr_add is to add/populate comm-dev cps attributes
 * in an object
 */

bool dn_pas_comm_dev_attr_add (cps_api_object_t obj)
{
    STD_ASSERT(obj != NULL);

    pas_comm_dev_t *rec = dn_comm_dev_rec_get();
    if (rec == NULL) {
        PAS_ERR("Comm Dev not initialized");
        return false;
    }

    if (cps_api_object_attr_add(obj, BASE_PAS_COMM_DEV_CHASSIS_SERVICE_TAG,
                rec->chasis_service_tag, SDI_COMM_DEV_SERVICE_TAG_SIZE) == false) {
        PAS_ERR("Adding BASE_PAS_COMM_DEV_CHASSIS_SERVICE_TAG attribute failed");
        return false;
    }

    if (cps_api_object_attr_add(obj, BASE_PAS_COMM_DEV_COMM_DEV_FIRMWARE_REV,
                rec->comm_dev_firmware_rev, SDI_COMM_DEV_FW_VERSION_SIZE) == false) {
        PAS_ERR("Adding  BASE_PAS_COMM_DEV_COMM_DEV_FIRMWARE_REV attribute failed");
        return false;
    }

    return true;
}

/*
 * dn_pas_comm_dev_host_sw_version_set is used to set the host-system
 * software revision in communication device.
 */

bool dn_pas_comm_dev_host_sw_version_set (char *sw_ver)
{
    STD_ASSERT(sw_ver != NULL);

    pas_comm_dev_t    *rec = dn_comm_dev_rec_get();
    pas_host_system_t *hs_rec = dn_host_system_rec_get();
    if ((rec == NULL) || (hs_rec == NULL)
            || (hs_rec->booted == false)) {
        PAS_ERR("Comm Dev/Host system not initialized");
        return false;
    }
    safestrncpy(hs_rec->software_rev, sw_ver,
            sizeof(hs_rec->software_rev));

    if (sdi_comm_dev_host_sw_version_set(rec->sdi_comm_dev_hdl, (uint8_t *) sw_ver)
            != STD_ERR_OK) {
        PAS_ERR("Writing hostsystem SW version to Comm Dev failed");
        return false;
    }

    return true;
}
