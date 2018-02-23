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

/** ************************************************************************
 *
 * \file dn_pas.h
 *
 * API definition for PAS (Platform Abstraction Service) library.
 */

#ifndef __DN_PAS_H
#define __DN_PAS_H

#include "std_type_defs.h"
#include "cps_api_key.h"
#include "dell-base-pas.h"

#ifdef __cplusplus
extern "C" {
#endif

/** ************************************************************************
 *
 * Return current slot number
 *
 * Return the slot number of the card the current CPU is running on.
 * Slot numbers start at 1, and are consecutive, up to a platform-dependent
 * limit.
 */

bool dn_pas_slot(uint_t *slot);

/** ************************************************************************
 *
 * Get object qualifier and instance fields for an status object
 *
 * Function to extract instance information from the given PAS status
 * object.  Slot is extracted, with flags indicating
 * whether or not each was specified in the given object. *
 */

void dn_pas_obj_key_pas_status_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                               );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for an status object
 *
 * Function to set instance information in the given PAS status object.
 * Slot may be given.
 */

void dn_pas_obj_key_pas_status_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for an entity object
 *
 * Function to extract instance information from the given PAS entity
 * object.  Entity type and slot are extracted, with flags indicating
 * whether or not each was specified in the given object.
 */

void dn_pas_obj_key_entity_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot
                               );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for an entity object
 *
 * Function to set instance information in the given PAS entity object.
 * Entity type and slot may be given.
 */

void dn_pas_obj_key_entity_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a PSU object
 *
 * Function to extract instance information from the given PAS PSU
 * object.  Slot is extracted, with flag indicating
 * whether or not it was specified in the given object.
 */

void dn_pas_obj_key_psu_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                            );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a PSU object
 *
 * Function to set instance information in the given PAS PSU object.
 * Slot may be given.
 */

void dn_pas_obj_key_psu_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                            );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a fan tray object
 *
 * Function to extract instance information from the given PAS fan tray
 * object.  Slot is extracted, with flag indicating
 * whether or not it was specified in the given object.
 */

void dn_pas_obj_key_fan_tray_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                                 );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a fan tray object
 *
 * Function to set instance information in the given PAS fan tray object.
 * Slot may be given.
 */

void dn_pas_obj_key_fan_tray_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a card object
 *
 * Function to extract instance information from the given PAS card
 * object.  Slot is extracted, with flag indicating
 * whether or not it was specified in the given object.
 */

void dn_pas_obj_key_card_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                             );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a card object
 *
 * Function to set instance information in the given PAS card object.
 * Slot may be given.
 */

void dn_pas_obj_key_card_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                             );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a chassis object
 *
 * Function to extract instance information from the given PAS chassis
 * object.  Slot is extracted, with flag indicating
 * whether or not it was specified in the given object.
 */

void dn_pas_obj_key_chassis_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    uint8_t             *reboot_type
                                );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a chassis object
 *
 * Function to set instance information in the given PAS chassis object.
 * Slot may be given.
 */

void dn_pas_obj_key_chassis_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    uint8_t             reboot_type
                                );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a fan object
 *
 * Function to extract instance information from the given PAS fan
 * object.  Entity type, slot and fan index are extracted, with flags 
 * indicating whether or not each was specified in the given object.
 */

void dn_pas_obj_key_fan_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *fan_idx_valid,
    uint_t              *fan_idx
                            );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a fan object
 *
 * Function to set instance information in the given PAS fan object.
 * Entity type, slot and fan index may be given.
 */

void dn_pas_obj_key_fan_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                fan_idx_valid,
    uint_t              fan_idx
                            );
/** ************************************************************************
 *
 * Get object qualifier and instance fields for a power monitor object
 *
 * Function to extract instance information from the given PAS power monitor 
 * object.  Entity type, slot and power monitor index are extracted, with flags 
 * indicating whether or not each was specified in the given object.
 */

void dn_pas_obj_key_pm_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *pm_idx_valid,
    uint_t              *pm_idx
                            );


/** ************************************************************************
 *
 * Set object qualifier and instance fields for a power monitor object
 *
 * Function to set instance information in the given PAS power monitor object.
 * Entity type, slot and pm index may be given.
 */

void dn_pas_obj_key_pm_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                pm_idx_valid,
    uint_t              pm_idx
                            );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for an LED object
 *
 * Function to extract instance information from the given PAS LED
 * object.  Entity type, slot and LED name are extracted, with flags 
 * indicating whether or not each was specified in the given object.
 */

void dn_pas_obj_key_led_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *led_name_valid,
    char                *led_name_buf,
    uint_t              led_name_buf_size
                            );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for an LED object
 *
 * Function to set instance information in the given PAS LED object.
 * Entity type, slot and LED name may be given.
 */

