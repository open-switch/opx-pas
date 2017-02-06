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

/*********************************************************************
 * @file pas_res_structs.h
 * @brief This file contains the structure definitions of all the
 *        system resources.
 *
 ********************************************************************/

#ifndef __PAS_RES_STRUCTS_H
#define __PAS_RES_STRUCTS_H


#include "dell-base-pas.h"
#include "dell-base-common.h"
#include "dell-base-interface-common.h"
#include "dell-base-platform-common.h"
#include "sdi_media.h"
#include "sdi_entity_info.h"
#include "private/pas_config.h"

#include <time.h>
#include <stdio.h>

/*
 * Set of common macros to handle the length of struct fields
 * which will be used for accessing the data.
 *
 */

enum {
    PAS_NAME_LEN_MAX          = NAME_MAX,
    PAS_HW_REV_LEN            = SDI_HW_REV_LEN,
    PAS_PPID_LEN              = SDI_PPID_LEN,
    PAS_MAC_ADDRESS_LEN       = SDI_MAC_ADDR_LEN,
    PAS_MEDIA_TRANSCEIVER_LEN = (sizeof(sdi_media_transceiver_descr_t)),
    PAS_MEDIA_VENDOR_SPFC_LEN = 32,
    PAS_RES_KEY_SIZE          = 128
};


/* EEPROM fields */

typedef struct _pas_eeprom_t {
    char                         vendor_name [PAS_NAME_LEN_MAX];
    char                         product_name [PAS_NAME_LEN_MAX];
    char                         part_number [PAS_NAME_LEN_MAX];
    char                         hw_version [PAS_HW_REV_LEN];
    char                         platform_name [PAS_NAME_LEN_MAX];
    char                         ppid [PAS_PPID_LEN];
    char                         service_tag [PAS_NAME_LEN_MAX];
} pas_eeprom_t;

/* Operational state and fault code */

typedef struct {
    BASE_CMN_OPER_STATUS_TYPE_t  oper_status;
    PLATFORM_FAULT_TYPE_t        fault_type;
} pas_oper_fault_state_t;


/*
 * pas_chassis_t structure is to hold chassis attributes
 * and it will be used to cache the data in data store for
 * further access.
 */

typedef struct _pas_chassis_t {
    sdi_resource_hdl_t          sdi_entity_info_hdl; /* SDI entity info handle */
    bool                        valid;          /* Contents valid flag */
    pas_oper_fault_state_t      oper_fault_state[1];
    pas_eeprom_t                eeprom[1];      /* Chassis EEPROM contents */
    uint_t                      num_mac_addresses;
    uint8_t                     base_mac_addresses [PAS_MAC_ADDRESS_LEN];
    uint_t                      power_usage;    /* Total power consumed by chassis */
    bool                        power_off;      /* Kill power to chassis */
    uint_t                      active_rpm_slot;/* Slot of active RPM */
    uint8_t                     reboot_type;    /* 1 for Cold reboot, 2 for Warm reboot */
} pas_chassis_t;

static inline const char *dn_pas_res_key_chassis(void)
{
    return ("chassis");
}

/*
 * pas_entity_t structure is to hold common attributes
 * of the entity and it will be used to cache the data
 * in data store for further access.
 */

typedef struct _pas_entity_t {
    uint_t                       entity_type;
    uint_t                       slot;
    sdi_entity_hdl_t             sdi_entity_hdl;      /* SDI entity handle */
    sdi_resource_hdl_t           sdi_entity_info_hdl; /* SDI entity info handle */
    bool                         valid;          /* Contents valid flag */
    char                         name[PAS_NAME_LEN_MAX];
    bool                         present;
    bool                         power_status;
    uint64_t                     insertion_cnt;
    time_t                       insertion_timestamp;
    uint_t                       init_fail_cnt;
    bool                         init_ok;
    uint_t                       fault_cnt;
    BASE_CMN_ADMIN_STATUS_TYPE_t admin_status;
    pas_oper_fault_state_t       oper_fault_state[1];
    pas_eeprom_t                 eeprom[1]; /* Entity EEPROM contents */
    bool                         eeprom_valid;
    bool                         power_on;
    uint_t                       num_fans;
    uint_t                       num_leds;
    uint_t                       num_displays;
    uint_t                       num_temp_sensors;
    uint_t                       num_plds;
    uint8_t                      reboot_type;    /* 1 for Cold reboot, 2 for Warm reboot */
} pas_entity_t;

static inline const char *dn_pas_res_key_entity(
    char   *buf,
    uint_t buf_size,
    uint_t entity_type,
    uint_t slot
                                                )
{
    snprintf(buf, buf_size, "entity.%u.%u", entity_type, slot);
    return ((const char *) buf);
}

