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
 * file  : pas_fuse_parser.c
 * brief : FUSE realtime parser function to parse paths into valid SDI read/write function calls.
 * date  : 03-2015
 *
 */
#include "private/pas_fuse_common.h"
#include "private/pas_fuse_handlers.h"
#include "private/pald.h"
#include "private/pas_log.h"

#include "std_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

#define ENTITY_TYPE_UNDEFINED       ((sdi_entity_type_t) -1)
#define ENTITY_INSTANCE_UNDEFINED   (-1)
#define ENTITY_HDL_UNDEFINED        ((sdi_entity_hdl_t) NULL)
#define RESOURCE_TYPE_UNDEFINED     ((sdi_resource_type_t) -1)
#define RESOURCE_INSTANCE_UNDEFINED (-1)
#define RESOURCE_HDL_UNDEFINED      ((sdi_resource_hdl_t) NULL)
#define FILETYPE_UNDEFINED          (-1)


/** helper method to return the size for files */
static off_t fuse_default_file_size(dev_node_t *node);


/** helper method to return the size for directory */
static off_t fuse_default_dir_size(dev_node_t *node);


/** helper method to return the size for files of nvram resource type */
static off_t fuse_nvram_file_size(dev_node_t *node);


enum {
    FUSE_PATH_LEN     = 0,
    E_TYPE_PATH_LEN   = 1,
    E_INST_PATH_LEN   = 2,
    R_TYPE_PATH_LEN   = 3,
    R_INST_PATH_LEN   = 4,
    FILETYPE_PATH_LEN = 5,
    MAX_PATH_LEN      = FILETYPE_PATH_LEN,
};

/**  directory names*/
static const char FUSE_DIR_NAME_DEFAULT1[] = ".";
static const char FUSE_DIR_NAME_DEFAULT2[] = "..";
static const char FUSE_DIR_NAME_BASE[]     = "/";


typedef struct _valid_entity_t {

    char              *name;
    sdi_entity_type_t type;

} valid_entity_t;


valid_entity_t valid_entity_types[] = {

    {"system_board", SDI_ENTITY_SYSTEM_BOARD},
    {"fan_tray"    , SDI_ENTITY_FAN_TRAY},
    {"psu_tray"    , SDI_ENTITY_PSU_TRAY}

};

typedef struct _valid_resource_t {

    char                *name;
    sdi_resource_type_t type;

} valid_resource_t;

valid_resource_t valid_resource_types[] = {

    {"thermal_sensor", SDI_RESOURCE_TEMPERATURE},
    {"fan"           , SDI_RESOURCE_FAN},
    {"led"           , SDI_RESOURCE_LED},
    {"display_led"   , SDI_RESOURCE_DIGIT_DISPLAY_LED},
    {"entity_info"   , SDI_RESOURCE_ENTITY_INFO},
    {"nvram"         , SDI_RESOURCE_NVRAM},
    {"media"         , SDI_RESOURCE_MEDIA}
};

typedef struct _filename_enum_map_t {

    off_t (*get_st_size)(dev_node_t *node);
    char *filename;
    uint_t fuse_filetype;

} filename_enum_map_t;

static filename_enum_map_t fan_files[] = {
    {fuse_default_file_size, "speed"                          , FUSE_FAN_FILETYPE_SPEED},
    {fuse_default_file_size, "alert_on"                       , FUSE_FAN_FILETYPE_ALERT},
    {fuse_default_file_size, "alias_name"                     , FUSE_FAN_FILETYPE_ALIAS},
    {fuse_default_file_size, "diag_mode"                      , FUSE_FAN_FILETYPE_DIAG_MODE}
};

static filename_enum_map_t temperature_files[] = {
    {fuse_default_file_size, "temperature"                    , FUSE_THERMAL_SENSOR_FILETYPE_TEMPERATURE},
    {fuse_default_file_size, "low_threshold"                  , FUSE_THERMAL_SENSOR_FILETYPE_LOW_THRESHOLD},
    {fuse_default_file_size, "high_threshold"                 , FUSE_THERMAL_SENSOR_FILETYPE_HIGH_THRESHOLD},
    {fuse_default_file_size, "critical_threshold"             , FUSE_THERMAL_SENSOR_FILETYPE_CRITICAL_THRESHOLD},
    {fuse_default_file_size, "alert_on"                       , FUSE_THERMAL_SENSOR_FILETYPE_ALERT_ON},
    {fuse_default_file_size, "alias_name"                     , FUSE_THERMAL_SENSOR_FILETYPE_ALIAS},
    {fuse_default_file_size, "diag_mode"                      , FUSE_THERMAL_SENSOR_FILETYPE_DIAG_MODE}
};

static filename_enum_map_t led_files[] = {
    {fuse_default_file_size, "state"                          , FUSE_LED_FILETYPE_STATE},
    {fuse_default_file_size, "alias_name"                     , FUSE_LED_FILETYPE_ALIAS},
    {fuse_default_file_size, "diag_mode"                      , FUSE_LED_FILETYPE_DIAG_MODE}
};

static filename_enum_map_t nvram_files[] = {
    {fuse_nvram_file_size  , "data"                           , FUSE_NVRAM_FILETYPE_DATA},
    {fuse_default_file_size, "diag_mode"                      , FUSE_NVRAM_FILETYPE_DIAG_MODE}
};

static filename_enum_map_t display_led_files[] = {
    {fuse_default_file_size, "display_string"                 , FUSE_DISPLAY_LED_FILETYPE_STRING},
    {fuse_default_file_size, "state"                          , FUSE_DISPLAY_LED_FILETYPE_STATE},
    {fuse_default_file_size, "alias_name"                     , FUSE_DISPLAY_LED_FILETYPE_ALIAS},
    {fuse_default_file_size, "diag_mode"                      , FUSE_DISPLAY_LED_FILETYPE_DIAG_MODE}
};

