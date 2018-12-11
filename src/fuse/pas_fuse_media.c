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

/**
 * file : pas_fuse_media.c
 * brief: pas daemon interfthe layer to entity_info SDI API
 * date : 04/2015
 *
 */


#include "private/pas_fuse_handlers.h"
#include "std_bit_masks.h"

#define SUPPORTED   1
#define UNSUPPORTED 0

#define EEPROM_PAGE_MAX_SIZE    256


/* Cursor is csv of format: "device,bank,page,offset" . If field is N/A, set as -1 */
typedef struct {
    int bank;
    int page;
    int device;
    int offset;
} eeprom_cursor_t;
static eeprom_cursor_t eeprom_cursor = {
                                 .bank = -1,
                                 .page = -1,
                                 .device = -1,
                                 .offset = -1};

/** Internal helper function for media reads */
static bool media_get(
        dev_node_t *node,
        uint_t     filetype,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to convert interget to string Speed */
static const char *int_to_str_speed(sdi_media_speed_t speed);

/** Internal helper function to get the status */
static void media_monitor_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media control status */
static void media_control_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the channel tx control status */
static void media_tx_control_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the channel status */
static void media_channel_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the channel monitor TX bias and RX power */
static void media_channel_monitor_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media parameters */
static void media_parameter_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media vendor information */
static void media_vendor_info_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media threshold information */
static void media_channel_monitor_threshold_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media threshold information */
static void media_monitor_threshold_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media vendor OUI */
static void media_vendor_oui_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media module monitor */
static void media_module_monitor_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media diagnostic mode */
static void media_diag_mode_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media speed */
static void media_speed_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media alias name */
static void media_alias_name_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to get the media presence */
static void media_presence_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to read specific media eeprom data  */
static void media_eeprom_data_read(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to read eeprom cursor  */
static void media_eeprom_cursor_read(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

/** Internal helper function to dump eeprom based on page in cursor  */
static void media_eeprom_page_dump(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        );

typedef struct {

    int  array;
    char *format;
    char *disp_str;

} media_internal_arg_list_t;

struct {

    int  status;
    void (* func) (dev_node_t *node,
                   int        array,
                   char       *format,
                   char       *disp_str,
                   char       *trans_buf,
                   size_t     *len,
                   int        *res);

    media_internal_arg_list_t args;

} internal_media_func_tbl_t[] = {

    [FUSE_MEDIA_FILETYPE_PRESENCE] = {
        status  : SUPPORTED,
        func    : media_presence_get,
        args    : {
                    0,
                    "%-25s : %s", "Present" }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_HIGH_ALARM_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_HIGH_ALARM,
                    "%-25s : %s",
                    "Temp High Alarm Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_LOW_ALARM_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_LOW_ALARM,
                    "%-25s : %s",
                    "Temp Low Alarm Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_HIGH_WARNING_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_HIGH_WARNING,
                    "%-25s : %s",
                    "Temp High Warning Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_LOW_WARNING_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_LOW_WARNING,
                    "%-25s : %s",
                    "Temp Low Warning Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_HIGH_ALARM_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_HIGH_ALARM,
                    "%-25s : %s",
                    "Volt High Alarm Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_LOW_ALARM,
                    "%-25s : %s",
                    "Volt Low Alarm Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_HIGH_WARNING_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_HIGH_WARNING,
                    "%-25s : %s",
                    "Volt High Warning Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_LOW_WARNING_STATUS] = {
        status  : SUPPORTED,
        func    : media_monitor_status_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_LOW_WARNING,
                    "%-25s : %s",
                    "Volt Low Warning Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_DISABLE_STATUS] = {
        status  : SUPPORTED,
        func    : media_channel_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TXDISABLE,
                    "%s\n",
                    "Tx Disable Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_FAULT_STATUS] = {
        status  : SUPPORTED,
        func    : media_channel_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TXFAULT,
                    "%s\n",
                    "Tx Fault Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_LOSS_STATUS] = {
        status  : SUPPORTED,
        func    : media_channel_status_get,
        args    : {
                    SDI_MEDIA_STATUS_TXLOSS,
                    "%s\n",
                    "Tx Loss Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_RX_LOSS_STATUS] = {
        status  : SUPPORTED,
        func    : media_channel_status_get,
        args    : {
                    SDI_MEDIA_STATUS_RXLOSS,
                    "%s\n",
                    "Rx Loss Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_CONTROL] = {
        status  : SUPPORTED,
        func    : media_tx_control_status_get,
        args    : {
                    0,
                    "%s\n",
                    "Tx Control Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_SPEED] = {
        status  : SUPPORTED,
        func    : media_speed_get,
        args    : {
                    0,
                    "%-25s : %s",
                    "Media Speed"
                  }
    },

    [FUSE_MEDIA_FILETYPE_WAVELENGTH] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_WAVELENGTH,
                    "%-25s : %d nm",
                    "Wavelength"
                  }
    },

    [FUSE_MEDIA_FILETYPE_WAVELENGTH_TOLERANCE] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_WAVELENGTH_TOLERANCE,
                    "%-25s : %d nm",
                    "Wavelength Tolerance"
                  }
    },

    [FUSE_MEDIA_FILETYPE_MAX_CASE_TEMP] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_WAVELENGTH_TOLERANCE,
                    "%-25s : %d deg C",
                    "Max Casing Temp"
                  }
    },

    [FUSE_MEDIA_FILETYPE_CC_BASE] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_CC_BASE,
                    "%-25s : 0x%02x",
                    "CC Base"
                  }
    },

    [FUSE_MEDIA_FILETYPE_CC_EXT] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_CC_EXT,
                    "%-25s : 0x%02x",
                    "CC Ext"
                  }
    },

    [FUSE_MEDIA_FILETYPE_CONNECTOR] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_CONNECTOR,
                    "%-25s : 0x%02x",
                    "Connector"
                  }
    },

    [FUSE_MEDIA_FILETYPE_ENCODING_BITRATE] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_NM_BITRATE,
                    "%-25s : %d MBits/s",
                    "Bitrate"
                  }
    },

    [FUSE_MEDIA_FILETYPE_IDENTIFIER] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_IDENTIFIER,
                    "%-25s : 0x%x",
                    "Identifier"
                  }
    },

    [FUSE_MEDIA_FILETYPE_EXT_IDENTIFIER] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_EXT_IDENTIFIER,
                    "%-25s : 0x%x",
                    "Ext Id"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LENGTH_SMF_KM] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_LENGTH_SMF_KM,
                    "%-25s : %d km",
                    "Length(SMF)"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LENGTH_OM1] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_LENGTH_OM1,
                    "%-25s : %d m",
                    "Length(OM1)"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LENGTH_OM2] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_LENGTH_OM2,
                    "%-25s : %d m",
                    "Length(OM2)"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LENGTH_OM3] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_LENGTH_OM3,
                    "%-25s : %d 2m",
                    "Length(OM3)"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LENGTH_CABLE_ASSEMBLY] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_LENGTH_CABLE_ASSEMBLY,
                    "%-25s : %d m",
                    "Length(Copper)"
                  }
    },

    [FUSE_MEDIA_FILETYPE_DIAG_MON_TYPE] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_DIAG_MON_TYPE,
                    "%-25s : %d",
                    "Diag Mon Type"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LENGTH_SMF] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_LENGTH_SMF,
                    "%-25s : %d m",
                    "Length SMF 1m"
                  }
    },

    [FUSE_MEDIA_FILETYPE_OPTIONS] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_OPTIONS,
                    "%-25s : %d",
                    "Options"
                  }
    },

    [FUSE_MEDIA_FILETYPE_ADVANCED_OPTIONS] = {
        status  : SUPPORTED,
        func    : media_parameter_get,
        args    : {
                    SDI_MEDIA_ENHANCED_OPTIONS,
                    "%-25s : %d",
                    "Advanced Options"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VENDOR_NAME] = {
        status  : SUPPORTED,
        func    : media_vendor_info_get,
        args    : {
                    SDI_MEDIA_VENDOR_NAME,
                    "%-25s : %s",
                    "Vendor Name"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VENDOR_OUI] = {
        status  : SUPPORTED,
        func    : media_vendor_oui_get,
        args    : {
                    SDI_MEDIA_VENDOR_OUI,
                    "%-25s : 0x%02x : 0x%02x",
                    "Vendor OUI"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VENDOR_SN] = {
        status  : SUPPORTED,
        func    : media_vendor_info_get,
        args    : {
                    SDI_MEDIA_VENDOR_SN,
                    "%-25s : %s",
                    "Vendor SN"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VENDOR_DATE] = {
        status  : SUPPORTED,
        func    : media_vendor_info_get,
        args    : {
                    SDI_MEDIA_VENDOR_DATE,
                    "%-25s : %s",
                    "Vendor Datecode"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VENDOR_PN] = {
        status  : SUPPORTED,
        func    : media_vendor_info_get,
        args    : {
                    SDI_MEDIA_VENDOR_PN,
                    "%-25s : %s",
                    "Vendor PN"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VENDOR_REVISION] = {
        status  : SUPPORTED,
        func    : media_vendor_info_get,
        args    : {
                    SDI_MEDIA_VENDOR_REVISION,
                    "%-25s : %s",
                    "Vendor Rev"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_HIGH_ALARM_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_HIGH_ALARM,
                    "%-30s : %d V",
                    "Volt High Alarm threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_LOW_ALARM_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_LOW_ALARM,
                    "%-30s : %d V",
                    "Volt Low Alarm threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_HIGH_WARNING_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_HIGH_WARNING,
                    "%-30s : %d V",
                    "Volt High Warning threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT_LOW_WARNING_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_VOLT_LOW_WARNING,
                    "%-30s : %d V",
                    "Volt Low Warning threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_HIGH_ALARM_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_HIGH_ALARM,
                    "%-30s : %d deg C",
                    "Temp High Alarm threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_LOW_ALARM_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_LOW_ALARM,
                    "%-30s : %d deg C",
                    "Temp Low Alarm threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_HIGH_WARNING_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_HIGH_WARNING,
                    "%-30s : %d deg C",
                    "Temp High Warning threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMP_LOW_WARNING_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TEMP_LOW_WARNING,
                    "%-30s : %d deg C",
                    "Temp Low Warning threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_DISABLE_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_channel_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TXDISABLE,
                    "%-30s : %d mA",
                    "TX Disable threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_FAULT_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_channel_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TXFAULT,
                    "%-30s : %d mA",
                    "TX Fault threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TX_LOSS_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_channel_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TXLOSS,
                    "%-30s : %d mA",
                    "TX Loss threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_RX_LOSS_THRESHOLD] = {
        status  : SUPPORTED,
        func    : media_channel_monitor_threshold_get,
        args    : {
                    SDI_MEDIA_STATUS_TXLOSS,
                    "%-30s : %d dBm",
                    "RX Loss threshold"
                  }
    },

    [FUSE_MEDIA_FILETYPE_LP_MODE] = {
        status  : SUPPORTED,
        func    : media_control_status_get,
        args    : {
                    SDI_MEDIA_LP_MODE,
                    "%-25s : %s",
                    "Low Power Mode"
                  }
    },

    [FUSE_MEDIA_FILETYPE_RESET] = {
        status  : SUPPORTED,
        func    : media_control_status_get,
        args    : {
                    SDI_MEDIA_RESET,
                    "%-25s : %s",
                    "Reset Status"
                  }
    },

    [FUSE_MEDIA_FILETYPE_VOLT] = {
        status  : SUPPORTED,
        func    : media_module_monitor_get,
        args    : {
                    SDI_MEDIA_VOLT,
                    "%-25s : %.6f V",
                    "Voltage"
                  }
    },

    [FUSE_MEDIA_FILETYPE_TEMPERATURE] = {
        status  : SUPPORTED,
        func    : media_module_monitor_get,
        args    : {
                    SDI_MEDIA_TEMP,
                    "%-25s : %.6f deg C",
                    "Temperature"
                  }
    },

    [FUSE_MEDIA_FILETYPE_INTERNAL_RX_POWER_MONITOR] = {
        status  : SUPPORTED,
        func    : media_channel_monitor_get,
        args    : {
                    SDI_MEDIA_INTERNAL_TX_POWER_BIAS,
                    "%s\n",
                    "Internal TX Power Bias (mA)\n"
                  }
    },

    [FUSE_MEDIA_FILETYPE_INTERNAL_TX_POWER_BIAS] = {
        status  : SUPPORTED,
        func    : media_channel_monitor_get,
        args    : {
                    SDI_MEDIA_INTERNAL_RX_POWER_MONITOR,
                    "%s\n",
                    "Internal RX Power Monitor (dBm)\n"
                  }
    },

    [FUSE_MEDIA_FILETYPE_ALIAS] = {
        status  : SUPPORTED,
        func    : media_alias_name_get,
        args    : {
                    0,
                    "%-25s : %s",
                    "Alias"
                  }
    },

    [FUSE_MEDIA_FILETYPE_DIAG_MODE] = {
        status  : SUPPORTED,
        func    : media_diag_mode_get,
        args    : {
                    0,
                    "%-25s : %s",
                    "Diagnostic Mode"
                  }
    },

    [FUSE_MEDIA_FILETYPE_EEPROM_CURSOR] = {
        status  : SUPPORTED,
        func    : media_eeprom_cursor_read,
        args    : {
                    0,
                    "%-25s : Device 0x%x, Bank 0x%x, Page 0x%x, Offset %d",
                    "EEPROM Cursor"
                  }
    },

    [FUSE_MEDIA_FILETYPE_EEPROM_DATA] = {
        status  : SUPPORTED,
        func    : media_eeprom_data_read,
        args    : {
                    0,
                    "%-25s : 0x%02x 0d%03d 0b%s 0c%c",
                    "EEPROM Data"
                  }
    },

    [FUSE_MEDIA_FILETYPE_EEPROM_PAGE_DUMP] = {
        status  : SUPPORTED,
        func    : media_eeprom_page_dump,
        args    : {
                    0,
                    "",
                    "EEPROM Page Dump"
                  }
    },

};

/** PAS Daemon entity_info read interface */
int dn_pas_fuse_media_read(
        dev_node_t *node,
        char       *buf,
        size_t     size,
        off_t      offset
        )
{
    int    res                               = -ENOTSUP;
    size_t len                               = 0;
    char   trans_buf[FUSE_FILE_DEFAULT_SIZE] = { 0 };
    memset(buf, 0, size);

    /** check for node & buffer validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    /** get the media attribute from SDI */
    if(media_get(node, node->fuse_filetype, trans_buf, &len, &res)) {

        /** check if we want to read at an offset */
        if (offset < len) {
            size = dn_pas_fuse_calc_size(len, size, offset);
            memcpy(buf, &trans_buf[offset], size);
            res = size;
        }
    }

    return res;
}

/** PAS Daemon entity_info write interfthe */
int dn_pas_fuse_media_write(
        dev_node_t *node,
        const char *buf,
        size_t     size,
        off_t      offset
        )
{
    //write permission denied
    int    res   = -ENOTSUP;
    int    ret = 0;
    uint_t data = 0;
    uint8_t write_data = 0;
    uint_t state = 0;
    sdi_media_eeprom_addr_t addr = {
                                    .device_addr = SDI_MEDIA_DEVICE_ADDR_AUTO,
                                    .page        = SDI_MEDIA_PAGE_SELECT_NOT_SUPPORTED,
                                    .offset      = 0
                                   };

    /** check for node & buffer validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    /** switch on filetype */
    switch (node->fuse_filetype) {

        /** diagnostic mode */
        case FUSE_MEDIA_FILETYPE_DIAG_MODE:
            {
                if (dn_pas_fuse_atoui(buf, &state)) {

                    switch(state) {
                        case 1  : /** TODO: define variable and set to true */
                                  res = size;
                                  break;

                        case 0  : /** TODO: define variable and set to false */
                                  res = size;
                                  break;

                        default : res = -EINVAL;
                                  break;
                    }

                } else res = -EINVAL;

                break;
            }

        case FUSE_MEDIA_FILETYPE_EEPROM_CURSOR:

            /* Cursor is csv of format: "device,bank,page,offset" . If field is N/A, set as -1 */
            /* device, bank, page are in hexadecimal. Offset is in decimal */
            /* will handle potential errors in string input */
            ret = sscanf(buf, "%x,%x,%x,%d", &(eeprom_cursor.device)
                                           , &(eeprom_cursor.bank)
                                           , &(eeprom_cursor.page)
                                           , &(eeprom_cursor.offset));
            if (ret != 4){
                res = -EINVAL;
            } else {
                res = size;
            }
            break;

        case FUSE_MEDIA_FILETYPE_EEPROM_DATA:

            /* banks not yet implemented */
            addr.device_addr = eeprom_cursor.device;
            addr.page        = eeprom_cursor.page;
            addr.offset      = eeprom_cursor.offset;

            ret = sscanf(buf, "%u", &data);

            write_data = (uint8_t)data;
            /*Need to test validity of eeprom_cursor */
            if ((addr.offset > (EEPROM_PAGE_MAX_SIZE-1)) || (ret !=1)) {
                res = -EINVAL;
                break;
            }

            if (STD_ERR_OK !=
                sdi_media_write_generic(node->fuse_resource_hdl,
                   &addr, &write_data, sizeof(uint8_t))) {
                res = -EPERM;
            } else {
                res = size;
            }
           break;
        default:
            {
                if(node->fuse_filetype >= FUSE_MEDIA_FILETYPE_MIN &&
                   node->fuse_filetype < FUSE_MEDIA_FILETYPE_MAX) {

                    res = -EPERM;
                } else {

                    res = -ENOENT;
                }
                break;
            }
    }

    return res;
}

static const char *int_to_str_speed(sdi_media_speed_t speed)
{
    switch (speed) {
        case SDI_MEDIA_SPEED_1G:
            return "1 Gbps";
        case SDI_MEDIA_SPEED_10G:
            return "10 Gbps";
        case SDI_MEDIA_SPEED_25G:
            return "25 Gbps";
        case SDI_MEDIA_SPEED_40G:
            return "40 Gbps";
        default:
            return "NA";
    }
}

/** Internal helper function to get the monitor information */
static void media_channel_monitor_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    float  value                            = 0;
    int    channel                          = 0;
    char   temp_buf[FUSE_FILE_DEFAULT_SIZE] = { 0 };

    snprintf(trans_buf, FUSE_FILE_DEFAULT_SIZE, format, disp_str);

    for (channel = 0; channel <= 3; ++channel) {

        if (STD_ERR_OK ==
                sdi_media_channel_monitor_get(node->fuse_resource_hdl, channel, array, &value)) {

            snprintf(temp_buf, FUSE_FILE_DEFAULT_SIZE, "%-5s %d : %.6f\n",
                    "Channel", channel, value);

            if((strlen(trans_buf) + strlen(temp_buf)) < FUSE_FILE_DEFAULT_SIZE) {

                strncat(trans_buf, temp_buf, FUSE_FILE_DEFAULT_SIZE - strlen(trans_buf));
            }

        } else {

            snprintf(temp_buf, FUSE_FILE_DEFAULT_SIZE, "%-5s %d : %s\n",
                    "Channel", channel, "NA");

            if((strlen(trans_buf) + strlen(temp_buf)) < FUSE_FILE_DEFAULT_SIZE) {

                strncat(trans_buf, temp_buf, FUSE_FILE_DEFAULT_SIZE - strlen(trans_buf));
            }
        }
    }

    *len = strlen(trans_buf) + 1;
    *res = *len;
}

/** Internal helper function to get the media monitor status */
static void media_monitor_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    uint_t status = 0;

    if (STD_ERR_OK ==
            sdi_media_module_monitor_status_get(node->fuse_resource_hdl,
                array,
                &status)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE,
                len, res , format,
                disp_str , status ? "True" : "False");
    }
}

/** Internal helper function to get the media parameters */
static void media_parameter_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    uint_t value = 0;
    t_std_error err = STD_ERR_OK;

    if (STD_ERR_OK ==
            (err = sdi_media_parameter_get(node->fuse_resource_hdl,
                                           array,
                                           &value))) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE,
                len, res, format, disp_str, value);

    } else if (STD_ERR_EXT_PRIV(err) == ENOTSUP) {

        *res = -ENOTSUP;
    }
}