/*
 * pas_psu_t structure is to hold PSU attributes and it will be
 * used to cache the data in data store for further access.
 */

typedef struct _pas_psu_t {
    pas_entity_t                  *parent;        /* Parent entity */
    bool                          valid;          /* Contents valid flag */
    PLATFORM_INPUT_POWER_TYPE_t   input_type;
    PLATFORM_FAN_AIRFLOW_TYPE_t   fan_airflow_type;
} pas_psu_t;

static inline const char *dn_pas_res_key_psu(char *buf, uint_t buf_size, uint_t slot)
{
    snprintf(buf, buf_size, "psu.%u", slot);
    return ((const char *) buf);
}

/*
 * pas_fan_tray_t structure is to hold fan tray attributes and
 * it will be used to cache the data in data store for further
 * access.
 */


typedef struct _pas_fan_tray_t {
    pas_entity_t                *parent;        /* Parent entity */
    bool                        valid;          /* Contents valid flag */
    PLATFORM_FAN_AIRFLOW_TYPE_t fan_airflow_type;
} pas_fan_tray_t;

static inline const char *dn_pas_res_key_fan_tray(char *buf, uint_t buf_size, uint_t slot)
{
    snprintf(buf, buf_size, "fan-tray.%u", slot);
    return ((const char *) buf);
}

/*
 * pas_card_t structure is to hold card attributes and
 * it will be used to cache the data in data store for further
 * access.
 */


typedef struct _pas_card_t {
    pas_entity_t     *parent;        /* Parent entity */
    bool             valid;          /* Contents valid flag */
    uint_t           card_type;
} pas_card_t;

static inline const char *dn_pas_res_key_card(char *buf, uint_t buf_size, uint_t slot)
{
    snprintf(buf, buf_size, "card.%u", slot);
    return ((const char *) buf);
}


/*
 * pas_fan_t structure is to hold fan resource attributes and it
 * will be used to cache the data in data store for further access.
 */

typedef struct _pas_fan_t {
    pas_entity_t           *parent;          /* Parent entity */
    uint_t                 fan_idx;
    sdi_resource_hdl_t     sdi_resource_hdl; /* SDI resource handle */
    bool                   valid;            /* Contents valid flag */
    pas_oper_fault_state_t oper_fault_state[1];
    uint_t                 fault_cnt;
    uint_t                 targ_speed, obs_speed;
    bool                   speed_control_en; /* Enable speed control */
    uint_t                 speed_err_margin; /* In % */
    struct {
        uint_t incr, decr, limit, sum;
    } speed_err_integ;
    bool                   speed_err;
    uint_t                 max_speed;
} pas_fan_t;

static inline const char *dn_pas_res_key_fan(
    char   *buf,
    uint_t buf_size,
    uint_t entity_type,
    uint_t slot,
    uint_t fan_idx
        )
{
    snprintf(buf, buf_size, "fan.%u.%u.%u", entity_type, slot, fan_idx);
    return ((const char *) buf);
}


/*
 * pas_led_t structure is to hold LED resource attributes and it
 * will be used to cache the data in data store for further access.
 */

typedef struct _pas_led_t {
    pas_entity_t           *parent;          /* Parent entity */
    uint_t                 led_idx;
    sdi_resource_hdl_t     sdi_resource_hdl; /* SDI resource handle */
    char                   name[PAS_NAME_LEN_MAX];
    pas_oper_fault_state_t oper_fault_state[1];
    bool                   req_on, sdi_on_valid, sdi_on;
} pas_led_t;

static inline const char *dn_pas_res_key_led_idx(
    char   *buf,
    uint_t buf_size,
    uint_t entity_type,
    uint_t slot,
    uint_t led_idx
        )
{
    snprintf(buf, buf_size, "led-idx.%u.%u.%u", entity_type, slot, led_idx);
    return ((const char *) buf);
}

static inline const char *dn_pas_res_key_led_name(
    char       *buf,
    uint_t     buf_size,
    uint_t     entity_type,
    uint_t     slot,
    const char *led_name
        )
{
    snprintf(buf, buf_size, "led-name.%u.%u.%s", entity_type, slot, led_name);
    return ((const char *) buf);
}

/*
 * pas_display_t structure is to hold display attributes and it
 * will be used to cache the data in data store for further access.
 */

typedef struct _pas_display_t {
    pas_entity_t           *parent;          /* Parent entity */
    uint_t                 disp_idx;
    sdi_resource_hdl_t     sdi_resource_hdl; /* SDI resource handle */
    char                   name[PAS_NAME_LEN_MAX];
    pas_oper_fault_state_t oper_fault_state[1];
    char                   *mesg;
} pas_display_t;

