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
 *
 * Implemenation of PAS shared lib, per API defined in dn_pas.h
 */

#include "private/pas_utils.h"
#include "private/pas_fuse_main.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"     
#include "std_utils.h"
#include "cps_api_key.h"
#include "cps_api_object_key.h"
#include "cps_api_object.h"
#include "cps_class_map.h"
#include "dell-base-pas.h"
#include "dn_platform_utils.h"


/* \todo dn_pas_slot will be deleted once the chassis manager stops using them */

bool dn_pas_slot(uint_t *slot)
{
    return dn_platform_unit_id_get(slot);
}

/* Get object qualifier and instance fields for an pas status object */

void dn_pas_obj_key_pas_status_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *slot_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type nor slot given */

        return;
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_READY_SLOT);
    if ((*slot_valid = (a != 0))) {

        *slot = cps_api_object_attr_data_uint(a);
    }
}

/* Set object qualifier and instance fields for an pas status object */

void dn_pas_obj_key_pas_status_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_READY_OBJ,
                                    qual
                                    );

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_READY_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_READY_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}


/* Get object qualifier and instance fields for an entity object */

void dn_pas_obj_key_entity_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid = *slot_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type nor slot given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_ENTITY_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_ENTITY_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for an entity object */

void dn_pas_obj_key_entity_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_ENTITY_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj, BASE_PAS_ENTITY_ENTITY_TYPE, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_ENTITY_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_ENTITY_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_ENTITY_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a PSU object */

void dn_pas_obj_key_psu_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *slot_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Slot not given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_PSU_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a psu object */

void dn_pas_obj_key_psu_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                             )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_PSU_OBJ,
                                    qual
                                    );

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_PSU_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_PSU_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a fan tray object */

void dn_pas_obj_key_fan_tray_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *slot_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Slot not given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_FAN_TRAY_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a fan tray object */

void dn_pas_obj_key_fan_tray_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                             )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_FAN_TRAY_OBJ,
                                    qual
                                    );

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_FAN_TRAY_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_FAN_TRAY_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a card object */

void dn_pas_obj_key_card_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *slot_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Slot not given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_CARD_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a card object */

void dn_pas_obj_key_card_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot
                             )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_CARD_OBJ,
                                    qual
                                    );

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_CARD_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_CARD_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a chassis object */

void dn_pas_obj_key_chassis_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    uint8_t             *reboot_type
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *qual = cps_api_key_get_qual(key);

    a = cps_api_object_attr_get(obj, BASE_PAS_CHASSIS_REBOOT);
    if (a != 0) {
        *reboot_type = cps_api_object_attr_data_u8(a);
    }
}


/* Set object qualifier and instance fields for a chassis object */

void dn_pas_obj_key_chassis_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    uint8_t             reboot_type
                             )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_CHASSIS_OBJ,
                                    qual
                                    );

    cps_api_object_attr_add_u8(obj, BASE_PAS_CHASSIS_REBOOT, reboot_type);
}

/* Get object qualifier and instance fields for a fan object */

void dn_pas_obj_key_fan_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *fan_idx_valid,
    uint_t              *fan_idx
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid = *slot_valid = *fan_idx_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type, slot nor fan index given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_FAN_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_FAN_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_FAN_FAN_INDEX);
    if ((*fan_idx_valid = (a != 0))) {
        *fan_idx = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a fan object */

void dn_pas_obj_key_fan_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                fan_idx_valid,
    uint_t              fan_idx
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_FAN_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj, BASE_PAS_FAN_ENTITY_TYPE, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_FAN_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_FAN_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_FAN_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (fan_idx_valid) {
        uint8_t temp = fan_idx;

        cps_api_object_attr_add_u8(obj, BASE_PAS_FAN_FAN_INDEX, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_FAN_FAN_INDEX,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a power monitor object */

void dn_pas_obj_key_pm_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *entity_type_valid,
    uint_t              *entity_type,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *pm_idx_valid,
    uint_t              *pm_idx
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid = *slot_valid = *pm_idx_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type, slot nor power monitor index given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_POWER_MONITOR_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_POWER_MONITOR_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_POWER_MONITOR_MONITOR_INDEX);
    if ((*pm_idx_valid = (a != 0))) {
        *pm_idx = cps_api_object_attr_data_u8(a);
    }
}
/* Set object qualifier and instance fields for a power-monitor object */

void dn_pas_obj_key_pm_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                pm_idx_valid,
    uint_t              pm_idx
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_POWER_MONITOR_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj, BASE_PAS_POWER_MONITOR_ENTITY_TYPE, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_POWER_MONITOR_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_POWER_MONITOR_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_POWER_MONITOR_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (pm_idx_valid) {
        uint8_t temp = pm_idx;

        cps_api_object_attr_add_u8(obj, BASE_PAS_POWER_MONITOR_MONITOR_INDEX, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_POWER_MONITOR_MONITOR_INDEX,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for an LED object */

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
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid = *slot_valid = *led_name_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type, slot nor LED name given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_LED_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_LED_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_LED_NAME);
    if ((*led_name_valid = (a != 0))) {
        safestrncpy(led_name_buf,
                    (char *) cps_api_object_attr_data_bin(a),
                    led_name_buf_size
                    );
    }
}

/* Set object qualifier and instance fields for an LED object */

void dn_pas_obj_key_led_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                led_name_valid,
    char                *led_name
                            )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_LED_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj, BASE_PAS_LED_ENTITY_TYPE, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_LED_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_LED_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_LED_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (led_name_valid) {
        cps_api_object_attr_add_str(obj, BASE_PAS_LED_NAME, led_name);
        cps_api_set_key_data(obj,
                             BASE_PAS_LED_NAME,
                             cps_api_object_ATTR_T_BIN,
                             led_name, strlen(led_name) + 1
                             );
    }
}