static filename_enum_map_t entity_info_files[] = {
    {fuse_default_file_size, "product_name"                   , FUSE_ENTITY_INFO_FILETYPE_PROD_NAME},
    {fuse_default_file_size, "ppid"                           , FUSE_ENTITY_INFO_FILETYPE_PPID},
    {fuse_default_file_size, "hw_revision"                    , FUSE_ENTITY_INFO_FILETYPE_HW_REVISION},
    {fuse_default_file_size, "platform_name"                  , FUSE_ENTITY_INFO_FILETYPE_PLATFORM_NAME},
    {fuse_default_file_size, "vendor_name"                    , FUSE_ENTITY_INFO_FILETYPE_VENDOR_NAME},
    {fuse_default_file_size, "mac_size"                       , FUSE_ENTITY_INFO_FILETYPE_MAC_SIZE},
    {fuse_default_file_size, "base_mac"                       , FUSE_ENTITY_INFO_FILETYPE_BASE_MAC},
    {fuse_default_file_size, "num_fans"                       , FUSE_ENTITY_INFO_FILETYPE_NUM_FANS},
    {fuse_default_file_size, "max_speed"                      , FUSE_ENTITY_INFO_FILETYPE_MAX_SPEED},
    {fuse_default_file_size, "airflow"                        , FUSE_ENTITY_INFO_FILETYPE_AIRFLOW},
    {fuse_default_file_size, "power_rating"                   , FUSE_ENTITY_INFO_FILETYPE_POWER_RATING},
    {fuse_default_file_size, "alias_name"                     , FUSE_ENTITY_INFO_FILETYPE_ALIAS},
    {fuse_default_file_size, "service_tag"                    , FUSE_ENTITY_INFO_FILETYPE_SERVICE_TAG},
    {fuse_default_file_size, "diag_mode"                      , FUSE_ENTITY_INFO_FILETYPE_DIAG_MODE}
};