static inline const char *dn_pas_res_key_disp_idx(
    char   *buf,
    uint_t buf_size,
    uint_t entity_type,
    uint_t slot,
    uint_t disp_idx
        )
{
    snprintf(buf, buf_size, "disp-idx.%u.%u.%u", entity_type, slot, disp_idx);
    return ((const char *) buf);
}

static inline const char *dn_pas_res_key_disp_name(
    char       *buf,
    uint_t     buf_size,
    uint_t     entity_type,
    uint_t     slot,
    const char *disp_name
        )
{
    snprintf(buf, buf_size, "disp-name.%u.%u.%s", entity_type, slot, disp_name);
    return ((const char *) buf);
}

/*
 * pas_temp_threshold_t structure is to hold temperature threshold
 * attributes and it will be used to cache the data in data store
 * for further access.
 */

typedef struct _pas_temp_threshold_t {
    bool valid;
    int  hi;
    int  lo;
} pas_temp_threshold_t;


/*
 * pas_temperature_sensor_t structure is to hold temperature sensor
 * attributes and it will be used to cache the data in data store
 * for further access.
 */

typedef struct _pas_temperature_sensor_t {
    pas_entity_t           *parent;          /* Parent entity */
    uint_t                 sensor_idx;
    sdi_resource_hdl_t     sdi_resource_hdl; /* SDI resource handle */
    bool                   valid;            /* Contents valid flag */
    char                   name[PAS_NAME_LEN_MAX];
    pas_oper_fault_state_t oper_fault_state[1];
    uint_t                 nsamples;
    int                    prev, cur; /* Previous and current readings */
    uint_t                 range_index;
    int                    shutdown_threshold;
    uint_t                 num_thresh;
    pas_temp_threshold_t   *thresholds;
    bool                   thresh_en;
    struct {
        int  dir;
        int  temperature;
    } last_thresh_crossed[1];
} pas_temperature_sensor_t;

static inline const char *dn_pas_res_key_temp_sensor_idx(
    char   *buf,
    uint_t buf_size,
    uint_t entity_type,
    uint_t slot,
    uint_t sensor_idx
        )
{
    snprintf(buf, buf_size, "temp-sensor-idx.%u.%u.%u", entity_type, slot, sensor_idx);
    return ((const char *) buf);
}

static inline const char *dn_pas_res_key_temp_sensor_name(
    char       *buf,
    uint_t     buf_size,
    uint_t     entity_type,
    uint_t     slot,
    const char *sensor_name
        )
{
    snprintf(buf, buf_size, "temp-sensor-name.%u.%u.%s", entity_type, slot, sensor_name);
    return ((const char *) buf);
}

/*
 * pas_pld_t structure is to hold PLD resource attributes and it
 * will be used to cache the data in data store for further access.
 */

typedef struct _pas_pld_t {
    sdi_resource_hdl_t          sdi_resource_hdl; /* SDI resource handle */
    bool                        valid;            /* Contents valid flag */
    char                        name[PAS_NAME_LEN_MAX];
    pas_oper_fault_state_t      oper_fault_state[1];
    char                        version [PAS_HW_REV_LEN];
} pas_pld_t;



/*
 * pas_port_module_t structure is to hold port module attributes
 * and it will be used to cache the data in data store for
 * further access.
 */

typedef struct _pas_port_module_t {
    bool                         present;
    uint64_t                     insertion_cnt;
    uint64_t                     insertion_timestamp;
    BASE_CMN_ADMIN_STATUS_TYPE_t admin_status;
    pas_oper_fault_state_t       oper_fault_state[1];
} pas_port_module_t;


/*
 * pas_media_t structure is to hold physical media resource
 * attributes and it will be used to cache the data in
 * data store for further access.
 */

