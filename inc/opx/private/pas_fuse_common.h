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

/**
 * file : pas_fuse_common.h
 * brief: pas daemon interface layer to SDI API
 * date : 04/2015
 *
 */

#ifndef __PAS_FUSE_COMMON_H
#define __PAS_FUSE_COMMON_H

#include "stdio.h"
#include "std_utils.h"
#include "sdi_entity.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fuse.h>
#include <ctype.h>
#include <inttypes.h>

enum {

    /** max size of FUSE directory pathnames supported */
    FUSE_FUSE_MAX_PATH               = 128,

    /** default size of FUSE directory file */
    FUSE_DIR_DEFAULT_SIZE            = 512,

    /** default number of FUSE directory entries (. and ..) */
    FUSE_DIR_NUM_DEFAULT             = 2,

    /** default mode of FUSE directory type and permissions */
    FUSE_DIR_MODE_DEFAULT            = (S_IFDIR | 0755),

    /** default size of FUSE (non-directory) file */
    FUSE_FILE_DEFAULT_SIZE           = 128,

    /** default number of entries of FUSE (non-directory) file */
    FUSE_FILE_NUM_DEFAULT            = 1,

    /** standard setting of FUSE directory for READ_ONLY file */
    FUSE_FILE_MODE_READ              = (S_IFREG | 0444),

    /** standard setting of FUSE directory for WRITE_ONLY file */
    FUSE_FILE_MODE_WRITE             = (S_IFREG | 0222),

    /** standard setting of FUSE directory for READ/WRITE file */
    FUSE_FILE_MODE_RW                = (S_IFREG | 0666),

    /** default SDI device fuse_filetype is DIRECTORY */
    FUSE_SDI_DEVICE_DIR              = 0,

    /** FUSE diag mode filetype */
    FUSE_DIAG_MODE_FILETYPE          = 0,
};