static filename_enum_map_t media_files[] = {
    {fuse_default_file_size, "media_presence"                , FUSE_MEDIA_FILETYPE_PRESENCE},
    /** module status */
    {fuse_default_file_size, "media_status_temp_high_alarm"  , FUSE_MEDIA_FILETYPE_TEMP_HIGH_ALARM_STATUS},
    {fuse_default_file_size, "media_status_temp_low_alarm"   , FUSE_MEDIA_FILETYPE_TEMP_LOW_ALARM_STATUS},
    {fuse_default_file_size, "media_status_temp_high_warning", FUSE_MEDIA_FILETYPE_TEMP_HIGH_WARNING_STATUS},
    {fuse_default_file_size, "media_status_temp_low_warning" , FUSE_MEDIA_FILETYPE_TEMP_LOW_WARNING_STATUS},
    {fuse_default_file_size, "media_status_volt_high_alarm"  , FUSE_MEDIA_FILETYPE_VOLT_HIGH_ALARM_STATUS},
    {fuse_default_file_size, "media_status_volt_low_alarm"   , FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_STATUS},
    {fuse_default_file_size, "media_status_volt_high_warning", FUSE_MEDIA_FILETYPE_VOLT_HIGH_WARNING_STATUS},
    {fuse_default_file_size, "media_status_volt_low_warning" , FUSE_MEDIA_FILETYPE_VOLT_LOW_WARNING_STATUS},

    /** channel status 1..4 */
    {fuse_default_file_size, "media_status_tx_disable"       , FUSE_MEDIA_FILETYPE_TX_DISABLE_STATUS},
    {fuse_default_file_size, "media_status_tx_fault"         , FUSE_MEDIA_FILETYPE_TX_FAULT_STATUS},
    {fuse_default_file_size, "media_status_tx_loss"          , FUSE_MEDIA_FILETYPE_TX_LOSS_STATUS},
    {fuse_default_file_size, "media_status_rx_loss"          , FUSE_MEDIA_FILETYPE_RX_LOSS_STATUS},

    /** media transmission control 1..4 */
    {fuse_default_file_size, "media_tx_control"              , FUSE_MEDIA_FILETYPE_TX_CONTROL},
    {fuse_default_file_size, "media_speed"                   , FUSE_MEDIA_FILETYPE_SPEED},
    {fuse_default_file_size, "media_dell_qualified"          , FUSE_MEDIA_FILETYPE_DELL_QUALIFIED},

    /** parameter */
    {fuse_default_file_size, "media_wavelength"              , FUSE_MEDIA_FILETYPE_WAVELENGTH},
    {fuse_default_file_size, "media_wavelength_tolerance"    , FUSE_MEDIA_FILETYPE_WAVELENGTH_TOLERANCE},
    {fuse_default_file_size, "media_max_case_temp"           , FUSE_MEDIA_FILETYPE_MAX_CASE_TEMP},
    {fuse_default_file_size, "media_cc_base"                 , FUSE_MEDIA_FILETYPE_CC_BASE},
    {fuse_default_file_size, "media_cc_ext"                  , FUSE_MEDIA_FILETYPE_CC_EXT},
    {fuse_default_file_size, "media_connector"               , FUSE_MEDIA_FILETYPE_CONNECTOR},
    {fuse_default_file_size, "media_encoding_bitrate"        , FUSE_MEDIA_FILETYPE_ENCODING_BITRATE},
    {fuse_default_file_size, "media_identifier"              , FUSE_MEDIA_FILETYPE_IDENTIFIER},
    {fuse_default_file_size, "media_ext_identifier"          , FUSE_MEDIA_FILETYPE_EXT_IDENTIFIER},
    {fuse_default_file_size, "media_length_smf_km"           , FUSE_MEDIA_FILETYPE_LENGTH_SMF_KM},
    {fuse_default_file_size, "media_length_om1"              , FUSE_MEDIA_FILETYPE_LENGTH_OM1},
    {fuse_default_file_size, "media_length_om2"              , FUSE_MEDIA_FILETYPE_LENGTH_OM2},
    {fuse_default_file_size, "media_length_om3"              , FUSE_MEDIA_FILETYPE_LENGTH_OM3},
    {fuse_default_file_size, "media_length_cable_assembly"   , FUSE_MEDIA_FILETYPE_LENGTH_CABLE_ASSEMBLY},
    {fuse_default_file_size, "media_diag_mon_type"           , FUSE_MEDIA_FILETYPE_DIAG_MON_TYPE},
    {fuse_default_file_size, "media_length_smf"              , FUSE_MEDIA_FILETYPE_LENGTH_SMF},
    {fuse_default_file_size, "media_options"                 , FUSE_MEDIA_FILETYPE_OPTIONS},
    {fuse_default_file_size, "media_enhanced_options"        , FUSE_MEDIA_FILETYPE_ADVANCED_OPTIONS},

    /** vendor information */
    {fuse_default_file_size, "media_vendor_name"             , FUSE_MEDIA_FILETYPE_VENDOR_NAME},
    {fuse_default_file_size, "media_vendor_oui"              , FUSE_MEDIA_FILETYPE_VENDOR_OUI},
    {fuse_default_file_size, "media_vendor_sn"               , FUSE_MEDIA_FILETYPE_VENDOR_SN},
    {fuse_default_file_size, "media_vendor_date"             , FUSE_MEDIA_FILETYPE_VENDOR_DATE},
    {fuse_default_file_size, "media_vendor_pn"               , FUSE_MEDIA_FILETYPE_VENDOR_PN},
    {fuse_default_file_size, "media_vendor_revision"         , FUSE_MEDIA_FILETYPE_VENDOR_REVISION},

    /** dell product information */
    {fuse_default_file_size, "dell_media_info_magic_key0"    , FUSE_MEDIA_FILETYPE_DELL_INFO_MAGIC_KEY0},
    {fuse_default_file_size, "dell_media_info_magic_key1"    , FUSE_MEDIA_FILETYPE_DELL_INFO_MAGIC_KEY1},
    {fuse_default_file_size, "dell_media_info_revision"      , FUSE_MEDIA_FILETYPE_DELL_INFO_REVISION},
    {fuse_default_file_size, "dell_media_info_product_id"    , FUSE_MEDIA_FILETYPE_DELL_INFO_PRODUCT_ID},
    {fuse_default_file_size, "dell_media_info_reserved"      , FUSE_MEDIA_FILETYPE_DELL_INFO_RESERVED},

    /** module threshold */
    {fuse_default_file_size, "media_threshold_temp_high_alarm"  , FUSE_MEDIA_FILETYPE_TEMP_HIGH_ALARM_THRESHOLD},
    {fuse_default_file_size, "media_threshold_temp_low_alarm"   , FUSE_MEDIA_FILETYPE_TEMP_LOW_ALARM_THRESHOLD},
    {fuse_default_file_size, "media_threshold_temp_high_warning", FUSE_MEDIA_FILETYPE_TEMP_HIGH_WARNING_THRESHOLD},
    {fuse_default_file_size, "media_threshold_temp_low_warning" , FUSE_MEDIA_FILETYPE_TEMP_LOW_WARNING_THRESHOLD},
    {fuse_default_file_size, "media_threshold_volt_high_alarm"  , FUSE_MEDIA_FILETYPE_VOLT_HIGH_ALARM_THRESHOLD},
    {fuse_default_file_size, "media_threshold_volt_low_alarm"   , FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_THRESHOLD},
    {fuse_default_file_size, "media_threshold_volt_high_warning", FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_THRESHOLD},
    {fuse_default_file_size, "media_threshold_volt_low_warning" , FUSE_MEDIA_FILETYPE_VOLT_LOW_WARNING_THRESHOLD},

    /** channel threshold 1..4 */
    {fuse_default_file_size, "media_threshold_tx_disable"       , FUSE_MEDIA_FILETYPE_TX_DISABLE_THRESHOLD},
    {fuse_default_file_size, "media_threshold_tx_fault"         , FUSE_MEDIA_FILETYPE_TX_FAULT_THRESHOLD},
    {fuse_default_file_size, "media_threshold_tx_loss"          , FUSE_MEDIA_FILETYPE_TX_LOSS_THRESHOLD},
    {fuse_default_file_size, "media_threshold_rx_loss"          , FUSE_MEDIA_FILETYPE_RX_LOSS_THRESHOLD},
    {fuse_default_file_size, "media_lp_mode"                    , FUSE_MEDIA_FILETYPE_LP_MODE},
    {fuse_default_file_size, "media_reset"                      , FUSE_MEDIA_FILETYPE_RESET},

    /** per media module */
    {fuse_default_file_size, "media_volt"                       , FUSE_MEDIA_FILETYPE_VOLT},
    {fuse_default_file_size, "media_temperature"                , FUSE_MEDIA_FILETYPE_TEMPERATURE},

    /** per channel 1..4 */
    {fuse_default_file_size, "media_internal_rx_power_monitor"  , FUSE_MEDIA_FILETYPE_INTERNAL_RX_POWER_MONITOR},
    {fuse_default_file_size, "media_internal_tx_power_bias"     , FUSE_MEDIA_FILETYPE_INTERNAL_TX_POWER_BIAS},
    
    {fuse_default_file_size, "alias_name"                       , FUSE_MEDIA_FILETYPE_ALIAS},
    {fuse_default_file_size, "diag_mode"                        , FUSE_MEDIA_FILETYPE_DIAG_MODE}
};


typedef struct _resource_files_t {
    
    char *dirname;
    uint_t max_file_count;
    uint_t min_file_count;
    filename_enum_map_t *filename_map;

} resource_files_t;