/** Internal helper function to get the media control status */
static void media_control_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    bool status = 0;

    if (STD_ERR_OK ==
            sdi_media_module_control_status_get(node->fuse_resource_hdl,
                array, &status)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format, disp_str, status ? "True" : "False");
    }
}


/** Internal helper function to get the media vendor information */
static void media_vendor_info_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{

    char temp_buf[FUSE_FILE_DEFAULT_SIZE];

    if (STD_ERR_OK == sdi_media_vendor_info_get(node->fuse_resource_hdl, array,
                (char *) &temp_buf,
                FUSE_FILE_DEFAULT_SIZE)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len,
                res, format, disp_str, temp_buf);
    }
}

/** Internal helper function to get the media threshold information */
static void media_channel_monitor_threshold_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    uint_t threshold = 0;

    if (STD_ERR_OK ==
            sdi_media_channel_monitor_threshold_get(node->fuse_resource_hdl,
                array, &threshold)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format, disp_str, threshold);
    }
}

/** Internal helper function to get the media tx control status */
static void media_tx_control_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    bool   status                           = 0;
    uint_t channel                          = 0;
    char   temp_buf[FUSE_FILE_DEFAULT_SIZE] = { 0 };

    snprintf(trans_buf, FUSE_FILE_DEFAULT_SIZE, format, disp_str);

    for (channel = 0; channel <= 3; ++channel) {

        if (STD_ERR_OK ==
                sdi_media_tx_control_status_get(node->fuse_resource_hdl, channel, &status)) {

            snprintf(temp_buf, FUSE_FILE_DEFAULT_SIZE, "%-5s %d : %s\n",
                    "Channel", channel, status ? "True" : "False");

            if((strlen(trans_buf) + strlen(temp_buf)) < FUSE_FILE_DEFAULT_SIZE) {

                strncat(trans_buf, temp_buf, FUSE_FILE_DEFAULT_SIZE - strlen(trans_buf));
            }

        } else {

            snprintf(temp_buf, FUSE_FILE_DEFAULT_SIZE, "%-5s %d : %s\n",
                    "Channel", channel, "NA");

            if((strlen(trans_buf) + strlen(temp_buf)) < FUSE_FILE_DEFAULT_SIZE) {

                strncat(trans_buf, temp_buf, FUSE_FILE_DEFAULT_SIZE - strlen(trans_buf));
            }
        }
    }

    *len = strlen(trans_buf) + 1;
    *res = *len;
}