void dn_pas_obj_key_led_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                led_name_valid,
    char                *led_name
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a display object
 *
 * Function to extract instance information from the given PAS display
 * object.  Entity type, slot and display name are extracted, with flags 
 * indicating whether or not each was specified in the given object.
 */

void dn_pas_obj_key_display_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *disp_name_valid,
    char                *disp_name_buf,
    uint_t              disp_name_buf_size
                               );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a display object
 *
 * Function to set instance information in the given PAS display object.
 * Entity type, slot and display name may be given.
 */

void dn_pas_obj_key_display_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                disp_name_valid,
    char                *disp_name
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a temperature object
 *
 * Function to extract instance information from the given PAS temperature
 * sensor object.  Entity type, slot and sensor name are extracted, with
 * flags indicating whether or not each was specified in the given object.
 */

void dn_pas_obj_key_temperature_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *sensor_name_valid,
    char                *sensor_name_buf,
    uint_t              sensor_name_buf_size
                               );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a temperature object
 *
 * Function to set instance information in the given PAS temperature sensor
 * object.  Entity type, slot and sensor name may be given.
 */

void dn_pas_obj_key_temperature_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                sensor_name_valid,
    char                *sensor_name
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a temperature threshold object
 *
 * Function to extract instance information from the given PAS temperature
 * threshold object.  Entity type, slot, sensor name and threshold index
 * are extracted, with flags indicating whether or not each was specified
 * in the given object.
 */

void dn_pas_obj_key_temp_thresh_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *sensor_name_valid,
    char                *sensor_name_buf,
    uint_t              sensor_name_buf_size,
    bool                *thresh_idx_valid,
    uint_t              *thresh_idx
                                    );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a temperature threshold object
 *
 * Function to set instance information in the given PAS temperature
 * threshold object.  Entity type, slot, sensor name and threshold index
 * may be given.
 */

void dn_pas_obj_key_temp_thresh_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                sensor_name_valid,
    char                *sensor_name,
    bool                thresh_idx_valid,
    uint_t              thresh_idx
                                    );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a media object
 *
 * Function to extract instance information from the given PAS optical
 * media adapter object.  Slot, port module slot and port number are
 * extracted, with flags indicating whether or not each was specified in the
 * given object.
 */

void dn_pas_obj_key_media_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *port_module_valid,
    uint_t              *port_module,
    bool                *port_valid,
    uint_t              *port
                               );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a media object
 *
 * Function to set instance information in the given PAS optical media
 * adapter object.  Slot, port module slot and port number  may be given.
 */

void dn_pas_obj_key_media_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot,
    bool                port_module_valid,
    uint_t              port_module,
    bool                port_valid,
    uint_t              port
                               );

/** ************************************************************************
 *
 * Get object qualifier and instance fields for a media channel object
 *
 * Function to extract instance information from the given PAS optical
 * media adapter channel object.  Slot, port module slot, port number and
 * channel are extracted, with flags indicating whether or not each was
 * specified in the given object.
 */

void dn_pas_obj_key_media_channel_get(
        cps_api_object_t    obj,
        cps_api_qualifier_t *qual,
        bool                *slot_valid,
        uint_t              *slot,
        bool                *port_module_valid,
        uint_t              *port_module,
        bool                *port_valid,
        uint_t              *port,
        bool                *channel_valid,
        uint_t              *channel
        );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a media channel object
 *
 * Function to set instance information in the given PAS optical media
 * adapter channel object.  Slot, port module slot, port number and channel
 * may be given.
 */

void dn_pas_obj_key_media_channel_set(
        cps_api_object_t    obj,
        cps_api_qualifier_t qual,
        bool                slot_valid,
        uint_t              slot,
        bool                port_module_valid,
        uint_t              port_module,
        bool                port_valid,
        uint_t              port,
        bool                channel_valid,
        uint_t              channel
        );


/** ************************************************************************
 *
 * Get object qualifier and instance fields for a media object
 *
 * Function to extract instance information from the given PAS optical
 * media adapter object.  Slot, port module slot and port number are
 * extracted, with flags indicating whether or not each was specified in the
 * given object.
 */

void dn_pas_obj_key_media_config_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *node_id_valid,
    uint_t              *node_id,
    bool                *slot_valid,
    uint_t              *slot
                               );

/** ************************************************************************
 *
 * Set object qualifier and instance fields for a media object
 *
 * Function to set instance information in the given PAS optical media
 * adapter object.  Slot, port module slot and port number  may be given.
 */

void dn_pas_obj_key_media_config_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                node_id_valid,
    uint_t              node_id,
    bool                slot_valid,
    uint_t              slot
                               );
#ifdef __cplusplus
}
#endif

#endif /* !defined(__DN_PAS_H) */