static const resource_files_t resource_files[] = {
    [SDI_RESOURCE_FAN] = {
        dirname          : "/fan",
        max_file_count   : FUSE_FAN_FILETYPE_MAX,
        min_file_count   : FUSE_FAN_FILETYPE_MIN,
        filename_map     : fan_files
    },

    [SDI_RESOURCE_TEMPERATURE] = {
        dirname          : "/thermal_sensor",
        max_file_count   : FUSE_THERMAL_SENSOR_FILETYPE_MAX,
        min_file_count   : FUSE_THERMAL_SENSOR_FILETYPE_MIN,
        filename_map     : temperature_files
    },

    [SDI_RESOURCE_LED] = {
        dirname          : "/led",
        max_file_count   : FUSE_LED_FILETYPE_MAX,
        min_file_count   : FUSE_LED_FILETYPE_MIN,
        filename_map     : led_files
    },

    [SDI_RESOURCE_DIGIT_DISPLAY_LED] = {
        dirname          : "/display_led",
        max_file_count   : FUSE_DISPLAY_LED_FILETYPE_MAX,
        min_file_count   : FUSE_DISPLAY_LED_FILETYPE_MIN,
        filename_map     : display_led_files
    },

    [SDI_RESOURCE_ENTITY_INFO] = {
        dirname          : "/entity_info",
        max_file_count   : FUSE_ENTITY_INFO_FILETYPE_MAX,
        min_file_count   : FUSE_ENTITY_INFO_FILETYPE_MIN,
        filename_map     : entity_info_files
    },

    [SDI_RESOURCE_NVRAM] = {
        dirname          : "/nvram",
        max_file_count   : FUSE_NVRAM_FILETYPE_MAX,
        min_file_count   : FUSE_NVRAM_FILETYPE_MIN,
        filename_map     : nvram_files
    },

    [SDI_RESOURCE_MEDIA] = {
        dirname          : "/media",
        max_file_count   : FUSE_MEDIA_FILETYPE_MAX,
        min_file_count   : FUSE_MEDIA_FILETYPE_MIN,
        filename_map     : media_files
    }
};


/** Call back function to learn the resource handles.*/
typedef struct resource_handle_table {

    sdi_resource_type_t resource_type;
    sdi_resource_hdl_t *resource_hdl_tbl;
    uint_t res_count;

} resource_handle_table_t;


/** Call back method called for each resource within every entity
 * Used to calculate the number of resources within each entity */
static void fuse_resource_cb(
        sdi_resource_hdl_t hdl,
        void               *user_data
        )
{
    resource_handle_table_t *tbl = (resource_handle_table_t *) user_data;

    if (sdi_resource_type_get(hdl) == tbl->resource_type) {

        tbl->resource_hdl_tbl[tbl->res_count++] = hdl;
    }
}


/** helper method to return the size for default dir size */
static off_t fuse_default_dir_size(dev_node_t *node)
{
    return FUSE_DIR_DEFAULT_SIZE;
}


/** helper method to return the size for files of default resource type */
static off_t fuse_default_file_size(dev_node_t *node)
{
    return FUSE_FILE_DEFAULT_SIZE;
}


/** helper method to return the size for files of nvram resource type */
static off_t fuse_nvram_file_size(dev_node_t *node)
{
    uint_t  nvram_size = 0;
    return (sdi_nvram_size(node->fuse_resource_hdl, &nvram_size) != STD_ERR_OK
            ? FUSE_FILE_DEFAULT_SIZE : nvram_size
            );
}


/** helper method to return maximum count for files of a particular resource type */
static uint_t fuse_resource_filetype_max(sdi_resource_type_t resource_type)
{
    return resource_files[resource_type].max_file_count;
}


/** helper method to return maximum count for files of a particular resource type */
static uint_t fuse_resource_filetype_min(sdi_resource_type_t resource_type)
{
    return resource_files[resource_type].min_file_count;
}


/*---------------------------------------------------------------------*/
/*                        FUSE NODE HANDLING                           */
/*---------------------------------------------------------------------*/

/*
 * Method to retrieve entity type 
 */
static void fuse_get_entity_type(
        dev_node_t         *node, 
        std_parsed_string_t tokens, 
        int                count
        )
{
    sdi_entity_type_t e_type = ENTITY_TYPE_UNDEFINED;

    if(count >= E_TYPE_PATH_LEN && 
            count <= FILETYPE_PATH_LEN) {
        
        const char   *entity_type = std_parse_string_at(tokens, E_TYPE_PATH_LEN);
        size_t       i            = 0;

        for(i = 0; i < ARRAY_SIZE(valid_entity_types); i++) {
            
            if(0 == strcmp(entity_type, valid_entity_types[i].name)) {

                e_type = valid_entity_types[i].type;
                break;
            }
        }     
    }  
    node->fuse_entity_type = e_type;
}


/*
 * Method to retrieve entity instance 
 */
static void fuse_get_entity_instance(
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    sdi_entity_type_t e_type = node->fuse_entity_type;
    uint_t            e_inst = ENTITY_INSTANCE_UNDEFINED;


    if(count >= E_INST_PATH_LEN && 
            count <= FILETYPE_PATH_LEN &&
            e_type != ENTITY_TYPE_UNDEFINED) {

        const char *instance = std_parse_string_at(tokens, E_INST_PATH_LEN);

        if (dn_pas_fuse_atoui(instance, &e_inst) &&
               (e_inst <= sdi_entity_count_get(e_type))) {

            node->fuse_entity_instance = e_inst;
            return;
        }
    } 
    
    node->fuse_entity_instance = ENTITY_INSTANCE_UNDEFINED;
}


/*
 * Method to retrieve entity handle 
 */
static void fuse_get_entity_handle(
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    sdi_entity_type_t e_type = node->fuse_entity_type;
    uint_t            e_inst = node->fuse_entity_instance;
    sdi_entity_hdl_t  e_hdl  = ENTITY_HDL_UNDEFINED;


    if(count  >= E_INST_PATH_LEN && 
            count  <= FILETYPE_PATH_LEN &&
            e_type != ENTITY_TYPE_UNDEFINED && 
            e_inst != ENTITY_INSTANCE_UNDEFINED) { 

        e_hdl = sdi_entity_lookup(e_type, e_inst);
    }

    node->fuse_entity_hdl = e_hdl;
}