typedef struct _pas_media_t {
    sdi_resource_hdl_t           sdi_resource_hdl; /* SDI resource handle */
    bool                         valid;            /* Contents valid flag */
    uint_t                       polling_count;
    bool                         present;
    uint64_t                     insertion_cnt;
    uint64_t                     insertion_timestamp;
    BASE_CMN_ADMIN_STATUS_TYPE_t admin_status;
    BASE_CMN_OPER_STATUS_TYPE_t  oper_status;
    PLATFORM_MEDIA_SUPPORT_STATUS_t
                                 support_status;
    PLATFORM_FAULT_TYPE_t        fault_type;
    PLATFORM_MEDIA_CATEGORY_t    category;
    PLATFORM_MEDIA_TYPE_t        type;
    BASE_IF_SPEED_t              capability;
    char                         vendor_id [SDI_MEDIA_MAX_VENDOR_OUI_LEN];
    char                         serial_number [SDI_MEDIA_MAX_VENDOR_SERIAL_NUMBER_LEN];
    bool                         dell_qualified;
    bool                         high_power_mode;
    uint_t                       identifier;
    uint_t                       ext_identifier;
    uint_t                       connector;
    uint8_t                      transceiver [PAS_MEDIA_TRANSCEIVER_LEN];
    uint_t                       device_tech;
    uint_t                       encoding;
    uint_t                       br_nominal;
    uint_t                       rate_identifier;
    uint_t                       length_sfm_km;
    uint_t                       length_sfm;
    uint_t                       length_om2;
    uint_t                       length_om1;
    uint_t                       length_cable;
    uint_t                       length_om3;
    char                         vendor_name [SDI_MEDIA_MAX_VENDOR_NAME_LEN];
    uint_t                       ext_transceiver;
    uint8_t                      vendor_pn [SDI_MEDIA_MAX_VENDOR_PART_NUMBER_LEN];
    char                         vendor_rev [SDI_MEDIA_MAX_VENDOR_REVISION_LEN];
    uint_t                       wavelength;
    uint_t                       wavelength_tolerance;
    uint_t                       max_case_temp;
    uint_t                       br_max;
    uint_t                       br_min;
    uint_t                       sff_8472_compliance;
    uint_t                       cc_base;
    uint_t                       options;
    uint8_t                      date_code [SDI_MEDIA_MAX_VENDOR_DATE_LEN];
    uint_t                       diag_mon_type;
    uint_t                       enhanced_options;
    uint_t                       cc_ext;
    uint8_t                      vendor_specific [PAS_MEDIA_VENDOR_SPFC_LEN];
    bool                         supported_feature_valid;
    sdi_media_supported_feature_t
                                 supported_feature;
    bool                         rate_select_state;
    PLATFORM_POWER_MEASUREMENT_TYPE_t
                                 rx_power_measurement_type;
    double                       temp_high_alarm;
    double                       temp_low_alarm;
    double                       temp_high_warning;
    double                       temp_low_warning;
    double                       voltage_high_alarm;
    double                       voltage_low_alarm;
    double                       voltage_high_warning;
    double                       voltage_low_warning;
    double                       rx_power_high_alarm;
    double                       rx_power_low_alarm;
    double                       rx_power_high_warning;
    double                       rx_power_low_warning;
    double                       bias_high_alarm;
    double                       bias_low_alarm;
    double                       bias_high_warning;
    double                       bias_low_warning;
    double                       tx_power_high_alarm;
    double                       tx_power_low_alarm;
    double                       tx_power_high_warning;
    double                       tx_power_low_warning;
    double                       current_temperature;
    double                       current_voltage;
    PLATFORM_MEDIA_STATUS_t      temp_state;
    PLATFORM_MEDIA_STATUS_t      voltage_state;

    /* config attributes per media */

    float                        target_wavelength; /* user configured wavelength */
} pas_media_t;

static inline const char *dn_pas_res_key_media(char   *buf,
                                               uint_t buf_size,
                                               uint_t slot,
                                               uint_t port
                                               )
{
    snprintf(buf, buf_size, "media.%u.%u", slot, port);
    return ((const char *) buf);
}

enum media_config_bits {
    MEDIA_AUTONEG = 0,
    MEDIA_SUPPORTED_SPEED = 1
};

/*
 * pas_media_channel_t structure is to hold physical media channel
 * attributes and it will be used to cache the data in data store
 * for further access.
 */

typedef struct _pas_media_channel_t {
    sdi_resource_hdl_t           sdi_resource_hdl;
    bool                         state;
    bool                         tgt_state;   /* target state */
    double                       rx_power;
    double                       tx_power;
    double                       tx_bias_current;
    BASE_CMN_OPER_STATUS_TYPE_t  oper_status;
    bool                         rx_loss;
    bool                         tx_loss;
    bool                         tx_fault;
    bool                         tx_disable;
    PLATFORM_MEDIA_STATUS_t      rx_power_state;
    PLATFORM_MEDIA_STATUS_t      tx_power_state;
    PLATFORM_MEDIA_STATUS_t      tx_bias_state;
    bool                         cdr_enable;
    BASE_IF_SPEED_t              speed;
    /* User configured phy config  */
    uint8_t                      is_conf;
    bool                         autoneg;
    uint_t                       supported_speed_count;
    BASE_IF_SPEED_t              supported_speed [MAX_SUPPORTED_SPEEDS];
} pas_media_channel_t;

static inline const char *dn_pas_res_key_media_chan(char   *buf,
                                                    uint_t buf_size,
                                                    uint_t slot,
                                                    uint_t port,
                                                    uint_t channel
                                                    )
{
    snprintf(buf, buf_size, "media-chan.%u.%u.%u", slot, port, channel);
    return ((const char *) buf);
}


#endif  //__PAS_RES_STRUCTS_H