/* Get object qualifier and instance fields for a display object */

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
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid = *slot_valid = *disp_name_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type, slot nor LED name given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_DISPLAY_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_DISPLAY_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_DISPLAY_NAME);
    if ((*disp_name_valid = (a != 0))) {
        safestrncpy(disp_name_buf,
                    (char *) cps_api_object_attr_data_bin(a),
                    disp_name_buf_size
                    );
    }
}

/* Set object qualifier and instance fields for a display object */

void dn_pas_obj_key_display_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                disp_name_valid,
    char                *disp_name
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_DISPLAY_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj, BASE_PAS_DISPLAY_ENTITY_TYPE, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_DISPLAY_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_DISPLAY_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_DISPLAY_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (disp_name_valid) {
        cps_api_object_attr_add_str(obj, BASE_PAS_DISPLAY_NAME, disp_name);
        cps_api_set_key_data(obj,
                             BASE_PAS_DISPLAY_NAME,
                             cps_api_object_ATTR_T_BIN,
                             disp_name, strlen(disp_name) + 1
                             );
    }
}

/* Get object qualifier and instance fields for a temperature object */

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
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid = *slot_valid = *sensor_name_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type, slot nor sensor index given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_TEMPERATURE_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_TEMPERATURE_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_TEMPERATURE_NAME);
    if ((*sensor_name_valid = (a != 0))) {
        safestrncpy(sensor_name_buf,
                    (char *) cps_api_object_attr_data_bin(a),
                    sensor_name_buf_size
                    );
    }
}

/* Set object qualifier and instance fields for a temperature object */

void dn_pas_obj_key_temperature_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                entity_type_valid,
    uint_t              entity_type,
    bool                slot_valid,
    uint_t              slot,
    bool                sensor_name_valid,
    char                *sensor_name
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_TEMPERATURE_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj, BASE_PAS_TEMPERATURE_ENTITY_TYPE, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMPERATURE_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_TEMPERATURE_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMPERATURE_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (sensor_name_valid) {
        cps_api_object_attr_add_str(obj, BASE_PAS_TEMPERATURE_NAME, sensor_name);
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMPERATURE_NAME,
                             cps_api_object_ATTR_T_BIN,
                             sensor_name, strlen(sensor_name) + 1
                             );
    }
}

/* Get object qualifier and instance fields for a temperature threshold object */

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
                                    )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *entity_type_valid
        = *slot_valid
        = *sensor_name_valid
        = *thresh_idx_valid
        = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither entity type, slot nor sensor index given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_TEMP_THRESHOLD_ENTITY_TYPE);
    if ((*entity_type_valid = (a != 0))) {
        *entity_type = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_TEMP_THRESHOLD_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_TEMP_THRESHOLD_NAME);
    if ((*sensor_name_valid = (a != 0))) {
        strncpy(sensor_name_buf, 
                (char *) cps_api_object_attr_data_bin(a),
                sensor_name_buf_size
                );
    }

    a = cps_api_get_key_data(obj, BASE_PAS_TEMP_THRESHOLD_THRESHOLD_INDEX);
    if ((*thresh_idx_valid = (a != 0))) {
        *thresh_idx = cps_api_object_attr_data_uint(a);
    }
}