/*
 * Method to get the resource type 
 */
static void fuse_get_resource_type(
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    sdi_resource_type_t r_type = RESOURCE_TYPE_UNDEFINED;
    
    if(count >= R_TYPE_PATH_LEN && 
            count <= FILETYPE_PATH_LEN) {

        const char *resource_type = std_parse_string_at(tokens, R_TYPE_PATH_LEN);
        size_t     i              = 0;

        for(i = 0; i < ARRAY_SIZE(valid_resource_types); i++) {
            
            if(0 == strcmp(resource_type, valid_resource_types[i].name)) {

                r_type = valid_resource_types[i].type;
                break;
            }
        }     
    }
    node->fuse_resource_type = r_type;
}


/*
 * Method to retrieve resource instance 
 */
static void fuse_get_resource_instance(
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    sdi_entity_hdl_t    e_hdl  = node->fuse_entity_hdl;
    sdi_resource_type_t r_type = node->fuse_resource_type;
    uint_t              r_inst = RESOURCE_INSTANCE_UNDEFINED;


    if(count >= R_INST_PATH_LEN && 
            count <= FILETYPE_PATH_LEN &&
            e_hdl != ENTITY_HDL_UNDEFINED &&
            r_type != RESOURCE_TYPE_UNDEFINED) {

        const char *res_instance = std_parse_string_at(tokens, R_INST_PATH_LEN);
        if (dn_pas_fuse_atoui(res_instance, &r_inst) &&
                (r_inst <= sdi_entity_resource_count_get(e_hdl, r_type))) {

            node->fuse_resource_instance = r_inst;
            return;
        }
    } 
    
    node->fuse_resource_instance = RESOURCE_INSTANCE_UNDEFINED;
}


/*
 * Method to retrieve resource handle 
 */
static void fuse_get_resource_handle(
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    resource_handle_table_t res_struct;
    sdi_entity_hdl_t        e_hdl      = node->fuse_entity_hdl;
    sdi_resource_type_t     r_type     = node->fuse_resource_type;
    uint_t                  r_inst     = node->fuse_resource_instance;
    sdi_resource_hdl_t      r_hdl      = RESOURCE_HDL_UNDEFINED; 


    if(count  >= R_INST_PATH_LEN && 
            count  <= FILETYPE_PATH_LEN &&
            e_hdl  != ENTITY_HDL_UNDEFINED &&
            r_type != RESOURCE_TYPE_UNDEFINED &&
            r_inst != RESOURCE_INSTANCE_UNDEFINED) {

        /** get number of resources per entity */
        uint_t             r_size = sdi_entity_resource_count_get(e_hdl, r_type);
        sdi_resource_hdl_t arr[r_size];
        
        memset(&res_struct, 0, sizeof(res_struct));
        res_struct.resource_type    = r_type;
        res_struct.resource_hdl_tbl = arr;
        sdi_entity_for_each_resource(e_hdl, fuse_resource_cb, &res_struct);

        if(r_inst <= r_size) {
            /** number of resources = r_inst, index into table is r_inst-1 */
            r_hdl = arr[r_inst - 1];
        }
    } 
    
    node->fuse_resource_hdl = r_hdl;
    return;
}


/*
 * Method to get the fuse file mode 
 */
static void fuse_get_mode(
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    if(count == FILETYPE_PATH_LEN) {
        
        node->st_mode = FUSE_FILE_MODE_RW;
        return;
    }

    node->st_mode = FUSE_DIR_MODE_DEFAULT;
    return;
}


/*
 * Method to get the file size 
 */
static void fuse_get_size (
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    if(count == FILETYPE_PATH_LEN) {

        node->get_st_size = fuse_default_file_size;
        return;
    }

    node->get_st_size = fuse_default_dir_size;
    return;
}


/*
 * Method to retrieve filetype 
 */
static void fuse_get_filetype (
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count)
{
    uint_t filetype = FUSE_SDI_DEVICE_DIR;

    if(count == FILETYPE_PATH_LEN) { 

        int                   r_file_node = 0;
        sdi_resource_type_t   r_type      = node->fuse_resource_type;
        sdi_resource_hdl_t    r_hdl       = node->fuse_resource_hdl;
        const char            *filename   = std_parse_string_at(tokens, FILETYPE_PATH_LEN);

        /** for each resource filetype, register filespec in db */
        for (r_file_node = fuse_resource_filetype_min(r_type);                
                r_file_node < fuse_resource_filetype_max(r_type);
                r_file_node++) {

            if( r_type != RESOURCE_TYPE_UNDEFINED && 
                    r_hdl != RESOURCE_HDL_UNDEFINED) {

                if(0 == strcmp(filename, resource_files[r_type].filename_map[r_file_node].filename)) {

                    filetype            = resource_files[r_type].filename_map[r_file_node].fuse_filetype;
                    node->fuse_filetype = filetype;
                    node->get_st_size = resource_files[r_type].filename_map[r_file_node].get_st_size;
                    return;
                }
            }
        }

        node->fuse_filetype = FILETYPE_UNDEFINED;
        node->get_st_size = fuse_default_file_size;
        return;
    }

    node->fuse_filetype = filetype;
    node->get_st_size = fuse_default_dir_size;
}


/*
 * Method to retrieve number of links 
 */