/** Enum to specify fan filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {

    FUSE_FAN_FILETYPE_SPEED,
    FUSE_FAN_FILETYPE_ALERT,
    FUSE_FAN_FILETYPE_ALIAS,
    FUSE_FAN_FILETYPE_DIAG_MODE,
    FUSE_FAN_FILETYPE_MAX,
    FUSE_FAN_FILETYPE_MIN = FUSE_FAN_FILETYPE_SPEED

} fan_filetype_t;


/** Enum to specify thermal sensor filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {

    FUSE_THERMAL_SENSOR_FILETYPE_TEMPERATURE,
    FUSE_THERMAL_SENSOR_FILETYPE_LOW_THRESHOLD,
    FUSE_THERMAL_SENSOR_FILETYPE_HIGH_THRESHOLD,
    FUSE_THERMAL_SENSOR_FILETYPE_CRITICAL_THRESHOLD,
    FUSE_THERMAL_SENSOR_FILETYPE_ALERT_ON,
    FUSE_THERMAL_SENSOR_FILETYPE_ALIAS,
    FUSE_THERMAL_SENSOR_FILETYPE_DIAG_MODE,
    FUSE_THERMAL_SENSOR_FILETYPE_MAX,
    FUSE_THERMAL_SENSOR_FILETYPE_MIN = FUSE_THERMAL_SENSOR_FILETYPE_TEMPERATURE

} thermal_sensor_filetype_t;


/** Enum to specify led filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {

    FUSE_LED_FILETYPE_STATE,
    FUSE_LED_FILETYPE_ALIAS,
    FUSE_LED_FILETYPE_DIAG_MODE,
    FUSE_LED_FILETYPE_MAX,
    FUSE_LED_FILETYPE_MIN = FUSE_LED_FILETYPE_STATE

} led_filetype_t;


/** Enum to specify display NVRAM filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {

    FUSE_NVRAM_FILETYPE_DATA,
    FUSE_NVRAM_FILETYPE_DIAG_MODE,
    FUSE_NVRAM_FILETYPE_MAX,
    FUSE_NVRAM_FILETYPE_MIN = FUSE_NVRAM_FILETYPE_DATA

} nvram_filetype_t;


/** Enum to specify display LED filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {

    FUSE_DISPLAY_LED_FILETYPE_STRING,
    FUSE_DISPLAY_LED_FILETYPE_STATE,
    FUSE_DISPLAY_LED_FILETYPE_ALIAS,
    FUSE_DISPLAY_LED_FILETYPE_DIAG_MODE,
    FUSE_DISPLAY_LED_FILETYPE_MAX,
    FUSE_DISPLAY_LED_FILETYPE_MIN = FUSE_DISPLAY_LED_FILETYPE_STRING

} display_led_filetype_t;


/** Enum to specify entity_info filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {

    FUSE_ENTITY_INFO_FILETYPE_PROD_NAME,
    FUSE_ENTITY_INFO_FILETYPE_PPID,
    FUSE_ENTITY_INFO_FILETYPE_HW_REVISION,
    FUSE_ENTITY_INFO_FILETYPE_PLATFORM_NAME,
    FUSE_ENTITY_INFO_FILETYPE_VENDOR_NAME,
    FUSE_ENTITY_INFO_FILETYPE_MAC_SIZE,
    FUSE_ENTITY_INFO_FILETYPE_BASE_MAC,
    FUSE_ENTITY_INFO_FILETYPE_NUM_FANS,
    FUSE_ENTITY_INFO_FILETYPE_MAX_SPEED,
    FUSE_ENTITY_INFO_FILETYPE_AIRFLOW,
    FUSE_ENTITY_INFO_FILETYPE_POWER_RATING,
    FUSE_ENTITY_INFO_FILETYPE_ALIAS,
    FUSE_ENTITY_INFO_FILETYPE_DIAG_MODE,
    FUSE_ENTITY_INFO_FILETYPE_MAX,
    FUSE_ENTITY_INFO_FILETYPE_SERVICE_TAG,
    FUSE_ENTITY_INFO_FILETYPE_MIN = FUSE_ENTITY_INFO_FILETYPE_PROD_NAME

} entity_info_filetype_t;


/** Enum to specify media filetypes. Used to redirect FUSE read/write handlers to appropriate SDI calls */
typedef enum {
 
    FUSE_MEDIA_FILETYPE_PRESENCE,
    FUSE_MEDIA_FILETYPE_TEMP_HIGH_ALARM_STATUS,
    FUSE_MEDIA_FILETYPE_TEMP_LOW_ALARM_STATUS,
    FUSE_MEDIA_FILETYPE_TEMP_HIGH_WARNING_STATUS,
    FUSE_MEDIA_FILETYPE_TEMP_LOW_WARNING_STATUS,
    FUSE_MEDIA_FILETYPE_VOLT_HIGH_ALARM_STATUS,
    FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_STATUS,
    FUSE_MEDIA_FILETYPE_VOLT_HIGH_WARNING_STATUS,
    FUSE_MEDIA_FILETYPE_VOLT_LOW_WARNING_STATUS,

    /** channel 0..3 */
    FUSE_MEDIA_FILETYPE_TX_DISABLE_STATUS,
    FUSE_MEDIA_FILETYPE_TX_FAULT_STATUS,
    FUSE_MEDIA_FILETYPE_TX_LOSS_STATUS,
    FUSE_MEDIA_FILETYPE_RX_LOSS_STATUS,

    /** media transmission 0..3 */
    FUSE_MEDIA_FILETYPE_TX_CONTROL,

    FUSE_MEDIA_FILETYPE_SPEED,
    FUSE_MEDIA_FILETYPE_DELL_QUALIFIED,

    /** parameter*/
    FUSE_MEDIA_FILETYPE_WAVELENGTH,
    FUSE_MEDIA_FILETYPE_WAVELENGTH_TOLERANCE,
    FUSE_MEDIA_FILETYPE_MAX_CASE_TEMP,
    FUSE_MEDIA_FILETYPE_CC_BASE,
    FUSE_MEDIA_FILETYPE_CC_EXT,
    FUSE_MEDIA_FILETYPE_CONNECTOR,
    FUSE_MEDIA_FILETYPE_ENCODING_BITRATE,
    FUSE_MEDIA_FILETYPE_IDENTIFIER,
    FUSE_MEDIA_FILETYPE_EXT_IDENTIFIER,
    FUSE_MEDIA_FILETYPE_LENGTH_SMF_KM,
    FUSE_MEDIA_FILETYPE_LENGTH_OM1,
    FUSE_MEDIA_FILETYPE_LENGTH_OM2,
    FUSE_MEDIA_FILETYPE_LENGTH_OM3,
    FUSE_MEDIA_FILETYPE_LENGTH_CABLE_ASSEMBLY,
    FUSE_MEDIA_FILETYPE_DIAG_MON_TYPE,
    FUSE_MEDIA_FILETYPE_LENGTH_SMF,
    FUSE_MEDIA_FILETYPE_OPTIONS,
    FUSE_MEDIA_FILETYPE_ADVANCED_OPTIONS,

    /** vendor information */
    FUSE_MEDIA_FILETYPE_VENDOR_NAME,
    FUSE_MEDIA_FILETYPE_VENDOR_OUI,
    FUSE_MEDIA_FILETYPE_VENDOR_SN,
    FUSE_MEDIA_FILETYPE_VENDOR_DATE,
    FUSE_MEDIA_FILETYPE_VENDOR_PN,
    FUSE_MEDIA_FILETYPE_VENDOR_REVISION,

    /** dell product information */
    FUSE_MEDIA_FILETYPE_DELL_INFO_MAGIC_KEY0,
    FUSE_MEDIA_FILETYPE_DELL_INFO_MAGIC_KEY1,
    FUSE_MEDIA_FILETYPE_DELL_INFO_REVISION,
    FUSE_MEDIA_FILETYPE_DELL_INFO_PRODUCT_ID,
    FUSE_MEDIA_FILETYPE_DELL_INFO_RESERVED,

    /** module threshold */
    FUSE_MEDIA_FILETYPE_TEMP_HIGH_ALARM_THRESHOLD,
    FUSE_MEDIA_FILETYPE_TEMP_LOW_ALARM_THRESHOLD,
    FUSE_MEDIA_FILETYPE_TEMP_HIGH_WARNING_THRESHOLD,
    FUSE_MEDIA_FILETYPE_TEMP_LOW_WARNING_THRESHOLD,
    FUSE_MEDIA_FILETYPE_VOLT_HIGH_ALARM_THRESHOLD,
    FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_THRESHOLD,
    FUSE_MEDIA_FILETYPE_VOLT_HIGH_WARNING_THRESHOLD,
    FUSE_MEDIA_FILETYPE_VOLT_LOW_WARNING_THRESHOLD,

    /** channel threshold 0..3 */
    FUSE_MEDIA_FILETYPE_TX_DISABLE_THRESHOLD,
    FUSE_MEDIA_FILETYPE_TX_FAULT_THRESHOLD,
    FUSE_MEDIA_FILETYPE_TX_LOSS_THRESHOLD,
    FUSE_MEDIA_FILETYPE_RX_LOSS_THRESHOLD,

    FUSE_MEDIA_FILETYPE_LP_MODE,
    FUSE_MEDIA_FILETYPE_RESET,

    /** per media module */
    FUSE_MEDIA_FILETYPE_VOLT,
    FUSE_MEDIA_FILETYPE_TEMPERATURE,

    /** per channel 0..3 */
    FUSE_MEDIA_FILETYPE_INTERNAL_RX_POWER_MONITOR,
    FUSE_MEDIA_FILETYPE_INTERNAL_TX_POWER_BIAS,

    FUSE_MEDIA_FILETYPE_ALIAS,
    FUSE_MEDIA_FILETYPE_DIAG_MODE,

    FUSE_MEDIA_FILETYPE_MAX,
    FUSE_MEDIA_FILETYPE_MIN = FUSE_MEDIA_FILETYPE_PRESENCE

} media_filetype_t;