/** Internal helper function to get the channel status */
static void media_channel_status_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    uint_t status                           = 0;
    int    channel                          = 0;
    char   temp_buf[FUSE_FILE_DEFAULT_SIZE] = { 0 };

    snprintf(trans_buf, FUSE_FILE_DEFAULT_SIZE, format, disp_str);

    for (channel = 0; channel <= 3; ++channel) {

        if (STD_ERR_OK ==
                sdi_media_channel_status_get(node->fuse_resource_hdl, channel, array, &status)) {

            snprintf(temp_buf, FUSE_FILE_DEFAULT_SIZE, "%-5s %d : %s\n",
                    "Channel", channel, status ? "True" : "False");

            if((strlen(trans_buf) + strlen(temp_buf)) < FUSE_FILE_DEFAULT_SIZE) {

                strncat(trans_buf, temp_buf, FUSE_FILE_DEFAULT_SIZE - strlen(trans_buf));
            }

        } else {

            snprintf(temp_buf, FUSE_FILE_DEFAULT_SIZE, "%-5s %d : %s\n",
                    "Channel", channel, "NA");

            if((strlen(trans_buf) + strlen(temp_buf)) < FUSE_FILE_DEFAULT_SIZE) {

                strncat(trans_buf, temp_buf, FUSE_FILE_DEFAULT_SIZE - strlen(trans_buf));
            }
        }
    }

    *len = strlen(trans_buf) + 1;
    *res = *len;
}