static void fuse_get_nlink (
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    int nlink = 2;

    switch(count)
    {
        case FUSE_PATH_LEN    : 
            {
                nlink += ARRAY_SIZE(valid_entity_types);
                break;
            }

        case E_TYPE_PATH_LEN  : 
            {
                sdi_entity_type_t e_type = node->fuse_entity_type;
                if(e_type != ENTITY_TYPE_UNDEFINED) {

                    nlink += sdi_entity_count_get(e_type);
                }
                break;
            }
    
        case E_INST_PATH_LEN  :
            {
                sdi_entity_hdl_t e_hdl  = node->fuse_entity_hdl;
                if(e_hdl != ENTITY_HDL_UNDEFINED) {

                    uint_t r_count = 0;
                    uint_t i       = 0;

                    for(i = 0; i < ARRAY_SIZE(valid_resource_types); i++) {

                        if((r_count = sdi_entity_resource_count_get(e_hdl, valid_resource_types[i].type)) > 0) {
                            nlink += 1;
                        }
                    }
                }
                break;
            }
    
        case R_TYPE_PATH_LEN :
            {
                sdi_entity_hdl_t    e_hdl   = node->fuse_entity_hdl;
                sdi_resource_type_t r_type  = node->fuse_resource_type;

                if(e_hdl != ENTITY_HDL_UNDEFINED &&
                        r_type != RESOURCE_TYPE_UNDEFINED) {

                    uint_t r_count = 0;
                    uint_t i       = 0;

                    for(i = 0; i < ARRAY_SIZE(valid_resource_types); i++) {

                        if((r_count = sdi_entity_resource_count_get(e_hdl, valid_resource_types[i].type)) > 0) {
                            nlink += r_count;
                        }
                    }
                }
                break;
            } 
    
        case R_INST_PATH_LEN :
            {
                sdi_resource_type_t r_type  = node->fuse_resource_type;
                sdi_resource_hdl_t r_hdl    = node->fuse_resource_hdl;

                if(r_type != RESOURCE_TYPE_UNDEFINED &&
                        r_hdl != RESOURCE_HDL_UNDEFINED) {

                    uint_t i       = 0;

                    for(i = 0; i < ARRAY_SIZE(valid_resource_types); i++) {

                        nlink += fuse_resource_filetype_max(r_type);
                    }
                }
                break;    
            }
    
        default: node->st_nlink = 0; return;
    }

    node->st_nlink = nlink;
}

/** Method to retrieve entity presence */
static void fuse_get_entity_presence (
        dev_node_t          *node, 
        std_parsed_string_t tokens, 
        int                 count
        )
{
    bool             presence  = false;
    sdi_entity_hdl_t e_hdl     = node->fuse_entity_hdl;

    if(count >= E_TYPE_PATH_LEN &&
            count <= FILETYPE_PATH_LEN &&
            e_hdl != ENTITY_HDL_UNDEFINED) { 

        if(STD_ERR_OK == sdi_entity_presence_get(e_hdl, &presence)) {

            node->fuse_entity_presence = presence;
            return;
        }
    }

    node->fuse_entity_presence = false;
}


static std_parsed_string_t delim_tokenize(
        char       *a_str, 
        const char *a_delim,
        size_t     *count
        )
{
    
    std_parsed_string_t handle;
    
    if(!std_parse_string(&handle, a_str, a_delim)) {

        std_parse_string_free(handle);
        return NULL;
    }
   
    /** count value is used in other functions */
    *count   = std_parse_string_num_tokens(handle) - 1;
    
    return handle;
}
    

/** Internal helper method to validate paths */
static bool validate_path_format(
        std_parsed_string_t tokens, 
        int                 count
        )
{
    /** validate entity type in path */
    bool e_type_valid = false;   
    if(count >= E_TYPE_PATH_LEN) {

        uint_t i                  = 0;
        const char   *entity_type = std_parse_string_at(tokens, E_TYPE_PATH_LEN);

        for(i = 0; i < ARRAY_SIZE(valid_entity_types); i++) {

            if(strcmp(entity_type, valid_entity_types[i].name) == 0) {

                e_type_valid = true;
                break;
            }
        }

        if(!e_type_valid) {

            return false;
        }
    }

    /** validate entity instance in path */
    bool e_inst_valid = false;
    if(count >= E_INST_PATH_LEN) {

        uint_t       uval         = 0;
        const char   *entity_inst = std_parse_string_at(tokens, E_INST_PATH_LEN);

        if(dn_pas_fuse_atoui((const char *) entity_inst, &uval)) {

            if(uval > 0) {
                
                e_inst_valid = true;
            }
        }

        if(!e_inst_valid) {

            return false;
        }
    }

    /** validate resource type in path */
    bool r_type_valid = false;   
    if(count >= R_TYPE_PATH_LEN) {

        uint_t             i = 0;
        const char   *r_type = std_parse_string_at(tokens, R_TYPE_PATH_LEN);

        for(i = 0; i < ARRAY_SIZE(valid_resource_types); i++) {

            if(strcmp(r_type, valid_resource_types[i].name) == 0) {

                r_type_valid = true;
                break;
            }
        }

        if(!r_type_valid) {

            return false;
        }
    }

    /** validate resource instance in path */
    bool r_inst_valid = false;   
    if(count >= R_INST_PATH_LEN) {

        uint_t       uval           = 0;
        const char   *resource_inst = std_parse_string_at(tokens, R_INST_PATH_LEN);

        if(dn_pas_fuse_atoui((const char *) resource_inst, &uval)) {

            if(uval > 0) {
                
                r_inst_valid = true;
            }
        }

        if(!r_inst_valid) {

            return false;
        }
    }

    switch(count) {

        case E_TYPE_PATH_LEN: return e_type_valid;

        case E_INST_PATH_LEN: return e_inst_valid;

        case R_TYPE_PATH_LEN: return r_type_valid;

        case R_INST_PATH_LEN: return r_inst_valid;

        default             : return (e_type_valid &&
                                      e_inst_valid &&
                                      r_type_valid &&
                                      r_inst_valid);
    }
    //return (e_type_valid && e_inst_valid && r_type_valid && r_inst_valid);
}