/*
 *  directory hierarchy tree structure
 */
typedef struct dev_node_struct {

    mode_t              st_mode;                   /** type of node and permissions */
    nlink_t             st_nlink;                  /** number of hard links */
    off_t               (*get_st_size)(struct dev_node_struct *);    /** function to determine the size of the file */      
    char                path[FUSE_FUSE_MAX_PATH];  /** directory path */
    sdi_entity_type_t   fuse_entity_type;          /** physical interface class type */
    sdi_entity_hdl_t    fuse_entity_hdl;           /** identifier for class instance */
    sdi_resource_type_t fuse_resource_type;        /** identifier for subclass type */
    sdi_resource_hdl_t  fuse_resource_hdl;         /** identifier for subclass instance */
    uint_t              fuse_filetype;             /** description of the filetype eg. STATE, THRESHOLD etc.*/
    uint_t              fuse_entity_instance;      /** entity_intance */
    uint_t              fuse_resource_instance;    /** resource instance */
    bool                fuse_entity_presence;      /** place holder for entity presence */
    bool                valid;

} dev_node_t;


/* Retrieve the fields and print them into a buffer */
void dn_pas_fuse_print(char *buf, uint_t size, size_t *len, int *res, 
        char *format, ...); 

/** get mode for the file */
int dn_pas_fuse_get_mode(dev_node_t * node, mode_t * mode);

/** get number of links */
int dn_pas_fuse_get_nlink(dev_node_t * node, nlink_t * nlink);

/** get fspec size */
int dn_pas_fuse_get_fspec_size(dev_node_t * node, off_t * size);

/** Method to check for device permissions */
bool dn_pas_fuse_check_device_permission(int flags, mode_t st_mode);

/** common routine for calculating length and offset */
size_t dn_pas_fuse_calc_size(size_t len, size_t size, off_t offset);

/** common routine for getting unsigned int from char buffer */
bool dn_pas_fuse_atoui(const char *buf, uint_t * uval);

/** common routine for parsing a fuse path */
void dn_pas_fuse_realtime_parser (dev_node_t *node, char *path);

/** Method to retrieve the sub-directory list, only children, grand-children will be avoided */
void dn_pas_fuse_get_subdir_list(void *node,void *buf, fuse_fill_dir_t filler);

#endif // __PAS_FUSE_COMMON_H