/** Internal helper function to get the media threshold information */
static void media_monitor_threshold_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    uint_t value = 0;

    if (STD_ERR_OK ==
            sdi_media_module_monitor_threshold_get(node->fuse_resource_hdl,
                array, &value)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len,
                res, format, disp_str, value);
    }
}

/** Internal helper function to get the media vendor OUI */
static void media_vendor_oui_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res)
{
    char temp_buf[FUSE_FILE_DEFAULT_SIZE];
    if (STD_ERR_OK ==
            sdi_media_vendor_info_get(node->fuse_resource_hdl,
                SDI_MEDIA_VENDOR_OUI, (char *) &temp_buf, FUSE_FILE_DEFAULT_SIZE)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format,
                disp_str,
                temp_buf[0],
                temp_buf[1],
                temp_buf[2]);
    }
}

/** Internal helper function to get the media voltage */
static void media_module_monitor_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    float value = 0;

    if (STD_ERR_OK ==
            sdi_media_module_monitor_get(node->fuse_resource_hdl,
                array, &value)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format, disp_str, value);
    }
}

/** Internal helper function to get the media diagnostic mode */
static void media_diag_mode_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
            format, disp_str,
            (dn_pald_diag_mode_get() ? "up" : "down"));
}

/** Internal helper function to get the media speed */
static void media_speed_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    sdi_media_speed_t speed = 0;

    if (STD_ERR_OK ==
            sdi_media_speed_get(node->fuse_resource_hdl, &speed)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format, disp_str, int_to_str_speed(speed));
    }
}