/** Method to parse the path in realtime and make a valid node from it */
void dn_pas_fuse_realtime_parser (
        dev_node_t *node, 
        char       *path
        )
{
    std_parsed_string_t tokens = NULL;
    int     count  = 0;
    memset(node, 0, sizeof(dev_node_t));
    node->valid = false;

    /** make a copy of the path, to avoid path corruption */
    char temp_path[FUSE_FUSE_MAX_PATH];
    if(!safestrncpy(temp_path, path, FUSE_FUSE_MAX_PATH)) {
        return;
    }      
    
    /** Root node handling */
    if(strcmp(temp_path, "/") == 0) {
        if(!safestrncpy(node->path, temp_path, FUSE_FUSE_MAX_PATH)) {
            node->valid = false;
            return;
        }

        node->fuse_entity_type       = ENTITY_TYPE_UNDEFINED;
        node->fuse_entity_instance   = ENTITY_INSTANCE_UNDEFINED;
        node->fuse_entity_hdl        = ENTITY_HDL_UNDEFINED;
        node->fuse_resource_type     = RESOURCE_TYPE_UNDEFINED;
        node->fuse_resource_instance = RESOURCE_INSTANCE_UNDEFINED;
        node->fuse_resource_hdl      = RESOURCE_HDL_UNDEFINED;
        node->fuse_entity_presence   = false;
        fuse_get_mode(node, tokens, count);
        fuse_get_size(node, tokens, count);
        node->st_nlink               = 3;
        node->fuse_filetype          = FUSE_DIR_MODE_DEFAULT;
        node->valid                  = true;
        return;
    }

    /** Diagnostic node handling */
    if(strcmp(temp_path, "/diag_mode") == 0) {
        if(!safestrncpy(node->path, temp_path, FUSE_FUSE_MAX_PATH)) {
            node->valid = false;
            return;
        }
        node->fuse_entity_type       = ENTITY_TYPE_UNDEFINED;
        node->fuse_entity_instance   = ENTITY_INSTANCE_UNDEFINED;
        node->fuse_entity_hdl        = ENTITY_HDL_UNDEFINED;
        node->fuse_resource_type     = RESOURCE_TYPE_UNDEFINED;
        node->fuse_resource_instance = RESOURCE_INSTANCE_UNDEFINED;
        node->fuse_resource_hdl      = RESOURCE_HDL_UNDEFINED;
        node->fuse_entity_presence   = false;
        node->st_mode                = FUSE_FILE_MODE_RW;   
        node->get_st_size            = fuse_default_file_size;
        node->st_nlink               = 0;
        node->fuse_filetype          = FUSE_DIAG_MODE_FILETYPE;
        node->valid                  = true;
        return;
    }

    /** In Diagnostic mode return null object */
    if(dn_pald_diag_mode_get()) {
        if(!safestrncpy(node->path, temp_path, FUSE_FUSE_MAX_PATH)) {
            node->valid = false;
            return;
        }
        node->fuse_entity_type       = ENTITY_TYPE_UNDEFINED;
        node->fuse_entity_instance   = ENTITY_INSTANCE_UNDEFINED;
        node->fuse_entity_hdl        = ENTITY_HDL_UNDEFINED;
        node->fuse_resource_type     = RESOURCE_TYPE_UNDEFINED;
        node->fuse_resource_instance = RESOURCE_INSTANCE_UNDEFINED;
        node->fuse_resource_hdl      = RESOURCE_HDL_UNDEFINED;
        node->fuse_entity_presence   = false;
        node->st_mode                = FUSE_FILE_MODE_RW;   
        node->get_st_size            = fuse_default_file_size;
        node->st_nlink               = 0;
        node->fuse_filetype          = FUSE_DIR_MODE_DEFAULT;
        node->valid                  = true;
        return;
    }

    /** Tokenize the path */
    tokens = delim_tokenize(temp_path, "/", (size_t *) &count);
    
    do {
        if (tokens == 0) break;

        /** Validate the path */
        if(!validate_path_format(tokens, count)) {

            memset(node, 0, sizeof(dev_node_t));
            node->valid = false;
            break;
        }

        /** make a valid node from path */
        if(!safestrncpy(node->path, path, FUSE_FUSE_MAX_PATH)) {
            node->valid = false;
            break;  
        }

        fuse_get_entity_type(node, tokens, count);
        fuse_get_entity_instance(node, tokens, count);
        fuse_get_entity_handle(node, tokens, count);
        fuse_get_resource_type(node, tokens, count);
        fuse_get_resource_instance(node, tokens, count);
        fuse_get_resource_handle(node, tokens, count);
        fuse_get_entity_presence(node, tokens, count);
        fuse_get_mode(node, tokens, count);
        fuse_get_size(node, tokens, count);
        fuse_get_nlink(node, tokens, count);
        fuse_get_filetype(node, tokens, count);
        node->valid = true;

        /** Node correctness */
        if(count > FILETYPE_PATH_LEN || 
                (count == E_TYPE_PATH_LEN && 
                 node->fuse_entity_type == ENTITY_TYPE_UNDEFINED) ||
                (count == E_INST_PATH_LEN && 
                 node->fuse_entity_hdl == ENTITY_HDL_UNDEFINED) ||
                (count == R_TYPE_PATH_LEN && 
                 node->fuse_resource_type == RESOURCE_TYPE_UNDEFINED) ||
                (count == R_INST_PATH_LEN && 
                 node->fuse_resource_hdl == RESOURCE_HDL_UNDEFINED)) {

            memset(node, 0, sizeof(dev_node_t));
            node->valid = false;
            break;
        }


        /** Test entity presence */
        bool presence = true;
        if(count >= E_INST_PATH_LEN && 
                STD_ERR_OK == sdi_entity_presence_get(node->fuse_entity_hdl, &presence)) {

            if(presence == false) {

                memset(node, 0, sizeof(dev_node_t));
                node->valid = false;
                break;
            }
        }

        /** Test psu power status */
        bool power_state = true;
        if(count >= E_INST_PATH_LEN && node->fuse_entity_type == SDI_ENTITY_PSU_TRAY &&
                STD_ERR_OK == sdi_entity_psu_output_power_status_get(node->fuse_entity_hdl, &power_state)) {

            if(power_state == false) {

                memset(node, 0, sizeof(dev_node_t));
                free(tokens);
                node->valid = false;
                return;
            }
        }

        /** Test media presence */
        if(count >= R_INST_PATH_LEN && 
                node->fuse_resource_type == SDI_RESOURCE_MEDIA &&
                STD_ERR_OK == sdi_media_presence_get(node->fuse_resource_hdl, &presence)) {

            if(presence == false) {

                memset(node, 0, sizeof(dev_node_t));
                node->valid = false;
                break;
            }
        }
    } while(0);

    std_parse_string_free(tokens);
    return;
}