/* Set object qualifier and instance fields for a temperature object */

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
                                    )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_TEMP_THRESHOLD_OBJ,
                                    qual
                                    );

    if (entity_type_valid) {
        uint8_t temp = entity_type;

        cps_api_object_attr_add_u8(obj,
                                   BASE_PAS_TEMP_THRESHOLD_ENTITY_TYPE,
                                   temp
                                   );
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMP_THRESHOLD_ENTITY_TYPE,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj,
                                   BASE_PAS_TEMP_THRESHOLD_SLOT,
                                   temp
                                   );
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMP_THRESHOLD_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (sensor_name_valid) {
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_TEMP_THRESHOLD_NAME,
                                    sensor_name
                                    );
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMP_THRESHOLD_NAME,
                             cps_api_object_ATTR_T_BIN,
                             sensor_name, strlen(sensor_name) + 1
                             );
    }
    
    if (thresh_idx_valid) {
        uint8_t temp = thresh_idx;
        
        cps_api_object_attr_add_u8(obj,
                                   BASE_PAS_TEMP_THRESHOLD_THRESHOLD_INDEX,
                                   temp
                                   );
        cps_api_set_key_data(obj,
                             BASE_PAS_TEMP_THRESHOLD_THRESHOLD_INDEX,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a media object */

void dn_pas_obj_key_media_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *slot_valid,
    uint_t              *slot,
    bool                *port_module_valid,
    uint_t              *port_module,
    bool                *port_valid,
    uint_t              *port
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;
    
    *slot_valid = *port_module_valid = *port_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither slot, port module nor port given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }
    
    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_PORT_MODULE_SLOT);
    if ((*port_module_valid = (a != 0))) {
        *port_module = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_PORT);
    if ((*port_valid = (a != 0))) {
        *port = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a media object */

void dn_pas_obj_key_media_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                slot_valid,
    uint_t              slot,
    bool                port_module_valid,
    uint_t              port_module,
    bool                port_valid,
    uint_t              port
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_MEDIA_OBJ,
                                    qual
                                    );

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_MEDIA_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (port_module_valid) {
        uint8_t temp = port_module;

        cps_api_object_attr_add_u8(obj, BASE_PAS_MEDIA_PORT_MODULE_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_PORT_MODULE_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (port_valid) {
        uint8_t temp = port;

        cps_api_object_attr_add_u8(obj, BASE_PAS_MEDIA_PORT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_PORT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a media channel object */

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
        )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;

    *slot_valid = *port_module_valid = *port_valid = *channel_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither slot, port module, port nor channel given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_CHANNEL_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_CHANNEL_PORT_MODULE_SLOT);
    if ((*port_module_valid= (a != 0))) {
        *port_module = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_CHANNEL_PORT);
    if ((*port_valid = (a != 0))) {
        *port = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_CHANNEL_CHANNEL);
    if ((*channel_valid = (a != 0))) {
        *channel = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a media channel object */

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
        )
{

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_MEDIA_CHANNEL_OBJ,
                                    qual
                                    );

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_object_attr_add_u8(obj, BASE_PAS_MEDIA_CHANNEL_SLOT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_CHANNEL_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (port_module_valid) {
        uint8_t temp = port_module;

        cps_api_object_attr_add_u8(obj,
                BASE_PAS_MEDIA_CHANNEL_PORT_MODULE_SLOT, temp);

        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_CHANNEL_PORT_MODULE_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (port_valid) {
        uint8_t temp = port;

        cps_api_object_attr_add_u8(obj, BASE_PAS_MEDIA_CHANNEL_PORT, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_CHANNEL_PORT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (channel_valid) {
        uint8_t temp = channel;

        cps_api_object_attr_add_u8(obj, BASE_PAS_MEDIA_CHANNEL_CHANNEL, temp);
        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_CHANNEL_CHANNEL,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}

/* Get object qualifier and instance fields for a media config object */

void dn_pas_obj_key_media_config_get(
    cps_api_object_t    obj,
    cps_api_qualifier_t *qual,
    bool                *node_id_valid,
    uint_t              *node_id,
    bool                *slot_valid,
    uint_t              *slot
                               )
{
    cps_api_key_t         *key = cps_api_object_key(obj);
    cps_api_object_attr_t a;

    *slot_valid = *node_id_valid = false;

    *qual = cps_api_key_get_qual(key);

    if (cps_api_key_get_len(key) <= CPS_OBJ_KEY_APP_INST_POS) {
        /* Neither slot, port module nor port given */

        return;
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_CONFIG_NODE_ID);
    if ((*node_id_valid = (a != 0))) {
        *node_id = cps_api_object_attr_data_u8(a);
    }

    a = cps_api_get_key_data(obj, BASE_PAS_MEDIA_CONFIG_SLOT);
    if ((*slot_valid = (a != 0))) {
        *slot = cps_api_object_attr_data_u8(a);
    }
}

/* Set object qualifier and instance fields for a media config object */

void dn_pas_obj_key_media_config_set(
    cps_api_object_t    obj,
    cps_api_qualifier_t qual,
    bool                node_id_valid,
    uint_t              node_id,
    bool                slot_valid,
    uint_t              slot
                               )
{
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_MEDIA_CONFIG_OBJ,
                                    qual
                                    );

    if (node_id_valid) {
        uint8_t temp = node_id;

        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_CONFIG_NODE_ID,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }

    if (slot_valid) {
        uint8_t temp = slot;

        cps_api_set_key_data(obj,
                             BASE_PAS_MEDIA_CONFIG_SLOT,
                             cps_api_object_ATTR_T_BIN,
                             &temp, sizeof(temp)
                             );
    }
}