/** Internal helper function to get the media alias name */
static void media_alias_name_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    const char *alias_name;

    if (NULL != (alias_name = sdi_resource_alias_get(node->fuse_resource_hdl))) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res, format, disp_str, (char *) alias_name);
    }
}

/** Internal helper function to get the media presence */
static void media_presence_get(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    bool presence = false;

    if (STD_ERR_OK ==
            sdi_media_presence_get(node->fuse_resource_hdl,
                &presence)) {

        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format, disp_str, presence ? "True" : "False");
    }
}


static void media_eeprom_cursor_read(
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{

    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
            format, disp_str, eeprom_cursor.device
                            , eeprom_cursor.bank
                            , eeprom_cursor.page
                            , eeprom_cursor.offset);

}

/* converts decimal to binary-looking string */
/* size of buffer 'out' has to be greater than 8 */
static void convert_dec_to_binary_str (uint8_t num, char* out)
{
    short count = 7;
    while(count >= 0) {
        strcat(out, (num >> count--) & 0x01 ? "1" :"0" );
    }
}
static void media_eeprom_data_read (
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{

    uint8_t data = 0;
    char bin_out[10] = {0}; /* holds byte formatted as binary */
    sdi_media_eeprom_addr_t addr = {
                                    .device_addr = SDI_MEDIA_DEVICE_ADDR_AUTO,
                                    .page        = SDI_MEDIA_PAGE_SELECT_NOT_SUPPORTED,
                                    .offset      = 0
                                   };

    /* banks not yet implemented */
    addr.device_addr = eeprom_cursor.device;
    addr.page        = eeprom_cursor.page;
    addr.offset      = eeprom_cursor.offset;

    /*Need to test validity of eeprom_cursor */
    if (addr.offset > (EEPROM_PAGE_MAX_SIZE-1)) {
        return;
    }
    if (STD_ERR_OK ==
            sdi_media_read_generic(node->fuse_resource_hdl,
               &addr, &data, sizeof(data) )){
        convert_dec_to_binary_str(data, bin_out);
        dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, len, res,
                format, disp_str, data, data, bin_out, isprint(data) ? (char)data : '\0');
    }
}