/*
 * Internal helper functon to send sub directory inforation to FUSE 
 */
static void send_to_fuse_filler(
        char            *temp_path, 
        char            *subdir_root,
        fuse_fill_dir_t filler,
        void            *buf
        )
{
    dev_node_t tmp_dev_node;

    dn_pas_fuse_realtime_parser(&tmp_dev_node, temp_path);

    if(!tmp_dev_node.valid) return;

    struct stat st = { 0 };
    st.st_mode = tmp_dev_node.st_mode;
    st.st_nlink = tmp_dev_node.st_nlink;
    st.st_size = tmp_dev_node.get_st_size(&tmp_dev_node);
    filler(buf, tmp_dev_node.path + strlen(subdir_root), &st, 0);
}

/*
 * Method to retrieve the sub-directory list, only children, grand-children will be avoided
 */
void dn_pas_fuse_get_subdir_list(
        void            *node, 
        void            *buf, 
        fuse_fill_dir_t filler
        )
{

    //generate filename path
    dev_node_t *parent_node = (dev_node_t *) node;
    char subdir_root[FUSE_FUSE_MAX_PATH];
    safestrncpy(subdir_root, parent_node->path, FUSE_FUSE_MAX_PATH);


    if (strncmp(subdir_root, FUSE_DIR_NAME_BASE, FUSE_FUSE_MAX_PATH) != 0) {

        strncat(subdir_root, 
                FUSE_DIR_NAME_BASE,
                FUSE_FUSE_MAX_PATH - 1);
    }


    /* print subdirs  */
    filler(buf, FUSE_DIR_NAME_DEFAULT1, NULL, 0);
    filler(buf, FUSE_DIR_NAME_DEFAULT2, NULL, 0); 


    if(parent_node->fuse_resource_hdl != RESOURCE_HDL_UNDEFINED) {

        uint_t              i              = 0;
        sdi_resource_type_t r_type         = parent_node->fuse_resource_type;
        uint_t              max_file_count = fuse_resource_filetype_max(r_type);

        for(i = 0; i < max_file_count; i++) {


            /** Setup the path */
            char temp_path[FUSE_FUSE_MAX_PATH];
            snprintf(temp_path,
                    FUSE_FUSE_MAX_PATH,
                    "%s/%s",
                    parent_node->path,
                    resource_files[r_type].filename_map[i].filename);

            send_to_fuse_filler(temp_path, subdir_root, filler, buf); 
        }


    } else if (parent_node->fuse_resource_type != RESOURCE_TYPE_UNDEFINED) {

        uint_t              i              = 0;
        sdi_entity_hdl_t    e_hdl          = parent_node->fuse_entity_hdl;
        sdi_resource_type_t r_type         = parent_node->fuse_resource_type;
        uint_t              r_inst         = sdi_entity_resource_count_get(e_hdl, r_type);

        for(i = 0; i < r_inst; i++) {

            /** Setup the path */
            char temp_path[FUSE_FUSE_MAX_PATH];
            snprintf(temp_path,
                    FUSE_FUSE_MAX_PATH,
                    "%s/%d",
                    parent_node->path,
                    i+1);

            send_to_fuse_filler(temp_path, subdir_root, filler, buf); 
        }


    } else if (parent_node->fuse_entity_hdl != ENTITY_HDL_UNDEFINED) {

        uint_t              i              = 0;
        sdi_entity_hdl_t    e_hdl          = parent_node->fuse_entity_hdl;

        for(i = 0; i < ARRAY_SIZE(valid_resource_types); i++) {

            if(sdi_entity_resource_count_get(e_hdl, valid_resource_types[i].type) > 0) {

                /** Setup the path */
                char temp_path[FUSE_FUSE_MAX_PATH];
                snprintf(temp_path,
                        FUSE_FUSE_MAX_PATH,
                        "%s/%s",
                        parent_node->path,
                        valid_resource_types[i].name);

                send_to_fuse_filler(temp_path, subdir_root, filler, buf); 
            }
        }


    } else if (parent_node->fuse_entity_type != ENTITY_TYPE_UNDEFINED) {

        uint_t count = 0;
        if((count = sdi_entity_count_get(parent_node->fuse_entity_type)) > 0) {

            uint_t j = 0;

            for(j = 0; j < count; j++) {
                /** Setup the path */
                char temp_path[FUSE_FUSE_MAX_PATH];
                snprintf(temp_path,
                        FUSE_FUSE_MAX_PATH,
                        "%s/%d",
                        parent_node->path,
                        j+1);

                send_to_fuse_filler(temp_path, subdir_root, filler, buf); 
            }
        }


    } else {

        uint_t i = 0;
        char   temp_path[FUSE_FUSE_MAX_PATH];
        for(i = 0; i < ARRAY_SIZE(valid_entity_types); i++) {

            /** Setup the path */
            snprintf(temp_path,
                    FUSE_FUSE_MAX_PATH,
                    "%s%s",
                    parent_node->path,
                    valid_entity_types[i].name);

            send_to_fuse_filler(temp_path, subdir_root, filler, buf); 
        }

        /** Adding Diagnostic mode file to the subdir list */
        snprintf(temp_path,
                    FUSE_FUSE_MAX_PATH,
                    "%s%s",
                    parent_node->path,
                    "diag_mode");

        send_to_fuse_filler(temp_path, subdir_root, filler, buf); 
    }
}