/* Dump is raw data. Use utility such as hexdump to view output */
static void media_eeprom_page_dump (
        dev_node_t *node,
        int        array,
        char       *format,
        char       *disp_str,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    sdi_media_eeprom_addr_t addr = {
                                    .device_addr = SDI_MEDIA_DEVICE_ADDR_AUTO,
                                    .page        = SDI_MEDIA_PAGE_SELECT_NOT_SUPPORTED,
                                    .offset      = 0
                                   };
    /* banks not yet implemented */
    addr.device_addr = eeprom_cursor.device;
    addr.page        = eeprom_cursor.page;

    if (STD_ERR_OK ==
            sdi_media_read_generic(node->fuse_resource_hdl,
               &addr, (uint8_t*)trans_buf, EEPROM_PAGE_MAX_SIZE)){
        *len = EEPROM_PAGE_MAX_SIZE;
        *res = *len;
    }
}


/** Internal helper function for media reads */
static bool media_get(
        dev_node_t *node,
        uint_t     filetype,
        char       *trans_buf,
        size_t     *len,
        int        *res
        )
{
    if(filetype >= FUSE_MEDIA_FILETYPE_MIN
            && filetype < FUSE_MEDIA_FILETYPE_MAX) {

        if(internal_media_func_tbl_t[filetype].status == SUPPORTED) {

            internal_media_func_tbl_t[filetype].func(
                    node,
                    internal_media_func_tbl_t[filetype].args.array,
                    internal_media_func_tbl_t[filetype].args.format,
                    internal_media_func_tbl_t[filetype].args.disp_str,
                    trans_buf,
                    len,
                    res
                    );
            return true;
        }
    }
    return false;
}
