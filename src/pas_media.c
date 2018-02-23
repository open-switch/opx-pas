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
* @file pas_media.c
*
* @brief This file contains source code of physical media handling.
**************************************************************************/

#include "private/pas_log.h"
#include "private/pas_media.h"
#include "private/pas_media_sdi_wrapper.h"
#include "private/pas_data_store.h"
#include "private/pald.h"
#include "private/dn_pas.h"
#include "private/pas_event.h"
#include "private/pas_utils.h"
#include "dn_pas_media_vendor.h"
#include "cps_api_operation.h"
#include "cps_api_service.h"
#include "cps_api_events.h"
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define ARRAY_SIZE(a)         (sizeof(a)/sizeof(a[0]))

#define MEMEBER_SIZEOF(type, member)  (sizeof(((type *)NULL)->member))

#define CPS_API_ATTR_KEY_ID CPS_API_ATTR_RESERVE_RANGE_END
#define SYSTEM_BOARD_SLOT_NUMBER 1

/* Table to hold the sdi handle and media resource pointer and channel data
 * for all the ports*/

static phy_media_tbl_t *phy_media_tbl = NULL;
static uint_t phy_media_count = 0;

/* Mapping of media Attribute and corresponding member of _pas_media_t struct
 * and struct member info
 */

static const phy_media_member_info_t  media_mem_info[] = {

    {BASE_PAS_MEDIA_PRESENT, offsetof(pas_media_t, present),
        MEMEBER_SIZEOF(pas_media_t, present), cps_api_object_ATTR_T_BIN},
    {BASE_PAS_MEDIA_PORT_TYPE, offsetof(pas_media_t, port_type),
        MEMEBER_SIZEOF(pas_media_t, port_type), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_PORT_DENSITY, offsetof(phy_media_tbl_t, port_density),
        MEMEBER_SIZEOF(phy_media_tbl_t, port_density), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CHANNEL_COUNT, offsetof(phy_media_tbl_t, channel_cnt),
        MEMEBER_SIZEOF(phy_media_tbl_t, channel_cnt), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_INSERTION_CNT, offsetof(pas_media_t, insertion_cnt),
        MEMEBER_SIZEOF(pas_media_t, insertion_cnt), cps_api_object_ATTR_T_U64},

    {BASE_PAS_MEDIA_INSERTION_TIMESTAMP,
        offsetof(pas_media_t, insertion_timestamp),
        MEMEBER_SIZEOF(pas_media_t, insertion_timestamp),
        cps_api_object_ATTR_T_U64},

    {BASE_PAS_MEDIA_ADMIN_STATUS, offsetof(pas_media_t, admin_status),
        MEMEBER_SIZEOF(pas_media_t, admin_status), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_OPER_STATUS, offsetof(pas_media_t, oper_status),
        MEMEBER_SIZEOF(pas_media_t, oper_status), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_SUPPORT_STATUS, offsetof(pas_media_t, support_status),
        MEMEBER_SIZEOF(pas_media_t, support_status), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LOCKDOWN_STATE, offsetof(pas_media_t, lockdown_state),
        MEMEBER_SIZEOF(pas_media_t, lockdown_state), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CATEGORY, offsetof(pas_media_t, category),
        MEMEBER_SIZEOF(pas_media_t, category), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_TYPE, offsetof(pas_media_t, type),
        MEMEBER_SIZEOF(pas_media_t, type), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CAPABILITY, offsetof(pas_media_t, capability),
        MEMEBER_SIZEOF(pas_media_t, capability), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_VENDOR_ID, offsetof(pas_media_t, vendor_id),
        MEMEBER_SIZEOF(pas_media_t, vendor_id) - 1, cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_SERIAL_NUMBER, offsetof(pas_media_t, serial_number),
        MEMEBER_SIZEOF(pas_media_t, serial_number), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_QUALIFIED, offsetof(pas_media_t, qualified),
        MEMEBER_SIZEOF(pas_media_t, qualified),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_HIGH_POWER_MODE, offsetof(pas_media_t, high_power_mode),
        MEMEBER_SIZEOF(pas_media_t, high_power_mode),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_IDENTIFIER, offsetof(pas_media_t, identifier),
        MEMEBER_SIZEOF(pas_media_t, identifier), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_EXT_IDENTIFIER, offsetof(pas_media_t, ext_identifier),
        MEMEBER_SIZEOF(pas_media_t, ext_identifier),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CONNECTOR, offsetof(pas_media_t, connector),
        MEMEBER_SIZEOF(pas_media_t, connector), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_TRANSCEIVER, offsetof(pas_media_t, transceiver),
        MEMEBER_SIZEOF(pas_media_t, transceiver), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_ENCODING, offsetof(pas_media_t, encoding),
        MEMEBER_SIZEOF(pas_media_t, encoding), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_BR_NOMINAL, offsetof(pas_media_t, br_nominal),
        MEMEBER_SIZEOF(pas_media_t, br_nominal), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_RATE_IDENTIFIER, offsetof(pas_media_t, rate_identifier),
        MEMEBER_SIZEOF(pas_media_t, rate_identifier),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LENGTH_SFM_KM, offsetof(pas_media_t, length_sfm_km),
        MEMEBER_SIZEOF(pas_media_t, length_sfm_km), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LENGTH_SFM, offsetof(pas_media_t, length_sfm),
        MEMEBER_SIZEOF(pas_media_t, length_sfm), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LENGTH_OM2, offsetof(pas_media_t, length_om2),
        MEMEBER_SIZEOF(pas_media_t, length_om2), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LENGTH_OM1, offsetof(pas_media_t, length_om1),
        MEMEBER_SIZEOF(pas_media_t, length_om1), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LENGTH_CABLE, offsetof(pas_media_t, length_cable),
        MEMEBER_SIZEOF(pas_media_t, length_cable), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_LENGTH_OM3, offsetof(pas_media_t, length_om3),
        MEMEBER_SIZEOF(pas_media_t, length_om3), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_VENDOR_NAME, offsetof(pas_media_t, vendor_name),
        MEMEBER_SIZEOF(pas_media_t, vendor_name), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_EXT_TRANSCEIVER, offsetof(pas_media_t, ext_transceiver),
        MEMEBER_SIZEOF(pas_media_t, ext_transceiver),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_VENDOR_PN, offsetof(pas_media_t, vendor_pn),
        MEMEBER_SIZEOF(pas_media_t, vendor_pn), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_VENDOR_REV, offsetof(pas_media_t, vendor_rev),
        MEMEBER_SIZEOF(pas_media_t, vendor_rev) - 1, cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_WAVELENGTH, offsetof(pas_media_t, wavelength),
        MEMEBER_SIZEOF(pas_media_t, wavelength), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_MEDIA_CATEGORY_QSFP_PLUS_WAVELENGTH_TOLERANCE,
        offsetof(pas_media_t, wavelength_tolerance),
        MEMEBER_SIZEOF(pas_media_t, wavelength_tolerance),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_MEDIA_CATEGORY_QSFP_PLUS_MAX_CASE_TEMP,
        offsetof(pas_media_t, max_case_temp),
        MEMEBER_SIZEOF(pas_media_t, max_case_temp), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_MEDIA_CATEGORY_SFP_PLUS_BR_MAX,
        offsetof(pas_media_t, br_max), MEMEBER_SIZEOF(pas_media_t, br_max),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_MEDIA_CATEGORY_SFP_PLUS_BR_MIN,
        offsetof(pas_media_t, br_min), MEMEBER_SIZEOF(pas_media_t, br_min),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_MEDIA_CATEGORY_SFP_PLUS_SFF_8472_COMPLIANCE,
        offsetof(pas_media_t, sff_8472_compliance),
        MEMEBER_SIZEOF(pas_media_t, sff_8472_compliance),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CC_BASE, offsetof(pas_media_t, cc_base),
        MEMEBER_SIZEOF(pas_media_t, cc_base), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_OPTIONS, offsetof(pas_media_t, options),
        MEMEBER_SIZEOF(pas_media_t, options), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_DATE_CODE, offsetof(pas_media_t, date_code),
        MEMEBER_SIZEOF(pas_media_t, date_code) - 1, cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_DIAG_MON_TYPE, offsetof(pas_media_t, diag_mon_type),
        MEMEBER_SIZEOF(pas_media_t, diag_mon_type), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_ENHANCED_OPTIONS, offsetof(pas_media_t, enhanced_options),
        MEMEBER_SIZEOF(pas_media_t, enhanced_options),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CC_EXT, offsetof(pas_media_t, cc_ext),
        MEMEBER_SIZEOF(pas_media_t, cc_ext), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_RATE_SELECT_STATE, offsetof(pas_media_t, rate_select_state),
        MEMEBER_SIZEOF(pas_media_t, rate_select_state), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_RX_POWER_MEASUREMENT_TYPE,
        offsetof(pas_media_t, rx_power_measurement_type),
        MEMEBER_SIZEOF(pas_media_t, rx_power_measurement_type),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_TEMP_HIGH_ALARM_THRESHOLD,
        offsetof(pas_media_t, temp_high_alarm),
        MEMEBER_SIZEOF(pas_media_t, temp_high_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TEMP_LOW_ALARM_THRESHOLD,
        offsetof(pas_media_t, temp_low_alarm),
        MEMEBER_SIZEOF(pas_media_t, temp_low_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TEMP_HIGH_WARNING_THRESHOLD,
        offsetof(pas_media_t, temp_high_warning),
        MEMEBER_SIZEOF(pas_media_t, temp_high_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TEMP_LOW_WARNING_THRESHOLD,
        offsetof(pas_media_t, temp_low_warning),
        MEMEBER_SIZEOF(pas_media_t, temp_low_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_VOLTAGE_HIGH_ALARM_THRESHOLD,
        offsetof(pas_media_t, voltage_high_alarm),
        MEMEBER_SIZEOF(pas_media_t, voltage_high_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_VOLTAGE_LOW_ALARM_THRESHOLD,
        offsetof(pas_media_t, voltage_low_alarm),
        MEMEBER_SIZEOF(pas_media_t, voltage_low_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_VOLTAGE_HIGH_WARNING_THRESHOLD,
        offsetof(pas_media_t, voltage_high_warning),
        MEMEBER_SIZEOF(pas_media_t, voltage_high_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_VOLTAGE_LOW_WARNING_THRESHOLD,
        offsetof(pas_media_t, voltage_low_warning),
        MEMEBER_SIZEOF(pas_media_t, voltage_low_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_RX_POWER_HIGH_ALARM_THRESHOLD,
        offsetof(pas_media_t, rx_power_high_alarm),
        MEMEBER_SIZEOF(pas_media_t, rx_power_high_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_RX_POWER_LOW_ALARM_THRESHOLD,
        offsetof(pas_media_t, rx_power_low_alarm),
        MEMEBER_SIZEOF(pas_media_t, rx_power_low_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_RX_POWER_HIGH_WARNING_THRESHOLD,
        offsetof(pas_media_t, rx_power_high_warning),
        MEMEBER_SIZEOF(pas_media_t, rx_power_high_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_RX_POWER_LOW_WARNING_THRESHOLD,
        offsetof(pas_media_t, rx_power_low_warning),
        MEMEBER_SIZEOF(pas_media_t, rx_power_low_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_BIAS_HIGH_ALARM_THRESHOLD,
        offsetof(pas_media_t, bias_high_alarm),
        MEMEBER_SIZEOF(pas_media_t, bias_high_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_BIAS_LOW_ALARM_THRESHOLD,
        offsetof(pas_media_t, bias_low_alarm),
        MEMEBER_SIZEOF(pas_media_t, bias_low_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_BIAS_HIGH_WARNING_THRESHOLD,
        offsetof(pas_media_t, bias_high_warning),
        MEMEBER_SIZEOF(pas_media_t, bias_high_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_BIAS_LOW_WARNING_THRESHOLD,
        offsetof(pas_media_t, bias_low_warning),
        MEMEBER_SIZEOF(pas_media_t, bias_low_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TX_POWER_HIGH_ALARM_THRESHOLD,
        offsetof(pas_media_t, tx_power_high_alarm),
        MEMEBER_SIZEOF(pas_media_t, tx_power_high_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TX_POWER_LOW_ALARM_THRESHOLD,
        offsetof(pas_media_t, tx_power_low_alarm),
        MEMEBER_SIZEOF(pas_media_t, tx_power_low_alarm),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TX_POWER_HIGH_WARNING_THRESHOLD,
        offsetof(pas_media_t, tx_power_high_warning),
        MEMEBER_SIZEOF(pas_media_t, tx_power_high_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TX_POWER_LOW_WARNING_THRESHOLD,
        offsetof(pas_media_t, tx_power_low_warning),
        MEMEBER_SIZEOF(pas_media_t, tx_power_low_warning),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CURRENT_TEMPERATURE,
        offsetof(pas_media_t, current_temperature),
        MEMEBER_SIZEOF(pas_media_t, current_temperature),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CURRENT_VOLTAGE, offsetof(pas_media_t, current_voltage),
        MEMEBER_SIZEOF(pas_media_t, current_voltage),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_TEMP_STATE, offsetof(pas_media_t, temp_state),
        MEMEBER_SIZEOF(pas_media_t, temp_state), cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_VOLTAGE_STATE, offsetof(pas_media_t, voltage_state),
        MEMEBER_SIZEOF(pas_media_t, voltage_state), cps_api_object_ATTR_T_U32}

};

static const phy_media_member_info_t media_ch_mem_info [] = {

    {BASE_PAS_MEDIA_CHANNEL_STATE, offsetof(pas_media_channel_t, state),
        MEMEBER_SIZEOF(pas_media_channel_t, state), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_RX_POWER, offsetof(pas_media_channel_t, rx_power),
        MEMEBER_SIZEOF(pas_media_channel_t, rx_power),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_TX_POWER, offsetof(pas_media_channel_t, tx_power),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_power),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_TX_BIAS_CURRENT,
        offsetof(pas_media_channel_t, tx_bias_current),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_bias_current),
        cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_OPER_STATUS,
        offsetof(pas_media_channel_t, oper_status),
        MEMEBER_SIZEOF(pas_media_channel_t, oper_status),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CHANNEL_RX_LOSS, offsetof(pas_media_channel_t, rx_loss),
        MEMEBER_SIZEOF(pas_media_channel_t, rx_loss), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_TX_LOSS, offsetof(pas_media_channel_t, tx_loss),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_loss), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_TX_FAULT, offsetof(pas_media_channel_t, tx_fault),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_fault), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_TX_DISABLE, offsetof(pas_media_channel_t, tx_disable),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_disable), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_RX_POWER_STATE,
        offsetof(pas_media_channel_t, rx_power_state),
        MEMEBER_SIZEOF(pas_media_channel_t, rx_power_state),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CHANNEL_TX_POWER_STATE,
        offsetof(pas_media_channel_t, tx_power_state),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_power_state),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CHANNEL_TX_BIAS_STATE,
        offsetof(pas_media_channel_t, tx_bias_state),
        MEMEBER_SIZEOF(pas_media_channel_t, tx_bias_state),
        cps_api_object_ATTR_T_U32},

    {BASE_PAS_MEDIA_CHANNEL_CDR_ENABLE, offsetof(pas_media_channel_t, cdr_enable),
        MEMEBER_SIZEOF(pas_media_channel_t, cdr_enable), cps_api_object_ATTR_T_BIN},

    {BASE_PAS_MEDIA_CHANNEL_SPEED, offsetof(pas_media_channel_t, speed),
        MEMEBER_SIZEOF(pas_media_channel_t, speed), cps_api_object_ATTR_T_U32}
};


/* Presence publish attribute list */
/* Note: some other attributes need special adding. 
   This list is not exhaustive */
static const cps_api_attr_id_t pp_list[] = { BASE_PAS_MEDIA_PRESENT,
                                      BASE_PAS_MEDIA_PORT_TYPE,
                                      BASE_PAS_MEDIA_OPER_STATUS,
                                      BASE_PAS_MEDIA_SUPPORT_STATUS,
                                      BASE_PAS_MEDIA_LOCKDOWN_STATE,
                                      BASE_PAS_MEDIA_CATEGORY,
                                      BASE_PAS_MEDIA_TYPE,
                                      BASE_PAS_MEDIA_CAPABILITY,
                                      BASE_PAS_MEDIA_VENDOR_ID,
                                      BASE_PAS_MEDIA_SERIAL_NUMBER,
                                      BASE_PAS_MEDIA_QUALIFIED };

/* static declarations */

static bool dn_media_data_store_init (uint_t count);

static bool dn_pas_media_add_capabilities_to_obj(media_capability_t *cap,
                                                 cps_api_object_t *obj);

static bool dn_pas_media_channel_res_alloc (uint_t slot, uint_t port,
                PLATFORM_MEDIA_CATEGORY_t pcategory);

static void dn_pas_media_channel_res_free (uint_t slot, uint_t port);

static bool dn_pas_media_product_info_poll (uint_t port, cps_api_object_t obj);

static bool dn_pas_media_oir_poll (uint_t port, cps_api_object_t obj);

static bool dn_pas_phy_media_attr_lookup (cps_api_object_t obj,
                BASE_PAS_MEDIA_t attr, BASE_PAS_OBJECTS_t objid);

static bool dn_pas_media_channel_status_poll (uint_t port,
                uint_t channel, uint_t *status);

static bool dn_pas_media_channel_monitor_status_poll (uint_t port,
                uint_t channel, uint_t *status);

static bool dn_pas_media_module_monitor_status_poll(uint_t port,
                uint_t *status);

static void dn_pas_media_int_attr_poll (sdi_resource_hdl_t hdl,
        sdi_media_param_type_t type, uint_t *memp, cps_api_object_t obj,
        BASE_PAS_MEDIA_t attr_id, bool *ret);

static bool dn_pas_media_transceiver_state_set(uint_t port, bool lockdown);

static bool dn_pas_media_capability_poll (uint_t port, cps_api_object_t obj);

static bool dn_pas_media_obj_attr_add (phy_media_member_info_t const *memp,
        cps_api_object_t obj, void * res_data);

static bool dn_pas_media_obj_attr_list_add (phy_media_member_info_t *memp,
        cps_api_object_t obj, void * res_data, size_t member_size, int count);

/* Returns whether a port is fixed(false) or pluggable (true)*/
bool dn_pas_is_port_pluggable(uint_t port)
{

    return (dn_pas_config_media_get()->port_info_tbl[port]->port_type
            == PLATFORM_PORT_TYPE_PLUGGABLE);

}

bool dn_pas_media_compare_capabilities(const media_capability_t* const cap1,
                                       const media_capability_t* const cap2) 
{
    return (cap1->media_speed == cap2->media_speed)
           && (cap1->breakout_speed == cap2->breakout_speed)
           && (cap1->breakout_mode == cap2->breakout_mode)
           && (cap1->phy_mode == cap2->phy_mode);
}

media_capability_t* dn_pas_media_get_default_media_capability(
                                          phy_media_tbl_t *mtbl)
{
    STD_ASSERT(mtbl != NULL);
    return (mtbl->res_data->default_media_capability_index <
                                  PAS_MEDIA_MAX_CAPABILITIES)
           ? &(mtbl->res_data->media_capabilities[
                 mtbl->res_data->default_media_capability_index])
           : NULL;
}
/* Generate a string to be used to refer to ports(s) when displaying/logging */
/* This is in response to the double density port numbering disparity */
/* For example port with density 2 and sub_port_ids {10,11} will yield port_str "10,11" */
char* dn_pas_media_generate_port_str(phy_media_tbl_t *mtbl)
{
    size_t count = 0;
    size_t sub_str_size = 0;
    char* str_end = 0;

    STD_ASSERT(mtbl != NULL);
    if (mtbl->port_density == 0){
        PAS_ERR("Port density is 0 when constructing port string: %u",
            mtbl->fp_port);
        mtbl->port_density = PAS_MEDIA_PORT_DENSITY_DEFAULT;
    }
    str_end = mtbl->port_str;

    /* Each port string will have at least one number */
    do {
        /* Get number of bytes needed to store the number as string*/
        sub_str_size = snprintf(NULL, 0, "%d", mtbl->sub_port_ids[count]);

        if (((str_end - mtbl->port_str) + sub_str_size) >
             PAS_MEDIA_PORT_STR_BUF_LEN){
            PAS_ERR("Potential buffer overflow when constructing port_str");
            break;
        }
        /* Then convert number to string */
        snprintf(str_end, sub_str_size + 1, "%d", mtbl->sub_port_ids[count]);

        /* Append null terminator */
        *(str_end += sub_str_size) = '\0';

     /* If there are more sub port ids and buffer  space then append a comma and loop */
    } while ((++count < mtbl->port_density)
              && (strlen(mtbl->port_str) < PAS_MEDIA_PORT_STR_BUF_LEN)
              && strcpy(str_end ,","));

    return mtbl->port_str;
}

bool dn_pas_is_media_present(uint_t port)
{
    return (phy_media_tbl[port].res_data == NULL) 
           ? false : phy_media_tbl[port].res_data->present;
}

/*
 * To get pointer to a media entry in a phy_media_tbl by specifying the
 * port, this function validates port and media table initialization, if
 * media is not initialized it return NULL otherwise returns valid pointer
 * to a media entry in the table.
 */


phy_media_tbl_t * dn_phy_media_entry_get(uint_t port)
{
    if (port < PAS_MEDIA_START_PORT || port > phy_media_count) {
        PAS_ERR("Invalid port number (%u)", port);

        return NULL;
    }

    if (phy_media_tbl[port].res_data == NULL) {
        PAS_ERR("Not initialized");

        return NULL;
    }

    return (&phy_media_tbl[port]);
}

/* dn_phy_is_media_channel_valid validates is the channel is valied for the
 * specified port or not. it returns true if its a valid channel otherwise
 * it returns false
 */

static bool dn_phy_is_media_channel_valid (uint_t port, uint_t channel)
{
    phy_media_tbl_t       *mtbl = NULL;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (mtbl->channel_data == NULL)) {
        return false;
    }

    if (channel >= mtbl->channel_cnt) {
        return false;
    }

    return true;
}

/* Returns total media count, phy_media_count */

uint_t dn_phy_media_count_get (void)
{
    return phy_media_count;
}

/*
 * dn_media_obj_publish to publish the media object
 */

static bool dn_media_obj_publish (cps_api_object_t obj)
{

    if (obj == CPS_API_OBJECT_NULL) {
        PAS_ERR("No object");

        return false;
    }

    return (dn_pas_cps_notify(obj));
}

/*
 *
 * Call back function to learn the media resource handles.
 *
 */

static void dn_pas_media_resource_cb (sdi_resource_hdl_t hdl, void *user_data)
{
    static uint_t count_pluggable_from_sdi = 0;
    uint_t count_pluggable_from_config =
        dn_pas_config_media_get()->pluggable_media_count;
    uint_t counter = PAS_MEDIA_START_PORT;

    STD_ASSERT(hdl != NULL);



    if (sdi_resource_type_get(hdl) == SDI_RESOURCE_MEDIA) {


        STD_ASSERT(phy_media_tbl != NULL);

        for (; counter <= phy_media_count; counter++)
            if (dn_pas_is_port_pluggable(counter)
                && (phy_media_tbl[counter].res_hdl == NULL)) {
                    phy_media_tbl[counter].res_hdl = hdl;
                count_pluggable_from_sdi++;
                    break;
            }
    }

    if ((count_pluggable_from_sdi != count_pluggable_from_config)
       && (counter == phy_media_count) ) {
        PAS_ERR("Disparity between pluggable media count from SDI: %u and config: %u",
            count_pluggable_from_sdi, count_pluggable_from_config);
    }

    return;
}

/*
 * PAS media module initialization
 */

bool dn_pas_phy_media_init (void)
{

    bool                    ret = true;
    struct pas_config_media *cfg = dn_pas_config_media_get();
    sdi_entity_hdl_t        entity_hdl;

    if (cfg->port_count == 0) {
        PAS_ERR("Media count is 0. Media functionality not initialized.");
        return true;
    }
    phy_media_count = cfg->port_count;

    /* Alloc +1 for easier indexing*/
    phy_media_tbl = calloc(phy_media_count + 1, sizeof(phy_media_tbl_t));

    /* SYSTEM_BOARD_SLOT_NUMBER is hardcoded for now*/
    entity_hdl = sdi_entity_lookup(SDI_ENTITY_SYSTEM_BOARD, SYSTEM_BOARD_SLOT_NUMBER );

    STD_ASSERT(entity_hdl != NULL);

    /* Get the media resource handles*/
    sdi_entity_for_each_resource(entity_hdl, dn_pas_media_resource_cb,
                (void*)cfg);

    ret = dn_media_data_store_init(phy_media_count);

    return ret;
    }

static void dn_pas_media_generate_sub_ports(struct pas_config_media* cfg_media)
{
    uint_t next_logical_port = PAS_MEDIA_START_PORT;
    uint_t count             = PAS_MEDIA_START_PORT;

    while (count <= cfg_media->port_count){

        uint_t density = 0;
        while (density < cfg_media->port_info_tbl[count]->port_density) {

            phy_media_tbl[count].sub_port_ids[density++] = next_logical_port++;
    }

        count++;
    }


}

/*
 * PAS media module data store initialization
 */

static bool dn_media_data_store_init (uint_t count)
{

    uint_t               cnt;
    uint_t               slot = PAS_MEDIA_MY_SLOT; // My slot
    bool                 ret = true;
    pas_media_t          *ptr = NULL;

    if (count < PAS_MEDIA_START_PORT) {
        PAS_ERR("Attempt to access invalid port: %u", count);
        return false;
    }

    if ((ptr = calloc(count, sizeof(pas_media_t))) == NULL) {
        PAS_ERR("Failed to allocate storage");

        return false;
    }


    struct pas_config_media *cfg = dn_pas_config_media_get();

    for (cnt = PAS_MEDIA_START_PORT; cnt <= count; cnt++) {

        phy_media_tbl[cnt].res_data = ptr++;
        phy_media_tbl[cnt].fp_port = cnt;
        phy_media_tbl[cnt].port_density = cfg->port_info_tbl[cnt]->port_density;
        phy_media_tbl[cnt].res_data->lockdown_state = false;

        char res_key[PAS_RES_KEY_SIZE];

        if (dn_pas_res_insertc(dn_pas_res_key_media(res_key,
                                                    sizeof(res_key),
                                                    slot,
                                                    cnt
                                                    ),
                               phy_media_tbl[cnt].res_data) == false
            ) {
            PAS_ERR("Failed to insert into cache, port %u", cnt);

            ret = false;
            continue;
        }

        dn_pas_media_generate_sub_ports(cfg);

        if (dn_pas_media_generate_port_str(&phy_media_tbl[cnt]) ==  NULL) {
            PAS_ERR("Unable to generate port string");
        }

        // Set dafault parameters if not pollable/pluggable
        if (dn_pas_is_port_pluggable(cnt) == false) {
            phy_media_tbl[cnt].res_data->port_type    = cfg->port_info_tbl[cnt]->port_type;
            phy_media_tbl[cnt].res_data->category     = cfg->port_info_tbl[cnt]->category;
            phy_media_tbl[cnt].res_data->type         = cfg->port_info_tbl[cnt]->media_type;
            phy_media_tbl[cnt].res_data->capability   = cfg->port_info_tbl[cnt]->speed;
            phy_media_tbl[cnt].max_port_speed         = cfg->port_info_tbl[cnt]->speed;
            phy_media_tbl[cnt].res_data->present      = cfg->port_info_tbl[cnt]->present;
            phy_media_tbl[cnt].res_data->qualified = true;
            }

        else {
            sdi_media_speed_t                    speed = 0;
            phy_media_tbl[cnt].res_data->port_type     = PLATFORM_PORT_TYPE_PLUGGABLE;
            phy_media_tbl[cnt].res_data->type =
            PLATFORM_MEDIA_TYPE_AR_POPTICS_NOTPRESENT;
            dn_pas_media_capability_poll((cnt), NULL);


            /* The first speed obtained is the port speed */
            if (pas_sdi_media_speed_get(phy_media_tbl[cnt].res_hdl, &speed)
                    != STD_ERR_OK) {
                PAS_ERR("Failed to get port speed, port %u", cnt);
                return false;
            }
            phy_media_tbl[cnt].max_port_speed = dn_pas_capability_conv(speed);

        phy_media_tbl[cnt].res_data->polling_count =
            (cnt % cfg->rtd_interval) + 1;
        }
    }

    return ret;
}

bool dn_pas_media_add_port_info_to_cps_obj(cps_api_object_t obj,
            phy_media_tbl_t* mtbl, uint_t port) {

    phy_media_member_info_t temp =
        {ident: BASE_PAS_MEDIA_PORT_DENSITY,
         offset: offsetof(phy_media_tbl_t, port_density),
         size:   MEMEBER_SIZEOF(phy_media_tbl_t, port_density),
         type:   cps_api_object_ATTR_T_U32};

    if (dn_pas_media_obj_attr_add(&temp, obj, mtbl) == false) {
        PAS_ERR("Failed to add attribute to object: port density");
        return false;
    }

    /* This is for a list. Be sure to use list adding function */
    temp.ident  = BASE_PAS_MEDIA_SUB_PORT_ID;
    temp.offset = offsetof(phy_media_tbl_t, sub_port_ids[0]);
    temp.size   = MEMEBER_SIZEOF(phy_media_tbl_t, sub_port_ids[0]);
    temp.type   = cps_api_object_ATTR_T_U32;

    if (dn_pas_media_obj_attr_list_add (&temp, obj, mtbl,
               sizeof (mtbl->sub_port_ids[0]), (int)(mtbl->port_density)) == false) {
        PAS_ERR("Failed to add attribute list, sub_port ids, port %u", port);
        return false;
    }

    return true;
}

bool dn_pas_media_add_channel_info_to_cps_obj(cps_api_object_t obj,
            phy_media_tbl_t* mtbl, uint_t port) {

    static const phy_media_member_info_t temp =
        {ident: BASE_PAS_MEDIA_CHANNEL_COUNT,
         offset: offsetof(phy_media_tbl_t, channel_cnt),
         size:   MEMEBER_SIZEOF(phy_media_tbl_t, channel_cnt),
         type:   cps_api_object_ATTR_T_U32};

    if (dn_pas_media_obj_attr_add(&temp, obj, mtbl) == false) {
        PAS_ERR("Failed to add attribute to object: channel count, on port %u", port);
        return false;
    }

    return true;
}
/*dn_pas_media_data_publish creates and populates an object which need to be
 * published as based on presence state and list of attributes. This function
 * can be called by get handler or poller or media init. If handler is true
 * it returns the object.
 */

cps_api_object_t  dn_pas_media_data_publish (uint_t port,
        cps_api_attr_id_t const *list, uint_t count,
        bool handler)
{
    phy_media_tbl_t          *mtbl = NULL;
    cps_api_object_t         obj = CPS_API_OBJECT_NULL;
    uint_t                   slot;


    mtbl = dn_phy_media_entry_get(port);

    STD_ASSERT(mtbl != NULL);

    obj = cps_api_object_create();

    if (obj == CPS_API_OBJECT_NULL) return CPS_API_OBJECT_NULL;

    do {

        if (dn_pas_phy_media_is_present(port) == true) {
            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), list, count, obj,
                        mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, media present, port %u",
                        port
                        );

                break;
            }

            if (dn_pas_media_add_port_info_to_cps_obj(obj,mtbl,port)==false){
                break;
            }

            if (dn_pas_media_add_channel_info_to_cps_obj(
                                           obj,mtbl,port) == false) {
                break;
            }
             /* Add default media capabilities */
            if (!dn_pas_media_add_capabilities_to_obj(
                   dn_pas_media_get_default_media_capability(mtbl), obj)){
                PAS_ERR("Failed to add default capability attrs, port %s",
                        mtbl->port_str
                        );
                break;
            }
        } else {
            cps_api_attr_id_t attr = BASE_PAS_MEDIA_PRESENT;
            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                        mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, media absent, port %u",
                        port
                        );

                break;
            }
            attr = BASE_PAS_MEDIA_PORT_TYPE;

            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                        mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, port type , port %u",
                        port
                        );

                break;
            }

            attr = BASE_PAS_MEDIA_TYPE;

            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                        mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, media absent, port %u",
                        port
                        );

                break;
            }

            attr = BASE_PAS_MEDIA_CAPABILITY;

            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                         mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, media absent, port %u",
                        port);
                break;
            }

            attr = BASE_PAS_MEDIA_VENDOR_ID;

            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                         mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, vendor ID, port %u",
                        port);
                break;
            }

            attr = BASE_PAS_MEDIA_SERIAL_NUMBER;

            if (dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                         mtbl->res_data) == false) {
                PAS_ERR("Failed to add attribute list, serial number, port %u",
                        port);
                break;
            }
            
            if (dn_pas_media_add_port_info_to_cps_obj(obj, mtbl, port)==false) {
                break;
            }
        }

        if (handler == true) return obj;

        dn_pas_myslot_get(&slot);

        dn_pas_obj_key_media_set(obj, cps_api_qualifier_OBSERVED, true, slot,
                false, PAS_MEDIA_INVALID_PORT_MODULE, true, port);

        dn_media_obj_publish(obj);

        obj = CPS_API_OBJECT_NULL;

    } while (0);

    if (obj != CPS_API_OBJECT_NULL) {
        cps_api_object_delete(obj);
        obj = CPS_API_OBJECT_NULL;
    }

    return obj;
}

/*
 * To allocate and initilize resources for media channels.
 */

static bool dn_pas_media_channel_res_alloc (uint_t slot, uint_t port,
        PLATFORM_MEDIA_CATEGORY_t pcategory)
{

    uint_t                count, channel;
    pas_media_channel_t   *ch_data;
    bool                  ret = true;

    count = dn_pas_media_channel_count_get(pcategory);

    if (port < PAS_MEDIA_START_PORT) {
        PAS_ERR("Invalid port id, port(%u).",
                port);
        return false;
    }

    if (count > 0) {

        ch_data = calloc(count, sizeof(pas_media_channel_t));

        if (ch_data == NULL) {
            PAS_ERR("Failed to allocate storage, port %u", port);

            return false;
        }

        for (channel = PAS_MEDIA_CH_START; channel < count; channel++) {
            char res_key[PAS_RES_KEY_SIZE];

            if (dn_pas_res_insertc(dn_pas_res_key_media_chan(res_key,
                                                             sizeof(res_key),
                                                             slot,
                                                             port,
                                                             channel
                                                             ),
                                   &ch_data[channel]
                                   ) == false
                ) {
                PAS_ERR("Failed to insert into data store, port %u", port);
                ret = false;
            }
        }

        phy_media_tbl[port].channel_cnt = count;
        phy_media_tbl[port].channel_data = ch_data;
    }

    return ret;
}

/*
 * For freeing the media chaneel resources
 */

static void dn_pas_media_channel_res_free (uint_t slot, uint_t port)
{

    uint_t         count, channel;

    if (port < PAS_MEDIA_START_PORT) {
        PAS_ERR("Invalid port id, port(%u).",
                port);
        return;
    }
    count = phy_media_tbl[port].channel_cnt;

    for (channel = PAS_MEDIA_CH_START; channel < count; channel++) {
        char res_key[PAS_RES_KEY_SIZE];

        dn_pas_res_removec(dn_pas_res_key_media_chan(res_key, sizeof(res_key), slot, port, channel));
    }

    phy_media_tbl[port].channel_cnt = 0;
    if(phy_media_tbl[port].channel_data)
        free(phy_media_tbl[port].channel_data);
    phy_media_tbl[port].channel_data = NULL;
}

/* To poll the media presence flag, If there is any change in presence state
 * it populates the object with new value, so that caller can publish the
 * object
 */

static bool dn_pas_media_presence_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t          *mtbl = NULL;
    uint_t                   slot = PAS_MEDIA_MY_SLOT;
    bool                     presence = false;


    mtbl = dn_phy_media_entry_get(port);

    STD_ASSERT(mtbl != NULL);

    if (pas_sdi_media_presence_get(mtbl->res_hdl, &presence)
            != STD_ERR_OK) {
        PAS_ERR("Failed to get media presence, port %u", port);

        return false;
    }

    if (mtbl->res_data->present != presence) {
        if (presence == false) {
            dn_pas_media_channel_res_free(slot, port);
            memset(mtbl->res_data, 0, sizeof(*(mtbl->res_data)));
            mtbl->res_data->type = PLATFORM_MEDIA_TYPE_AR_POPTICS_NOTPRESENT;
            dn_pas_media_capability_poll(port, NULL);
            mtbl->res_data->port_type = PLATFORM_PORT_TYPE_PLUGGABLE;
        }

        sdi_media_module_init(mtbl->res_hdl, presence);

        mtbl->res_data->present = presence;

        if (obj != NULL) {

            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_PRESENT,
                        &presence, sizeof(presence)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }

    return true;
}



/* To poll the media category attribute, If there is any change in
 * category attribute it populates the object with new value,
 * caller can publish the object if its not empty.
 */

static bool dn_pas_media_category_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t             *mtbl = NULL;
    uint_t                      identifier = 0;
    uint_t                      ext_tran = 0;
    uint_t                      slot = PAS_MEDIA_MY_SLOT;
    PLATFORM_MEDIA_CATEGORY_t   pcategory;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (pas_sdi_media_parameter_get(mtbl->res_hdl, SDI_MEDIA_IDENTIFIER,
                &identifier) != STD_ERR_OK) {
        PAS_ERR("Failed to get media identifier, port %u", port);

        return false;
    }
    mtbl->res_data->identifier = identifier;

    pas_sdi_media_parameter_get(mtbl->res_hdl, SDI_MEDIA_EXT_COMPLIANCE_CODE,
                &ext_tran);

    mtbl->res_data->ext_transceiver = ext_tran;
    pas_sdi_media_parameter_get(mtbl->res_hdl, SDI_FREE_SIDE_DEV_PROP,
            &mtbl->res_data->free_side_dev_prop);

    mtbl->res_data->sdi_resource_hdl = mtbl->res_hdl;
    pcategory = dn_pas_category_get(mtbl->res_data);

    if (mtbl->res_data->category != pcategory) {

        dn_pas_media_channel_res_free(slot, port);
        dn_pas_media_channel_res_alloc(slot, port, pcategory);

        mtbl->res_data->category = pcategory;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CATEGORY,
                        &pcategory, sizeof(pcategory)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }

    return true;
}

static bool dn_pas_media_transceiver_code_poll (uint_t port,
        cps_api_object_t obj)
{
    phy_media_tbl_t                 *mtbl = NULL;
    sdi_media_transceiver_descr_t   transceiver;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (pas_sdi_media_transceiver_code_get(mtbl->res_hdl, &transceiver)
            != STD_ERR_OK) {
        return false;
    }

    if (memcmp(&transceiver, mtbl->res_data->transceiver, sizeof(transceiver))) {
        memcpy(mtbl->res_data->transceiver, &transceiver, sizeof(transceiver));

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_TRANSCEIVER,
                        &transceiver, sizeof(transceiver)) == false) {
                return false;
            }
        }
    }

    return true;
}


/*
 * dn_pas_media_type_poll is to poll and get the mediatype.
 */

static bool dn_pas_media_type_poll (uint_t port, cps_api_object_t obj)
{
    phy_media_tbl_t          *mtbl = NULL;
    PLATFORM_MEDIA_TYPE_t    type;
    bool                     supported = true, disable = false;
    bool                     high_power_mode = true;
    bool                     ret = true;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (dn_pas_media_product_info_poll(port, obj) == false) {

        mtbl->res_data->type = PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;

        return false;
    }

    if (dn_pas_media_transceiver_code_poll(port, obj) == false) {

        return false;
    }

    if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS)) {
        dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_DEVICE_TECH,
                &mtbl->res_data->device_tech, NULL, 0, &ret);
        if (ret == false) {
            return ret;
        }
    }

    if (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS) {
        dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_LENGTH_SMF,
                &mtbl->res_data->length_sfm, obj, BASE_PAS_MEDIA_LENGTH_SFM, &ret);
    }

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_OPTIONS,
            &mtbl->res_data->options, obj, BASE_PAS_MEDIA_OPTIONS, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_DIAG_MON_TYPE,
            &mtbl->res_data->diag_mon_type, obj,
            BASE_PAS_MEDIA_DIAG_MON_TYPE, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_LENGTH_SMF_KM,
            &mtbl->res_data->length_sfm_km, obj,
            BASE_PAS_MEDIA_LENGTH_SFM_KM, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_LENGTH_OM1,
            &mtbl->res_data->length_om1, obj, BASE_PAS_MEDIA_LENGTH_OM1, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_LENGTH_OM2,
            &mtbl->res_data->length_om2, obj, BASE_PAS_MEDIA_LENGTH_OM2, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_LENGTH_OM3,
            &mtbl->res_data->length_om3, obj, BASE_PAS_MEDIA_LENGTH_OM3, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_LENGTH_CABLE_ASSEMBLY,
            &mtbl->res_data->length_cable, obj,
            BASE_PAS_MEDIA_LENGTH_CABLE, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_CONNECTOR,
            &mtbl->res_data->connector, obj, BASE_PAS_MEDIA_CONNECTOR, &ret);

    type = dn_pas_media_type_get(mtbl->res_data);

    if ((type == PLATFORM_MEDIA_TYPE_SFPPLUS_10GBASE_ZR_TUNABLE)
            && (mtbl->res_data->target_wavelength != 0)) {
        dn_pas_media_wavelength_set(port);
    }

    /*
     * LR media is supported in top row only. Need to disable LR media its
     * plugged in bottom row.
     */
    if (dn_pas_is_media_type_supported_in_fp(port, type,
                &disable, &high_power_mode, &supported) == false) {

        if (supported == false) {
            mtbl->res_data->support_status = PLATFORM_MEDIA_SUPPORT_STATUS_NOT_SUPPORTED_DISABLED;
            dn_pas_media_transceiver_state_set(port, true);
            PAS_NOTICE("Optics type (%u) is not supported in this paltform(port %u), Disabling media transceiver.", type, port);
        } else if (disable == true) {
            mtbl->res_data->support_status = PLATFORM_MEDIA_SUPPORT_STATUS_NOT_SUPPORTED_DISABLED;
            dn_pas_media_transceiver_state_set(port, true);
            PAS_NOTICE("High power optics (type:%u) is not supported in this port(%u), Disabling media transceiver.",
                   type, port);
        } else {
            mtbl->res_data->support_status = PLATFORM_MEDIA_SUPPORT_STATUS_NOT_SUPPORTED;
            PAS_NOTICE("High power optics (type:%u) is not supported in this port(%u), Media transceiver not disabled.",
                   type, port);
        }
    }

    dn_pas_media_high_power_mode_set(port,
                (high_power_mode == false) ? false : true);

    if (mtbl->res_data->type != type) {
        mtbl->res_data->type = type;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_TYPE,
                        &type, sizeof(type)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }


    return true;
}

/*
 * dn_pas_media_capability_poll is to poll and get the capability of
 * media present in the port.
 */

static bool dn_pas_media_capability_poll (uint_t port, cps_api_object_t obj)
{
    phy_media_tbl_t          *mtbl = NULL;
    BASE_IF_SPEED_t          capability;
    sdi_media_speed_t        speed = 0;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (mtbl->res_data->present == false) {
    if (pas_sdi_media_speed_get(mtbl->res_hdl, &speed) != STD_ERR_OK) {
        PAS_ERR("Failed to get media speed, port %u", port);

        return false;
    }

    capability = dn_pas_capability_conv(speed);
    } else {
        capability = dn_pas_media_capability_get(mtbl);

    struct pas_config_media *cfg = dn_pas_config_media_get();

        if (cfg->lockdown && (mtbl->res_data->qualified == false)) {
            if (dn_pas_is_media_unsupported(mtbl, true)) {  /* log msg if unsupported */
            dn_pas_media_transceiver_state_set(port, cfg->lockdown);
            }
        }
    }

    if(mtbl->res_data->capability != capability) {

        mtbl->res_data->capability = capability;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CAPABILITY,
                        &capability, sizeof(capability)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }

    return true;
}

/*
 * dn_pas_media_wavelength_poll is to get the wave length of the media.
 */

static bool dn_pas_media_wavelength_poll (uint_t port, cps_api_object_t obj)
{
    phy_media_tbl_t        *mtbl = NULL;
    uint_t                 wavelength = 0;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (pas_sdi_media_parameter_get(mtbl->res_hdl, SDI_MEDIA_WAVELENGTH,
                &wavelength) != STD_ERR_OK) {
        PAS_ERR("Failed to get media wavelength, port %u", port);

        return false;
    }

    if(mtbl->res_data->wavelength != wavelength) {

        mtbl->res_data->wavelength = wavelength;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_WAVELENGTH,
                        &wavelength, sizeof(wavelength)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }

    return true;
}

/* Look up a function in the vendor media plug-in */

static void *pas_media_vendor_func(const char *name)
{
    static const char filename[] = "/usr/lib/libopx_pas_media_vendor.so";
    static void       *dlhdl;

    if (dlhdl == 0 && access(filename, R_OK) == 0) {
        dlhdl = dlopen(filename, RTLD_NOW);
        if (dlhdl == 0)  PAS_ERR("Failed to load media vendor plug-in, %s", dlerror());
    }
    return (dlhdl == 0 ? 0 : dlsym(dlhdl, name));
}
#define PAS_MEDIA_VENDOR_FUNC(nm)  ((typeof(& nm)) pas_media_vendor_func(# nm))

/*
 * dn_pas_media_dq_poll is to poll and get the qualified attribute
 * of the media present in the specified port.
 */

static bool dn_pas_media_dq_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t        *mtbl = NULL;
    bool                   qualified = true; /* Assume is qualified */

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    /* Call vendor media plug-in */
    typeof(&dn_pas_media_vendor_is_qualified) func = PAS_MEDIA_VENDOR_FUNC(dn_pas_media_vendor_is_qualified);
    if (func != 0 && (*func)(mtbl->res_hdl, &qualified) != STD_ERR_OK) {
        PAS_ERR("Failed to get qualified attribute, port %u", port);

        return false;
    }

    if(mtbl->res_data->qualified != qualified) {

        mtbl->res_data->qualified = qualified;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj,
                        BASE_PAS_MEDIA_QUALIFIED, &qualified,
                        sizeof(qualified)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }

    return true;
}
/*
 * dn_pas_media_high_power_mode_poll function is to poll and update the
 * high_power_mode attribute.
 */

bool dn_pas_media_high_power_mode_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t        *mtbl = NULL;
    bool                   lr_mode = false;
    bool                   mode = false;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP)
            || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP_PLUS)) {
        return true;
    }

    if (pas_sdi_media_module_control_status_get(mtbl->res_hdl,
                SDI_MEDIA_LP_MODE, &lr_mode) != STD_ERR_OK) {
        PAS_ERR("Failed to get media control status, port %u", port);

        return false;
    }

    mode = !lr_mode;

    if(mtbl->res_data->high_power_mode != mode) {

        mtbl->res_data->high_power_mode = mode;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj,
                        BASE_PAS_MEDIA_HIGH_POWER_MODE, &mode,
                        sizeof(mode)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }
    return true;
}


/*
 * dn_pas_media_vendor_name_poll is to poll and get the vendor name
 * of the inserted media in specified port.
 */

static bool dn_pas_media_vendor_name_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t       *mtbl = NULL;
    char                  vendor_name[SDI_MEDIA_MAX_VENDOR_NAME_LEN] = "";

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (pas_sdi_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_NAME,
                vendor_name, SDI_MEDIA_MAX_VENDOR_NAME_LEN)
            != STD_ERR_OK) {
        PAS_ERR("Failed to get media vendor name, port %u", port);

        return false;
    }

    if(memcmp(vendor_name, mtbl->res_data->vendor_name, sizeof(vendor_name))) {

        memcpy(mtbl->res_data->vendor_name, vendor_name, sizeof(vendor_name));

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_VENDOR_NAME,
                        vendor_name, sizeof(vendor_name)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }

    return true;
}

/*
 * dn_pas_media_vendor_oui_poll is to get the vendor OUI of the
 * media inserted in the specified port.
 */

static bool dn_pas_media_vendor_oui_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t       *mtbl = NULL;
    char                  vendor_oui[SDI_MEDIA_MAX_VENDOR_OUI_LEN] = "";


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (pas_sdi_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_OUI,
                vendor_oui, SDI_MEDIA_MAX_VENDOR_OUI_LEN)
            != STD_ERR_OK) {
        PAS_ERR("Failed to get media vendor OUI, port %u", port);

        return false;
    }

    if(memcmp(vendor_oui, mtbl->res_data->vendor_id, sizeof(vendor_oui)) != 0){

        memcpy(mtbl->res_data->vendor_id, vendor_oui, sizeof(vendor_oui));

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_VENDOR_ID,
                        vendor_oui, sizeof(vendor_oui)) == false) {
                PAS_ERR("Failed to add object attribute, port %u", port);

                return false;
            }
        }
    }
    return true;
}

/*
 * dn_pas_media_vendor_info_get is to get the vendor info of the media
 * based on sdi handle.
 */

static void dn_pas_media_vendor_info_get (sdi_resource_hdl_t hdl,
        sdi_media_vendor_info_type_t type, void *memp, size_t size,
        cps_api_object_t obj, BASE_PAS_MEDIA_t attr_id, bool *ret)
{

    if ((hdl == NULL) || (memp == NULL)) {
        PAS_ERR("Invalid parameter");

        if (ret != NULL) *ret = false;
    }

    if (pas_sdi_media_vendor_info_get(hdl, type, memp, size) != STD_ERR_OK) {
        PAS_ERR("Failed to get media vendor info");

        if (ret != NULL) *ret = false;
    }

    if (obj != NULL) {
        if (cps_api_object_attr_add(obj, attr_id, memp, size) == false) {
            PAS_ERR("Failed to add object attribute");

            if (ret != NULL) *ret = false;
        }
    }

}

/*
 * dn_pas_media_vendor_info_poll is to poll all the vendor information
 * of the media inserted in the specified port.
 */

static bool dn_pas_media_vendor_info_poll (uint_t port, cps_api_object_t obj)
{
    phy_media_tbl_t       *mtbl = NULL;
    bool                  ret = true;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    dn_pas_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_NAME,
            &mtbl->res_data->vendor_name, SDI_MEDIA_MAX_VENDOR_NAME_LEN,
            obj, BASE_PAS_MEDIA_VENDOR_NAME, &ret);

    dn_pas_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_DATE,
            &mtbl->res_data->date_code, SDI_MEDIA_MAX_VENDOR_DATE_LEN,
            obj, BASE_PAS_MEDIA_DATE_CODE, &ret);

    dn_pas_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_PN,
            &mtbl->res_data->vendor_pn, SDI_MEDIA_MAX_VENDOR_PART_NUMBER_LEN,
            obj, BASE_PAS_MEDIA_VENDOR_PN, &ret);

    dn_pas_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_REVISION,
            &mtbl->res_data->vendor_rev, SDI_MEDIA_MAX_VENDOR_REVISION_LEN,
            obj, BASE_PAS_MEDIA_VENDOR_REV, &ret);

    dn_pas_media_vendor_info_get(mtbl->res_hdl, SDI_MEDIA_VENDOR_SN,
            &mtbl->res_data->serial_number,
            SDI_MEDIA_MAX_VENDOR_SERIAL_NUMBER_LEN,
            obj, BASE_PAS_MEDIA_SERIAL_NUMBER, &ret);

    return ret;

}

/*
 * dn_pas_media_module_monitor_poll is to poll and get the real time
 * monitoring data of the media.
 */

static bool dn_pas_media_module_monitor_poll (uint_t port,
        sdi_media_module_monitor_t type)
{
    phy_media_tbl_t       *mtbl = NULL;
    float                 value = 0;
    bool                  ret = false;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
                || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP))
            && (mtbl->res_data->supported_feature.sfp_features.diag_mntr_support_status
                == false)) return true;

    if (pas_sdi_media_module_monitor_get(mtbl->res_hdl, type, &value)
            != STD_ERR_OK) {
        PAS_ERR("Failed to get media module monitor, port %u", port);

        return false;
    }

    if (type == SDI_MEDIA_TEMP) {

        mtbl->res_data->current_temperature = value;
        ret = true;

    } else if (type == SDI_MEDIA_VOLT) {

        mtbl->res_data->current_voltage = value;
        ret = true;
    }

    return ret;
}

/*
 * dn_pas_media_channel_monitor_poll is to poll and get the media
 * channel monitoring data.
 */

static bool dn_pas_media_channel_monitor_poll (uint_t port, uint_t channel,
        sdi_media_channel_monitor_t type)
{
    phy_media_tbl_t       *mtbl = NULL;
    float                 value = 0;
    bool                  ret = false;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
                || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP))
            && (mtbl->res_data->supported_feature.sfp_features.diag_mntr_support_status
                == false)) return true;

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        PAS_ERR("Failed to validate media channel, port %u channel %u",
                port, channel
                );

        return false;
    }

    if (pas_sdi_media_channel_monitor_get(mtbl->res_hdl, channel,
                type, &value) != STD_ERR_OK) {
        PAS_ERR("Failed to get media channel monitor, port %u channel %u type %u",
                port, channel, type
                );

        return false;
    }

    if (type == SDI_MEDIA_INTERNAL_RX_POWER_MONITOR) {

        mtbl->channel_data[channel].rx_power = value;
        ret = true;

    } else if (type == SDI_MEDIA_INTERNAL_TX_POWER_BIAS) {

        mtbl->channel_data[channel].tx_bias_current = value;
        ret = true;

    } else if (type == SDI_MEDIA_INTERNAL_TX_OUTPUT_POWER) {

        mtbl->channel_data[channel].tx_power = value;
        ret = true;
    }

    return ret;
}

/*
 * dn_pas_media_channel_tx_control_status_poll function is to poll
 * media channel tx control status.
 */

static bool dn_pas_media_channel_tx_control_status_poll (uint_t port,
        uint_t channel, cps_api_object_t obj)
{
    phy_media_tbl_t       *mtbl = NULL;
    bool                  status = false;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        PAS_ERR("Failed to validate media channel, port %u channel %u",
                port, channel
                );

        return false;
    }

    if (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS))
            && (mtbl->res_data->supported_feature.qsfp_features.tx_control_support_status
                == false)) return true;

    if (pas_sdi_media_tx_control_status_get(mtbl->res_hdl, channel,
                &status) != STD_ERR_OK) {
        PAS_ERR("Failed to get media tx control status, port %u channel %u",
                port, channel
                );

        return false;
    }

    if (mtbl->channel_data[channel].state != status) {

        mtbl->channel_data[channel].state = status;

        if (obj != NULL) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_STATE,
                        &status, sizeof(status)) == false) {
                return false;
            }
        }
    }

    return true;
}

/*
 * dn_pas_media_channel_monitor_status_check function is to check media
 * channel monitor status and set alarm state.
 */

static bool dn_pas_media_channel_monitor_status_check(uint_t port,
        uint_t channel, cps_api_object_t obj, uint_t status)
{
    phy_media_tbl_t          *mtbl = NULL;
    pas_media_channel_t      *ch_data = NULL;
    PLATFORM_MEDIA_STATUS_t  check = PLATFORM_MEDIA_STATUS_NORMAL_STATUS;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        return false;
    }


    ch_data = &(mtbl->channel_data[channel]);

    if(status & SDI_MEDIA_RX_PWR_HIGH_ALARM) {

        check = PLATFORM_MEDIA_STATUS_HIGH_ALARM;
    } else if (status & SDI_MEDIA_RX_PWR_LOW_ALARM) {

        check = PLATFORM_MEDIA_STATUS_LOW_ALARM;
    } else if (status & SDI_MEDIA_RX_PWR_HIGH_WARNING) {

        check = PLATFORM_MEDIA_STATUS_HIGH_WARNING;
    } else if (status & SDI_MEDIA_RX_PWR_LOW_WARNING) {

        check = PLATFORM_MEDIA_STATUS_LOW_WARNING;
    }

    if (check != ch_data->rx_power_state) {
        ch_data->rx_power_state = check;

        if (obj != NULL) {
            cps_api_object_attr_add_u32(obj,
                    BASE_PAS_MEDIA_CHANNEL_RX_POWER_STATE,
                    check);

            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_RX_POWER,
                    &ch_data->rx_power, sizeof(ch_data->rx_power));
        }
    }


    check = PLATFORM_MEDIA_STATUS_NORMAL_STATUS;

    if(status & SDI_MEDIA_TX_PWR_HIGH_ALARM) {

        check = PLATFORM_MEDIA_STATUS_HIGH_ALARM;
    } else if (status & SDI_MEDIA_TX_PWR_LOW_ALARM) {

        check = PLATFORM_MEDIA_STATUS_LOW_ALARM;
    } else if (status & SDI_MEDIA_TX_PWR_HIGH_WARNING) {

        check = PLATFORM_MEDIA_STATUS_HIGH_WARNING;
    } else if (status & SDI_MEDIA_TX_PWR_LOW_WARNING) {

        check = PLATFORM_MEDIA_STATUS_LOW_WARNING;
    }

    if (check != ch_data->tx_power_state) {
        ch_data->tx_power_state = check;

        if (obj != NULL) {
            cps_api_object_attr_add_u32(obj,
                    BASE_PAS_MEDIA_CHANNEL_TX_POWER_STATE,
                    check);

            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_TX_POWER,
                    &ch_data->tx_power, sizeof(ch_data->tx_power));
        }
    }

    check = PLATFORM_MEDIA_STATUS_NORMAL_STATUS;

    if(status & SDI_MEDIA_TX_BIAS_HIGH_ALARM ) {

        check = PLATFORM_MEDIA_STATUS_HIGH_ALARM;
    } else if (status & SDI_MEDIA_TX_BIAS_LOW_ALARM) {

        check = PLATFORM_MEDIA_STATUS_LOW_ALARM;
    } else if (status & SDI_MEDIA_TX_BIAS_HIGH_WARNING) {

        check = PLATFORM_MEDIA_STATUS_HIGH_WARNING;
    } else if (status & SDI_MEDIA_TX_BIAS_LOW_WARNING) {

        check = PLATFORM_MEDIA_STATUS_LOW_WARNING;
    }

    if (check != ch_data->tx_bias_state) {
        ch_data->tx_bias_state = check;

        if (obj != NULL) {

            cps_api_object_attr_add_u32(obj,
                    BASE_PAS_MEDIA_CHANNEL_TX_BIAS_STATE,
                    check);
            cps_api_object_attr_add(obj,
                    BASE_PAS_MEDIA_CHANNEL_TX_BIAS_CURRENT,
                    &ch_data->tx_bias_current,
                    sizeof(ch_data->tx_bias_current));
        }

    }

    return true;
}

/*
 * dn_pas_media_channel_status_check function to check and set alarm status.
 */

static bool dn_pas_media_channel_status_check (uint_t port, uint_t channel,
        cps_api_object_t obj, uint_t status)
{
    phy_media_tbl_t       *mtbl = NULL;
    bool                  ret = true;
    pas_media_channel_t   *ch_data = NULL;
    bool                  check = false;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        return false;
    }

    ch_data = &(mtbl->channel_data[channel]);

    check = status & SDI_MEDIA_STATUS_RXLOSS;

    if (ch_data->rx_loss != check) {

        ch_data->rx_loss = check;

        if (obj != NULL) {
            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_RX_LOSS,
                    &check, sizeof(check));
        }
    }

    check = status & SDI_MEDIA_STATUS_TXLOSS;

    if (ch_data->tx_loss != check) {

        ch_data->tx_loss = check;

        if (obj != NULL) {
            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_TX_LOSS,
                    &check, sizeof(check));
        }
    }

    check = status & SDI_MEDIA_STATUS_TXFAULT;

    if (ch_data->tx_fault != check) {

        ch_data->tx_fault = check;

        if (obj != NULL) {
            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_TX_FAULT,
                    &check, sizeof(check));
        }
    }

    check = status & SDI_MEDIA_STATUS_TXDISABLE;

    if (ch_data->tx_disable != check) {

        ch_data->tx_disable = check;

        if (obj != NULL) {
            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CHANNEL_TX_DISABLE,
                    &check, sizeof(check));
        }
    }

    return ret;
}



/*
 * Media channel real time data poll.
 */

static bool dn_pas_media_channel_rtd_poll(uint_t port, uint_t channel,
        cps_api_object_t obj)
{
    phy_media_tbl_t       *mtbl = NULL;
    bool                  ret = true;
    uint_t                mstatus, status;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        PAS_ERR("Invalid port/channel, port %u channel %u",
                port, channel
                );

        return false;
    }

    if (dn_pas_media_channel_tx_control_status_poll(port, channel, obj)
            == false) {
        PAS_ERR("Failed to poll media channel tx control status, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_cdr_get(port, channel,
                &(mtbl->channel_data[channel].cdr_enable))
            == false) {
        ret = false;
    }

    if (dn_pas_media_channel_monitor_poll(port, channel,
                SDI_MEDIA_INTERNAL_RX_POWER_MONITOR) == false) {
        PAS_ERR("Failed to poll media channel rx power, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_monitor_poll(port, channel,
                SDI_MEDIA_INTERNAL_TX_OUTPUT_POWER) == false) {
        PAS_ERR("Failed to poll media channel tx power, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_monitor_poll(port, channel,
                SDI_MEDIA_INTERNAL_TX_POWER_BIAS) == false) {
        PAS_ERR("Failed to poll media channel tx power bias, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_monitor_status_poll(port, channel, &mstatus)
            == false) {
        PAS_ERR("Failed to poll media channel monitor status, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_monitor_status_check(port, channel, obj,
                mstatus) == false) {
        PAS_ERR("Failed to check media channel monitor status, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_status_poll(port, channel, &status)
            == false) {
        PAS_ERR("Failed to poll media channel status, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    if (dn_pas_media_channel_status_check(port, channel, obj,
                status) == false) {
        PAS_ERR("Failed to check media channel status, port %u channel %u",
                port, channel
                );

        ret = false;
    }

    return ret;
}

/*
 * dn_pas_media_monitor_status_check function to check and set the
 * monitor status alarm.
 */

static bool dn_pas_media_monitor_status_check(uint_t port,
        cps_api_object_t obj, uint_t status)
{
    phy_media_tbl_t          *mtbl = NULL;
    pas_media_t              *res_data = NULL;
    PLATFORM_MEDIA_STATUS_t  check = PLATFORM_MEDIA_STATUS_NORMAL_STATUS;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    res_data = mtbl->res_data;

    if(status & SDI_MEDIA_STATUS_TEMP_HIGH_ALARM) {

        check = PLATFORM_MEDIA_STATUS_HIGH_ALARM;
    } else if (status & SDI_MEDIA_STATUS_TEMP_LOW_ALARM) {

        check = PLATFORM_MEDIA_STATUS_LOW_ALARM;
    } else if (status & SDI_MEDIA_STATUS_TEMP_HIGH_WARNING) {

        check = PLATFORM_MEDIA_STATUS_HIGH_WARNING;
    } else if (status & SDI_MEDIA_STATUS_TEMP_LOW_WARNING) {

        check = PLATFORM_MEDIA_STATUS_LOW_WARNING;
    }

    if (check != res_data->temp_state) {

        res_data->temp_state = check;

        if (obj != NULL) {

            cps_api_object_attr_add_u32(obj, BASE_PAS_MEDIA_TEMP_STATE,
                    check);

            cps_api_object_attr_add(obj, BASE_PAS_MEDIA_CURRENT_TEMPERATURE,
                    &res_data->current_temperature,
                    sizeof(res_data->current_temperature));
        }
    }


    check = PLATFORM_MEDIA_STATUS_NORMAL_STATUS;

    if(status & SDI_MEDIA_STATUS_VOLT_HIGH_ALARM) {

        check = PLATFORM_MEDIA_STATUS_HIGH_ALARM;
    } else if (status & SDI_MEDIA_STATUS_VOLT_LOW_ALARM) {

        check = PLATFORM_MEDIA_STATUS_LOW_ALARM;
    } else if (status & SDI_MEDIA_STATUS_VOLT_HIGH_WARNING) {

        check = PLATFORM_MEDIA_STATUS_HIGH_WARNING;
    } else if (status & SDI_MEDIA_STATUS_VOLT_LOW_WARNING) {

        check = PLATFORM_MEDIA_STATUS_LOW_WARNING;
    }

    if (check != res_data->voltage_state) {
        res_data->voltage_state = check;

        if (obj != NULL) {

            cps_api_object_attr_add_u32(obj, BASE_PAS_MEDIA_VOLTAGE_STATE,
                    check);
            cps_api_object_attr_add(obj,
                    BASE_PAS_MEDIA_CURRENT_VOLTAGE,
                    &res_data->current_voltage,
                    sizeof(res_data->current_voltage));
        }

    }

    return true;
}


/*
 * dn_pas_media_rtd_poll is to poll and get the real time data of both media
 * media channel monitoring data.
 */

static bool dn_pas_media_rtd_poll(uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t       *mtbl = NULL;
    bool                  ret = true;
    uint_t                status;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (dn_pas_media_module_monitor_poll(port, SDI_MEDIA_TEMP) == false) {
        PAS_ERR("Failed to poll media module temperature, port %u",
                port
                );

        ret = false;
    }

    if (dn_pas_media_module_monitor_poll(port, SDI_MEDIA_VOLT) == false) {
        PAS_ERR("Failed to poll media module voltage, port %u",
                port
                );

        ret = false;
    }

    if (dn_pas_media_module_monitor_status_poll(port, &status) == false) {
        PAS_ERR("Failed to poll media module status, port %u",
                port
                );

        ret = false;
    }

    if (dn_pas_media_monitor_status_check(port, obj, status) == false) {
        PAS_ERR("Failed to check media monitor status, port %u",
                port
                );

        ret = false;
    }

    return ret;
}

/*
 * dn_pas_media_int_attr_poll is to poll and get the integer type
 * attribute of the media.
 */

static void dn_pas_media_int_attr_poll (sdi_resource_hdl_t hdl,
        sdi_media_param_type_t type, uint_t *memp, cps_api_object_t obj,
        BASE_PAS_MEDIA_t attr_id, bool *ret)
{
    uint_t          value = 0;

    if ((hdl == NULL) || (memp == NULL)){
        PAS_ERR("Invalid parameter");

        if (ret != NULL) *ret = false;

        return;
    }

    if(pas_sdi_media_parameter_get(hdl, type, &value) != STD_ERR_OK) {
        PAS_ERR("Failed to get media parameter");

        if (ret != NULL) *ret = false;

        return;
    }

    if (*memp != value) {

        *memp = value;

        if (obj != NULL) {

            if (cps_api_object_attr_add_u32(obj, attr_id, value) == false) {
                PAS_ERR("Failed to add object attribute");

                if (ret != NULL) *ret = false;
            }
        }
    }
}

/*
 * dn_pas_media_data_poll is to poll all integer attributes of media
 * and update the local store.
 */

static bool dn_pas_media_data_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t *mtbl;
    bool ret = true;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_WAVELENGTH,
            &mtbl->res_data->wavelength, obj, BASE_PAS_MEDIA_WAVELENGTH, &ret);

    if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS)) {

        dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_WAVELENGTH_TOLERANCE,
                &mtbl->res_data->wavelength_tolerance, obj,
                BASE_PAS_MEDIA_MEDIA_CATEGORY_QSFP_PLUS_WAVELENGTH_TOLERANCE,
                &ret);

        dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_MAX_CASE_TEMP,
                &mtbl->res_data->max_case_temp, obj,
                BASE_PAS_MEDIA_MEDIA_CATEGORY_QSFP_PLUS_MAX_CASE_TEMP, &ret);
    }

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_CC_BASE,
            &mtbl->res_data->cc_base, obj, BASE_PAS_MEDIA_CC_BASE, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_CC_EXT,
            &mtbl->res_data->cc_ext, obj, BASE_PAS_MEDIA_CC_EXT, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_ENCODING_TYPE,
            &mtbl->res_data->encoding, obj, BASE_PAS_MEDIA_ENCODING, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_NM_BITRATE,
            &mtbl->res_data->br_nominal, obj, BASE_PAS_MEDIA_BR_NOMINAL, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_IDENTIFIER,
            &mtbl->res_data->identifier, obj, BASE_PAS_MEDIA_IDENTIFIER, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_EXT_IDENTIFIER,
            &mtbl->res_data->ext_identifier, obj,
            BASE_PAS_MEDIA_EXT_IDENTIFIER, &ret);

    if (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS) {

        dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_MAX_BITRATE,
                &mtbl->res_data->br_max, obj,
                BASE_PAS_MEDIA_MEDIA_CATEGORY_SFP_PLUS_BR_MAX, &ret);

        dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_MIN_BITRATE,
                &mtbl->res_data->br_min, obj,
                BASE_PAS_MEDIA_MEDIA_CATEGORY_SFP_PLUS_BR_MIN, &ret);
    }

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_ENHANCED_OPTIONS,
            &mtbl->res_data->enhanced_options, obj,
            BASE_PAS_MEDIA_ENHANCED_OPTIONS, &ret);

    dn_pas_media_int_attr_poll(mtbl->res_hdl, SDI_MEDIA_DIAG_MON_TYPE,
            &mtbl->res_data->rx_power_measurement_type, obj,
            BASE_PAS_MEDIA_DIAG_MON_TYPE, &ret);

    return ret;
}

/*
 * dn_pas_media_threshold_attr_poll is to poll threshold attributes of media.
 */

static void dn_pas_media_threshold_attr_poll (sdi_resource_hdl_t hdl,
        sdi_media_threshold_type_t type, double *memp, cps_api_object_t obj,
        BASE_PAS_MEDIA_t attr_id, bool *ret)
{
    float value = 0;

    if ((hdl == NULL) || (memp == NULL)) {
        PAS_ERR("Invalid parameter");

        return;
    }

    if(pas_sdi_media_threshold_get(hdl, type, &value) != STD_ERR_OK) {
        PAS_ERR("Failed to get media threshold");

        if (ret != NULL) *ret = false;
        value = 0;
    }

    if (*memp != value) {

        *memp = value;

        if (obj != NULL) {
            if (cps_api_object_attr_add_u32(obj, attr_id, value) == false) {
                PAS_ERR("Failed to add object attribute");

                if (ret != NULL) *ret = false;
            }
        }
    }
}

/*
 * dn_pas_media_threshold_poll is to poll all threshold attributes.
 */

static bool dn_pas_media_threshold_poll (uint_t port, cps_api_object_t obj)
{
    phy_media_tbl_t *mtbl;
    bool ret = true;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if ((((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)
                || (mtbl->res_data->category ==
                    PLATFORM_MEDIA_CATEGORY_QSFP_PLUS))
          && (mtbl->res_data->supported_feature.qsfp_features.paging_support_status
            == false))
        || (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
                || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP))
          && (mtbl->res_data->supported_feature.sfp_features.diag_mntr_support_status
            == false))) return true;

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TEMP_HIGH_ALARM_THRESHOLD, &mtbl->res_data->temp_high_alarm,
            obj, BASE_PAS_MEDIA_TEMP_HIGH_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TEMP_LOW_ALARM_THRESHOLD, &mtbl->res_data->temp_low_alarm,
            obj, BASE_PAS_MEDIA_TEMP_LOW_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TEMP_HIGH_WARNING_THRESHOLD, &mtbl->res_data->temp_high_warning,
            obj, BASE_PAS_MEDIA_TEMP_HIGH_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TEMP_LOW_WARNING_THRESHOLD, &mtbl->res_data->temp_low_warning,
            obj, BASE_PAS_MEDIA_TEMP_LOW_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_VOLT_HIGH_ALARM_THRESHOLD, &mtbl->res_data->voltage_high_alarm,
            obj, BASE_PAS_MEDIA_VOLTAGE_HIGH_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_VOLT_LOW_ALARM_THRESHOLD, &mtbl->res_data->voltage_low_alarm,
            obj, BASE_PAS_MEDIA_VOLTAGE_LOW_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_VOLT_HIGH_WARNING_THRESHOLD, &mtbl->res_data->voltage_high_warning,
            obj, BASE_PAS_MEDIA_VOLTAGE_HIGH_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_VOLT_LOW_WARNING_THRESHOLD, &mtbl->res_data->voltage_low_warning,
            obj, BASE_PAS_MEDIA_VOLTAGE_LOW_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_RX_PWR_HIGH_ALARM_THRESHOLD, &mtbl->res_data->rx_power_high_alarm,
            obj, BASE_PAS_MEDIA_RX_POWER_HIGH_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_RX_PWR_LOW_ALARM_THRESHOLD, &mtbl->res_data->rx_power_low_alarm,
            obj, BASE_PAS_MEDIA_RX_POWER_LOW_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_RX_PWR_HIGH_WARNING_THRESHOLD, &mtbl->res_data->rx_power_high_warning,
            obj, BASE_PAS_MEDIA_RX_POWER_HIGH_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_RX_PWR_LOW_WARNING_THRESHOLD, &mtbl->res_data->rx_power_low_warning,
            obj, BASE_PAS_MEDIA_RX_POWER_LOW_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_BIAS_HIGH_ALARM_THRESHOLD, &mtbl->res_data->bias_high_alarm,
            obj, BASE_PAS_MEDIA_BIAS_HIGH_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_BIAS_LOW_ALARM_THRESHOLD, &mtbl->res_data->bias_low_alarm,
            obj, BASE_PAS_MEDIA_BIAS_LOW_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_BIAS_HIGH_WARNING_THRESHOLD, &mtbl->res_data->bias_high_warning,
            obj, BASE_PAS_MEDIA_BIAS_HIGH_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_BIAS_LOW_WARNING_THRESHOLD, &mtbl->res_data->bias_low_warning,
            obj, BASE_PAS_MEDIA_BIAS_LOW_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_PWR_HIGH_ALARM_THRESHOLD, &mtbl->res_data->tx_power_high_alarm,
            obj, BASE_PAS_MEDIA_TX_POWER_HIGH_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_PWR_LOW_ALARM_THRESHOLD, &mtbl->res_data->tx_power_low_alarm,
            obj, BASE_PAS_MEDIA_TX_POWER_LOW_ALARM_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_PWR_HIGH_WARNING_THRESHOLD, &mtbl->res_data->tx_power_high_warning,
            obj, BASE_PAS_MEDIA_TX_POWER_HIGH_WARNING_THRESHOLD, &ret);

    dn_pas_media_threshold_attr_poll(mtbl->res_hdl,
            SDI_MEDIA_TX_PWR_LOW_WARNING_THRESHOLD, &mtbl->res_data->tx_power_low_warning,
            obj, BASE_PAS_MEDIA_TX_POWER_LOW_WARNING_THRESHOLD, &ret);

    return ret;

}

/*
 * dn_pas_media_channel_status_poll is to poll the channel status.
 */

static bool dn_pas_media_channel_status_poll (uint_t port,
        uint_t channel, uint_t *status)
{
    phy_media_tbl_t       *mtbl = NULL;
    uint_t                flags;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (status == NULL) {
        PAS_ERR("Invalid parameter");

        return false;
    }

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        PAS_ERR("Failed to validate media port/channel, port %u channel %u",
                port, channel
                );

        return false;
    }

    *status = 0;

    flags = PAS_MEDIA_CH_STATUS_FLAGS;

    if (pas_sdi_media_channel_status_get(mtbl->res_hdl, channel,
                flags, status) != STD_ERR_OK) {
        PAS_ERR("Failed to get media channel status, port %u channel %u",
                port, channel
                );

        return false;
    }

    return true;
}

/*
 * dn_pas_media_channel_monitor_status_poll is to poll
 * media channel monitor status.
 */

static bool dn_pas_media_channel_monitor_status_poll (uint_t port,
        uint_t channel, uint_t *status)
{
    phy_media_tbl_t       *mtbl = NULL;
    uint_t                flags;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
                || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP))
            && (mtbl->res_data->supported_feature.sfp_features.alarm_support_status
                == false)) return true;

    if (status == NULL) {
        PAS_ERR("Invalid parameter");

        return false;
    }

    if (dn_phy_is_media_channel_valid(port, channel) == false) {
        PAS_ERR("Failed to validate port/channel, port %u channel %u",
                port, channel
                );

        return false;
    }

    *status = 0;
    flags = PAS_MEDIA_MON_ALL_FLAGS;

    if (pas_sdi_media_channel_monitor_status_get(mtbl->res_hdl, channel,
                flags, status) != STD_ERR_OK) {
        PAS_ERR("Failed to get media channel status, port %u channel %u",
                port, channel
                );

        return false;
    }

    return true;
}



/*
 * dn_pas_media_module_monitor_status_poll is to poll media module monitor
 * status of media which is present in the specified port.
 *
 */

static bool dn_pas_media_module_monitor_status_poll (uint_t port,
        uint_t *status)
{

    phy_media_tbl_t     *mtbl = NULL;
    uint_t              flags;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    if (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
                || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP))
            && (mtbl->res_data->supported_feature.sfp_features.alarm_support_status
                == false)) return true;

    if (status == NULL) {
        PAS_ERR("Invalid parameter");

        return false;
    }

    *status = 0;
    flags = PAS_MEDIA_MON_ALL_FLAGS;

    if (pas_sdi_media_module_monitor_status_get(mtbl->res_hdl, flags, status)
            != STD_ERR_OK) {
        PAS_ERR("Failed to get media module monitor status, port %u",
                port
                );

        return false;
    }

    return true;
}

/*
 * dn_pas_media_product_info_poll is to poll and get the product info
 * of the media inserted in specified port.
 */

static bool dn_pas_media_product_info_poll (uint_t port, cps_api_object_t obj)
{

    phy_media_tbl_t                *mtbl = NULL;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    typeof(mtbl->res_data->vendor_specific) buf;

    typeof(&dn_pas_media_vendor_product_info_get) func = PAS_MEDIA_VENDOR_FUNC(dn_pas_media_vendor_product_info_get);
    if (func != 0 && (*func)(mtbl->res_hdl, buf) != STD_ERR_OK) {
        PAS_ERR("Failed to get media vendor product info, port %u",
                port
                );

        return false;
    }

    if (memcmp(mtbl->res_data->vendor_specific, buf,
                sizeof(mtbl->res_data->vendor_specific)) != 0) {

        memcpy(mtbl->res_data->vendor_specific, buf, sizeof(mtbl->res_data->vendor_specific));

        if (obj) {
            if (cps_api_object_attr_add(obj, BASE_PAS_MEDIA_VENDOR_SPECIFIC,
                        mtbl->res_data->vendor_specific, sizeof(mtbl->res_data->vendor_specific))
                    == false) {
                PAS_ERR("Failed to add attribute to object");

                return false;
            }
        }
    }

    return true;
}

static bool dn_pas_media_default_capability_poll(uint_t port,
                cps_api_object_t obj)
{
    phy_media_tbl_t* mtbl = NULL;
    int num_cap = 0;
    media_capability_t *cap_old = NULL, *cap_new = NULL;
    
    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);
    STD_ASSERT(mtbl->res_data != NULL);

    cap_old = dn_pas_media_get_default_media_capability(mtbl);

    /* Update the default cap */
    num_cap = dn_pas_media_construct_media_capabilities(mtbl);

    if (num_cap < 1){
        PAS_ERR("Invalid number of media capabilities: %d on port(s): %s",
            port, mtbl->port_str);
    }
    if (mtbl->res_data->default_media_capability_index >= num_cap){
        PAS_ERR("Invalid default media capability on port(s): %s",
            mtbl->port_str);
        mtbl->res_data->default_media_capability_index = 0;
    }

    cap_new = dn_pas_media_get_default_media_capability(mtbl); 

    if (!dn_pas_media_compare_capabilities(cap_old, cap_new) && (obj !=NULL)) {
        if (!dn_pas_media_add_capabilities_to_obj(cap_new, obj)){
            PAS_ERR("Unable to add default capability to obj: %s",
                mtbl->port_str);
            false;
        }
    }
   return true;
}

static bool dn_pas_media_add_capabilities_to_obj(media_capability_t *cap,
                                                 cps_api_object_t *obj)
{
    bool ret = true;
    if (obj == NULL) {
        PAS_ERR("Attempt to add capability to null CPS object");
        return false;
    }
    if (cap == NULL) {
        PAS_ERR("Attempt to add null capability to CPS object");
        return false;
    }

    if (cps_api_object_attr_add(obj,
            BASE_PAS_MEDIA_DEFAULT_MEDIA_SPEED,
            &(cap->media_speed),
            sizeof(cap->media_speed)) == false) {
        PAS_ERR("Failed to add default_speed object attr");
        ret &= false;
    }

    if (cps_api_object_attr_add(obj,
            BASE_PAS_MEDIA_DEFAULT_BREAKOUT_SPEED,
            &(cap->breakout_speed),
            sizeof(cap->breakout_speed)) == false) {
        PAS_ERR("Failed to add default_breakout_speed object attr");
        ret &= false;
    }

    if (cps_api_object_attr_add(obj,
            BASE_PAS_MEDIA_DEFAULT_BREAKOUT_MODE,
            &(cap->breakout_mode),
            sizeof(cap->breakout_mode)) == false) {
        PAS_ERR("Failed to add default_breakout_mode object attr");
        ret &= false;
    }

    if (cps_api_object_attr_add(obj,
            BASE_PAS_MEDIA_DEFAULT_PHY_MODE,
            &(cap->phy_mode),
            sizeof(cap->phy_mode)) == false) {
        PAS_ERR("Failed to add default_phy_mode object attr");
        ret &= false;
    }

    return ret;
}



/*
 * dn_pas_media_oir_poll is to poll the basic information of the media,
 * to publish on media presence detection.
 */

static bool dn_pas_media_oir_poll (uint_t port, cps_api_object_t obj)
{
    bool                 ret = true;
    bool                 presence;
    phy_media_tbl_t      *mtbl = NULL;


    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    presence = dn_pas_phy_media_is_present(port);

    if (dn_pas_media_presence_poll(port, obj) == false) {
        PAS_ERR("Failed to get media module presence, port %u",
                port
                );

        return false;
    }
    bool cur_presence = dn_pas_phy_media_is_present(port);
    if ((cur_presence == true)
            && (mtbl->res_data->type == PLATFORM_MEDIA_TYPE_SFP_T)) {

        if (cur_presence != presence) {
            mtbl->channel_data[PAS_MEDIA_CH_START].is_link_status_valid = false;
        }
        dn_pas_media_channel_serdes_control(port, PAS_MEDIA_CH_START);
    }

    if (presence == cur_presence) {
        return ret;
    }

    if (dn_pas_phy_media_is_present(port) == false) {

        dn_pas_media_high_power_mode_set(port, false);

        PAS_NOTICE("Optic removed from front panel port (%d).", port);

        return ret;
    }

    if (dn_pas_media_category_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media category, port %u",
                port
                );

        ret = false;
    } else {
        if (mtbl->res_data->supported_feature_valid == false) {
            if (pas_sdi_media_feature_support_status_get(
                        mtbl->res_hdl, &mtbl->res_data->supported_feature)
                    != STD_ERR_OK) {
                ret = false;
            } else {

                mtbl->res_data->supported_feature_valid = true;

                if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)
                        || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
                        || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)
                        || (mtbl->res_data->category
                            == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS)) {

                    mtbl->res_data->rate_select_state =
                        mtbl->res_data->supported_feature.qsfp_features.rate_select_status;
                } else {

                    mtbl->res_data->rate_select_state =
                        mtbl->res_data->supported_feature.sfp_features.rate_select_status;
                }
            }
        }
    }

    if (dn_pas_media_dq_poll(port, obj) == false) {

        ret = false;
    }

    if (dn_pas_media_wavelength_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media wavelength, port %u",
                port
                );

        ret = false;
    }
    if (dn_pas_media_vendor_oui_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media vendor OUI, port %u",
                port
                );

        ret = false;
    }

    if (dn_pas_media_type_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media type, port %u",
                port
                );

        ret = false;
    }

    PAS_NOTICE("Optic inserted in front panel port (%d), qualified: %s.",
            port, (mtbl->res_data->qualified == true) ?
            "Yes" : "No");

    if (dn_pas_media_capability_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media capability, port %u",
                port
                );

        ret = false;
    }

    if (dn_pas_media_vendor_name_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media vendor name, port %u",
                port
                );

        ret = false;
    }

    if (dn_pas_media_default_capability_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media breakout info, port %u",
                port
                );

        ret = false;
    }

    return ret;
}

/* dn_pas_is_media_obj_empty function to check is object empty or not.
 * if object is empty it returns true otherwise false.
 */

bool dn_pas_is_media_obj_empty (cps_api_object_t obj, BASE_PAS_OBJECTS_t objid)
{
    cps_api_object_it_t    it;
    cps_api_attr_id_t      attr_id;
    bool                   ret = true;

    if (obj == NULL) return true;

    if ((objid != BASE_PAS_MEDIA_OBJ)
            && (objid != BASE_PAS_MEDIA_CHANNEL_OBJ)) {
        return true;
    }

    cps_api_object_it_begin(obj, &it);

    while (cps_api_object_it_valid(&it)) {

        attr_id = cps_api_object_attr_id (it.attr);


        if (dn_pas_media_is_key_attr(objid, attr_id) == false) {
            ret = false;
            break;
        }

        cps_api_object_it_next (&it);
    }

    return ret;
}

/*
 * dn_pas_phy_media_attr_lookup function returns true if the specified
 * attribute is found otherwise returns false.
 */

static bool dn_pas_phy_media_attr_lookup (cps_api_object_t obj,
        BASE_PAS_MEDIA_t attr, BASE_PAS_OBJECTS_t objid)
{
    cps_api_object_it_t    it;
    cps_api_attr_id_t      attr_id;
    bool                   ret = false;

    if (obj == NULL) return false;

    cps_api_object_it_begin(obj, &it);

    if (attr == (BASE_PAS_MEDIA_t)PAS_MEDIA_NO_ALARM) {
        return !dn_pas_is_media_obj_empty(obj, objid);
    }

    while (cps_api_object_it_valid(&it)) {

        attr_id = cps_api_object_attr_id (it.attr);

        if (attr_id == attr) {

            ret = true;
            break;
        }

        cps_api_object_it_next (&it);
    }

    return ret;
}

/*
 * dn_pas_phy_media_is_present is to get the media presence status
 * from the local data structure.
 */

bool dn_pas_phy_media_is_present (uint_t port)
{
    phy_media_tbl_t          *mtbl = NULL;

    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    return mtbl->res_data->present;
}

void dn_pas_phy_media_channel_poll (uint_t port, uint_t channel, bool publish)
{

    cps_api_object_t      obj = CPS_API_OBJECT_NULL;
    uint_t                slot;


    obj = publish ? cps_api_object_create() : CPS_API_OBJECT_NULL;

    if (dn_pas_media_channel_rtd_poll(port, channel, obj) == false) {
        PAS_ERR("Failed to poll media channel real-time data, port %u channel %u",
                port, channel
                );
    }

    if (obj != CPS_API_OBJECT_NULL) {
        if (!dn_pas_phy_media_attr_lookup(obj,
                    PAS_MEDIA_NO_ALARM,
                    BASE_PAS_MEDIA_CHANNEL_OBJ
                    )
           ) {
            cps_api_object_delete(obj);
        } else {
            dn_pas_myslot_get(&slot);

            dn_pas_obj_key_media_channel_set(obj,
                    cps_api_qualifier_OBSERVED,
                    true, slot,
                    false, PAS_MEDIA_INVALID_PORT_MODULE,
                    true, port,
                    true, channel
                    );

            dn_media_obj_publish(obj);
        }
    }
}

void dn_pas_phy_media_channel_poll_all (uint_t port, bool publish)
{
    uint_t               channel, channel_cnt;
    phy_media_tbl_t      *mtbl = NULL;

    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return;
    }

    if (dn_pas_phy_media_is_present(port) == true) {

        channel_cnt = mtbl->channel_cnt;

        for (channel = PAS_MEDIA_CH_START; channel < channel_cnt; channel++) {
            dn_pas_phy_media_channel_poll(port, channel, publish);
        }
    }
}


/*
 * dn_pas_phy_media_poll is to poll media info for specified port.
 */

void dn_pas_phy_media_poll (uint_t port, bool publish)
{

    phy_media_tbl_t      *mtbl = NULL;
    cps_api_object_t     obj = CPS_API_OBJECT_NULL;
    uint_t               slot = PAS_MEDIA_MY_SLOT;
    bool                 presence;
    bool                 rtd_poll = false;

    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return;
    }

    /* Start by checking if media is pollable (pluggable)*/
    if (!dn_pas_is_port_pluggable(port)){
        return;
    }
    presence = mtbl->res_data->present;

    obj = publish ? cps_api_object_create() : CPS_API_OBJECT_NULL;

    if (dn_pas_media_oir_poll(port, obj) == false) {
        PAS_ERR("Failed to poll media OIR, port %u", port);

        mtbl->res_data->present = false;

        if (obj != CPS_API_OBJECT_NULL) {
            cps_api_object_delete(obj);
            obj = CPS_API_OBJECT_NULL;
        }
        return;
    }

    if (dn_pas_phy_media_is_present(port) == true) {
        struct pas_config_media *cfg = dn_pas_config_media_get();

        if ((mtbl->res_data->polling_count > 0)
                && (mtbl->res_data->polling_count <= cfg->rtd_interval)) {
            mtbl->res_data->polling_count += 1;
        } else {

            mtbl->res_data->polling_count = 1;
            rtd_poll = true;
        }

        if (rtd_poll == true) {
            if (dn_pas_media_rtd_poll(port, obj) == false) {
                PAS_ERR("Failed to poll media real-time data, port %u", port);
            }

            if (dn_pas_media_high_power_mode_poll(port, obj) == false) {
                PAS_ERR("Failed to poll media high-power mode, port %u", port);
            }
        }
    }

    if ((dn_pas_phy_media_is_present(port) == true)
            &&(presence != dn_pas_phy_media_is_present(port))) {

        /*Record Insertion: store unix time and increment count*/
        time_t insert_time;
        time(&insert_time);
        mtbl->res_data->insertion_timestamp = (uint64_t)&insert_time;
        ++mtbl->res_data->insertion_cnt;

        if (obj != CPS_API_OBJECT_NULL) {

            cps_api_object_delete(obj);
            obj = CPS_API_OBJECT_NULL;
        }

        if (dn_pas_media_data_poll(port, obj) == false) {
            PAS_ERR("Failed to poll media data, port %u", port);
        }

        if (dn_pas_media_threshold_poll(port, obj) == false) {
            PAS_ERR("Failed to poll media threshold, port %u", port);
        }

        if (dn_pas_media_vendor_info_poll(port, obj) == false) {
            PAS_ERR("Failed to poll media vendor info, port %u", port);
        }

        if (publish == true) {
            dn_pas_media_data_publish(port, pp_list,
                    ARRAY_SIZE(pp_list), false);
        }
    }

    if (obj != CPS_API_OBJECT_NULL) {

        if (dn_pas_phy_media_attr_lookup(obj, PAS_MEDIA_NO_ALARM,
                    BASE_PAS_MEDIA_OBJ) == true) {

            if (mtbl->res_data->present == false) {
                cps_api_attr_id_t attr = BASE_PAS_MEDIA_TYPE;

                dn_pas_media_obj_all_attr_add(media_mem_info,
                        ARRAY_SIZE(media_mem_info), &attr, 1, obj,
                        mtbl->res_data);
            }
            dn_pas_media_add_port_info_to_cps_obj(obj, mtbl, port);
            dn_pas_media_add_channel_info_to_cps_obj(obj, mtbl, port);

            dn_pas_obj_key_media_set(obj, cps_api_qualifier_REALTIME, true, slot,
                    false, PAS_MEDIA_INVALID_PORT_MODULE, true, port);

            dn_media_obj_publish(obj);
            obj = CPS_API_OBJECT_NULL;
        }
    }

    if (obj != CPS_API_OBJECT_NULL) cps_api_object_delete(obj);

    if (rtd_poll == true) {
        dn_pas_phy_media_channel_poll_all(port, publish);
    }
}

/*
 * dn_pas_phy_media_poll_all is to poll all media resources on the local
 * system board.
 */

void dn_pas_phy_media_poll_all (void *arg)
{

    uint_t           cnt;

    for (cnt = PAS_MEDIA_START_PORT; cnt <= phy_media_count; cnt++) {

        dn_pas_phy_media_poll(cnt, true);

    }
}

/*
 * dn_pas_media_obj_attr_add function to add media attribute to an object,
 * based on index.
 */

static bool dn_pas_media_obj_attr_add (phy_media_member_info_t const *memp,
        cps_api_object_t obj, void * res_data)
{
    bool                             ret = true;

    if ((obj == NULL) || (res_data == NULL)
            || (memp == NULL)) {
        PAS_ERR("Invalid parameter");

        return false;
    }

    if (memp->type == cps_api_object_ATTR_T_U16) {

        uint16_t value16 = *((uint16_t *) (((char *)res_data) + memp->offset));

        if (cps_api_object_attr_add_u16(obj, memp->ident, value16) == false) {
            ret = false;
        }

    } else if (memp->type == cps_api_object_ATTR_T_U32) {

        uint32_t value32 = *((uint32_t *) (((char *)res_data) + memp->offset));

        if (cps_api_object_attr_add_u32(obj, memp->ident, value32) == false) {
            ret = false;
        }

    } else if (memp->type == cps_api_object_ATTR_T_U64) {

        uint64_t value64 = *((uint64_t *) (((char *)res_data) + memp->offset));

        if (cps_api_object_attr_add_u64(obj, memp->ident, value64) == false) {
            ret = false;
        }

    } else if (memp->type ==  cps_api_object_ATTR_T_BIN) {

        void * data = (((char *) res_data) + memp->offset);

        if (cps_api_object_attr_add(obj, memp->ident, data, memp->size) == false) {
            ret = false;
        }

    } else {
        ret = false;
    }

    return ret;
}

/*
 * dn_pas_media_obj_attr_list_add: used when one needs to add from a list (array)
 * Example: say some struct S has an array A with N elements each of size B in bytes;
 * memp->offset = offsetof(S, A[0]), res_data = &S, member_size = B, count = n (where n=[1,N])
 */

static bool dn_pas_media_obj_attr_list_add (phy_media_member_info_t *memp,
        cps_api_object_t obj, void * res_data, size_t member_size, int count) {

    phy_media_member_info_t* tmp = (phy_media_member_info_t*)
                       malloc(sizeof(phy_media_member_info_t));
    *tmp = *memp;
    STD_ASSERT(count > 0);

    while (( count-- > 0) && (dn_pas_media_obj_attr_add (tmp, obj, res_data))) {
        tmp->offset += member_size;
    }

    free (tmp);
    return (count == -1);
}


/*
 * dn_pas_media_obj_all_attr_add function to add media attributes to an object,
 * based on attribute list, if list is null it populates all the attributes
 * of media object.
 */

bool dn_pas_media_obj_all_attr_add (phy_media_member_info_t const *memp,
        uint_t max_count, cps_api_attr_id_t const *list, uint_t count,
        cps_api_object_t obj, void *res_data)
{

    uint_t               cnt, inx;
    bool                 ret = true;

    if ((obj == NULL) || (res_data == NULL)) {
        PAS_ERR("Invalid parameter");

        return false;
    }

    if (list == NULL) {
        for (cnt = PAS_MEDIA_MEMBER_START; cnt < max_count; cnt++) {
            if (dn_pas_media_obj_attr_add(&memp[cnt], obj, res_data)
                    == false) {
                PAS_ERR("Failed to add attribute to object");

                ret = false;
            }
        }
    } else {
        /*
         * If list is non NULL, then it points to the list of attributes
         * which need to be added and count holds the number of attributes,
         * need to loop through the attribute list and find the corresponding
         * entry from the media member table and add the attribute to the
         * object.
         */

        for (cnt = 0; cnt < count; cnt++) {

            for (inx = PAS_MEDIA_MEMBER_START; inx < max_count; inx++) {

                if (memp[inx].ident == list[cnt]) {

                    break;
                }
            }

            if (inx >= max_count) {
                PAS_ERR("Invalid attribute id (%u)", list[cnt]);

                continue;
            }


            if (dn_pas_media_obj_attr_add(&memp[inx], obj, res_data) == false) {
                PAS_ERR("Failed to add attribute to object");

                ret = false;
            }
        }
    }

    return ret;
}

/*
 * dn_pas_channel_get is a helper function to handle the get
 * request of channel.
 */

bool dn_pas_channel_get (cps_api_qualifier_t qualifier, uint_t slot,
        uint_t port, uint_t channel, cps_api_get_params_t * param,
        cps_api_object_t req_obj)
{
    phy_media_tbl_t         *mtbl;
    cps_api_object_t        obj =  CPS_API_OBJECT_NULL;
    pas_media_channel_t     *chdata;
    bool                    ret = true;

    mtbl = dn_phy_media_entry_get(port);

    if (mtbl == NULL) {
        PAS_ERR("No media record of port (%u)", port);
        return false;
    }

    if (!dn_pas_is_media_present(port)){
        PAS_ERR("Attempt to get channel data on vacant port (%u)", port);
        return false;
    }

    if ((dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Error getting channel %u on port (%u)",channel , port);
        PAS_ERR("Port %u valid channels are 0 to %u",
                                 port, -1 + mtbl->channel_cnt);
        return false;
    }

    chdata = &(mtbl->channel_data[channel]);

    if ((obj = cps_api_object_create()) == NULL) {
        PAS_ERR("Failed to create CPS API object");

        return false;
    }


    dn_pas_obj_key_media_channel_set(obj, qualifier, true, slot, false,
            PAS_MEDIA_INVALID_PORT_MODULE, true, port, true, channel);

    if (dn_pas_is_media_obj_empty(req_obj, BASE_PAS_MEDIA_CHANNEL_OBJ)
            == true) {
        if (dn_pas_media_obj_all_attr_add(media_ch_mem_info,
                    ARRAY_SIZE(media_ch_mem_info), NULL, 0, obj, chdata)
                == false) {
            PAS_ERR("Failed to add attributes to object");

            ret = false;
        }
    } else {
        if (dn_pas_media_populate_current_data(BASE_PAS_MEDIA_CHANNEL_OBJ,
                    req_obj, obj, port, channel) == false) {
            PAS_ERR("Failed to populate object");

            ret = false;
        }
    }

    if (!cps_api_object_list_append(param->list, obj)) {
        PAS_ERR("Failed to append response object");

        cps_api_object_delete(obj);
        ret = false;
    }

    return ret;

}



/*
 * dn_pas_port_channel_get is a helper function to handle port level get
 * request of channel.
 */

bool dn_pas_port_channel_get (cps_api_qualifier_t qualifier, uint_t slot,
        uint_t port, cps_api_get_params_t *param, cps_api_object_t req_obj)
{
    phy_media_tbl_t     *mtbl;
    uint_t              channel;
    bool                ret = true;


    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    for (channel = PAS_MEDIA_CH_START; channel < mtbl->channel_cnt; channel++) {
        if(dn_pas_channel_get(qualifier, slot, port, channel, param, req_obj)
                == false) {
            PAS_ERR("Failed to get media channel, port %u channel %u",
                    port, channel
                    );

            ret = false;
        }
    }

    return ret;
}

/*
 * dn_pas_media_channel_ext_rate_select to set extended rate select bits per channel.
 */

bool dn_pas_media_channel_ext_rate_select (uint_t port, uint_t channel, bool enable)
{
    phy_media_tbl_t *mtbl;
    t_std_error  rc = STD_ERR_OK;
    sdi_media_fw_rev_t rev = SDI_MEDIA_FW_REV0;


    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);
        return false;
    }

    rev = pas_media_fw_rev_get(mtbl->res_data);

    rc = sdi_media_ext_rate_select(mtbl->res_hdl, channel, rev, enable);
    if (rc != STD_ERR_OK) {
        PAS_ERR("Error %d when attempting TX-side rate select on port %u channel %u",
                rc, mtbl->fp_port, channel);
        return false;
    }

    return true;
}

/*
 * dn_pas_media_channel_cdr_enable is to handle CDR enable/disable per channel.
 */

bool dn_pas_media_channel_cdr_enable (uint_t port, uint_t channel, bool enable)
{
    bool ret = true;
    phy_media_tbl_t *mtbl;
    t_std_error  rc = STD_ERR_OK;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)) {
        if ((rc = sdi_media_cdr_status_set(mtbl->res_hdl, channel, enable))
                    != STD_ERR_OK) {
            if (STD_ERR_EXT_PRIV(ret) != EOPNOTSUPP) {
                ret = false;
            }
        }
    }

    return ret;
}

/*
 * dn_pas_media_channel_cdr_get - get the CDR status per channel.
 */

bool dn_pas_media_channel_cdr_get (uint_t port, uint_t channel, bool * enable)
{

    bool ret = true;
    phy_media_tbl_t *mtbl;
    t_std_error  rc = STD_ERR_OK;

    if (enable == NULL) return false;

    *enable = false;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
            || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)) {
        if ((rc = sdi_media_cdr_status_get(mtbl->res_hdl, channel, enable))
                != STD_ERR_OK) {
            if (STD_ERR_EXT_PRIV(rc) != EOPNOTSUPP) {
                ret = false;
            }
        }
    }
    return ret;
}


/*
 * dn_pas_media_channel_state_set is handle the set request of the
 * channel state.
 */

bool dn_pas_media_channel_state_set (uint_t port, uint_t channel,
        bool state)
{
    bool ret = true;
    phy_media_tbl_t *mtbl;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if (((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD)
                || (mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS))
            && (mtbl->res_data->supported_feature.qsfp_features.tx_control_support_status
                == false)) return true;

    if (pas_sdi_media_tx_control(mtbl->res_hdl, channel, state)
            != STD_ERR_OK) {
        PAS_ERR("Failed to get media tx control, port %u channel %",
                port, channel
                );

        ret = false;
    } else {
        mtbl->channel_data[channel].state = state;
    }

    return ret;

}

/*
 * dn_pas_media_channel_serdes_control is to control Fiber/Serdes TX and RX 
 * based on phy link status and tx-disable.
 */

bool dn_pas_media_channel_serdes_control (uint_t port, uint_t channel)
{
    bool ret = true;
    phy_media_tbl_t *mtbl;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_NOTICE("Invalid port (%u)", port);
        return false;
    }

    if ((mtbl->res_data->type == PLATFORM_MEDIA_TYPE_SFP_T)
            && (mtbl->channel_data[channel].state == true)) {
        bool state = false;
        bool serdes_set = false;

        if (sdi_media_phy_link_status_get(mtbl->res_hdl, channel, SDI_MEDIA_DEFAULT,
                    &state) == STD_ERR_OK) {
            if (state == false) {
                sdi_media_phy_link_status_get(mtbl->res_hdl, channel, SDI_MEDIA_DEFAULT,
                        &state);
            }

            if ((mtbl->channel_data[channel].is_link_status_valid == false)
                    || (mtbl->channel_data[channel].phy_link_status != state)) {
                mtbl->channel_data[channel].is_link_status_valid = true;
                serdes_set = true;
            } 
            mtbl->channel_data[channel].phy_link_status = state;

            if (serdes_set == true) {
                if (sdi_media_phy_serdes_control(mtbl->res_hdl, channel,
                            SDI_MEDIA_DEFAULT, state) != STD_ERR_OK) {
                    ret = false;
                    PAS_ERR("Serdes control failed, port(%u), channel(%u), state(%u)",
                            port, channel, state);
                }
            }

        } else {
            PAS_ERR("PHY link status get failed port(%u), channel (%u)",
                    port, channel);
            ret = false;
        }

    }
    return ret;
}


/*
 * dn_pas_media_channel_led_set is to set the led per channel based on the speed.
 */

bool dn_pas_media_channel_led_set (uint_t port, uint_t channel,
        BASE_IF_SPEED_t speed)
{
    bool               ret = true;
    phy_media_tbl_t    *mtbl;
    sdi_media_speed_t  sdi_speed;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if ((sdi_speed = dn_pas_to_sdi_capability_conv(speed))
            == (sdi_media_speed_t)PAS_MEDIA_INVALID_ID) {
        PAS_ERR("Invalid speed, port %u, channel %u, speed %u",
                port, channel, speed
                );

        return false;
    }

    if (pas_sdi_media_led_set(mtbl->res_hdl, channel, sdi_speed) != STD_ERR_OK) {
        PAS_ERR("Failed to set media channel LED, port %u, channel %u, speed %u",
                port, channel, sdi_speed
                );

        ret = false;
    }


    return ret;
}

/*
 * dn_pas_media_phy_autoneg_config_set is to handle autoneg config set,
 * it updates in memory config and also pushes config to phy if
 * media is present.
 */

bool dn_pas_media_phy_autoneg_config_set (uint_t port, uint_t channel, bool autoneg,
        cps_api_operation_types_t operation)
{
    phy_media_tbl_t         *mtbl;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if (operation == cps_api_oper_DELETE) {
        mtbl->channel_data[channel].is_conf &= ~(1 << MEDIA_AUTONEG);
        mtbl->channel_data[channel].autoneg = false;
    } else {
        mtbl->channel_data[channel].autoneg = autoneg;
        mtbl->channel_data[channel].is_conf |= (1 << MEDIA_AUTONEG);
    }

    if (dn_pas_phy_media_is_present(port) == true) {
        return dn_pas_media_phy_autoneg_set(port, channel);
    }

    return true;
}

/*
 * dn_pas_media_phy_config_entry_get is find the default config entry for
 * specified media type, it returns pas_media_phy_defaults entry if it finds
 * and entry for the specified media type otherwise NULL.
 */

pas_media_phy_defaults * dn_pas_media_phy_config_entry_get(
        PLATFORM_MEDIA_TYPE_t type)
{
    pas_config_media_phy    *phy_config = dn_pas_config_media_phy_get();
    pas_media_phy_defaults  *phy_config_entry = NULL;
    uint_t                  index;

    for (index = 0; index < phy_config->count; index++) {
        if (phy_config->media_phy_defaults[index].media_type == type) {
            phy_config_entry = &phy_config->media_phy_defaults[index];
            break;
        }
    }

    return phy_config_entry;
}

/*
 * dn_pas_media_phy_autoneg_set is to push autoneg config to phy
 * for the specified port.
 */

bool dn_pas_media_phy_autoneg_set (uint_t port, uint_t channel)
{
    phy_media_tbl_t         *mtbl;
    pas_media_phy_defaults  *phy_config_entry = NULL;
    bool                    autoneg;
    sdi_media_type_t        type;


    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    phy_config_entry = dn_pas_media_phy_config_entry_get(mtbl->res_data->type);

    /* Dont need special handling/push any config to phy*/
    if (phy_config_entry == NULL) return true;

    if ( mtbl->channel_data[channel].is_conf & (1 << MEDIA_AUTONEG)) {
        autoneg = mtbl->channel_data[channel].autoneg;
    } else {
        autoneg = phy_config_entry->intrl_phy_autoneg;
    }

    type = dn_pas_to_sdi_type_conv(mtbl->res_data->type);

    if (sdi_media_phy_autoneg_set(mtbl->res_hdl, channel, type,
                                  autoneg) != STD_ERR_OK) {
        PAS_ERR("Failed to set autoneg, port %u, channel %u, autoneg %s",
                port, channel, autoneg ? "on" : "off"
                );

        return false;
    }

    return true;
}

/*
 * dn_pas_media_phy_supported_speed_set is to push supported speed
 * config to phy for the specified port.
 */

bool dn_pas_media_phy_supported_speed_set (uint_t port, uint_t channel)
{
    phy_media_tbl_t         *mtbl;
    pas_media_phy_defaults  *phy_config_entry = NULL;
    sdi_media_speed_t       sdi_sup_speed[MAX_SUPPORTED_SPEEDS];
    uint_t                  index, count;
    BASE_IF_SPEED_t         *sup_speed;
    sdi_media_type_t        type;


    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    phy_config_entry = dn_pas_media_phy_config_entry_get(mtbl->res_data->type);

    /* Dont need special handling/push any config to phy*/
    if (phy_config_entry == NULL) return true;

    if ( mtbl->channel_data[channel].is_conf & (1 << MEDIA_SUPPORTED_SPEED)) {
        sup_speed =  mtbl->channel_data[channel].supported_speed;
        count = mtbl->channel_data[channel].supported_speed_count;
    } else {
        sup_speed = phy_config_entry->intrl_phy_supported_speed;
        count = phy_config_entry->intrl_phy_supported_speed_count;
    }

    for (index = 0; index < count; index++) {
        sdi_sup_speed[index] = dn_pas_to_sdi_capability_conv(sup_speed[index]);
    }

    type = dn_pas_to_sdi_type_conv(mtbl->res_data->type);

    if (sdi_media_phy_speed_set(mtbl->res_hdl, channel, type, sdi_sup_speed,
                count) != STD_ERR_OK) {
        PAS_ERR("Failed to set speed, port %u, channel %u",
                port, channel
                );

        return false;
    }

    return true;
}

/*
 * dn_pas_media_phy_interface_mode_set is to push default interface mode
 * config to phy for the specified port.
 */

bool dn_pas_media_phy_interface_mode_set (uint_t port, uint_t channel)
{
    phy_media_tbl_t         *mtbl;
    pas_media_phy_defaults  *phy_config_entry = NULL;
    uint_t                  mode;
    sdi_media_type_t        type;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    phy_config_entry = dn_pas_media_phy_config_entry_get(mtbl->res_data->type);

    /* Dont need special handling/push any config to phy*/

    if (phy_config_entry == NULL) return true;

    if (strcmp(phy_config_entry->interface_mode, "SGMII") == 0) {
        mode = SDI_MEDIA_MODE_SGMII;
    } else if (strcmp(phy_config_entry->interface_mode, "GMII") == 0) {
        mode = SDI_MEDIA_MODE_GMII;
    } else if (strcmp(phy_config_entry->interface_mode, "MII") == 0) {
        mode = SDI_MEDIA_MODE_MII;
    } else if (strcmp(phy_config_entry->interface_mode, "SFI") == 0) {
        mode = 4;
    } else {
        return false;
    }

    type = dn_pas_to_sdi_type_conv(mtbl->res_data->type);

    if (sdi_media_phy_mode_set(mtbl->res_hdl, channel, type, mode) != STD_ERR_OK) {
        PAS_ERR("Failed to set phy mode, port %u, channel %u, mode %u",
                port, channel, mode
                );

        return false;
    }

    return true;
}


/*
 * dn_pas_media_phy_supported_speed_config_set is to handle supported
 * speed config set, it updates in memory config and also pushes
 * config to phy if media is present.
 */

bool dn_pas_media_phy_supported_speed_config_set (uint_t port, uint_t channel,
        BASE_IF_SPEED_t *sup_speed, uint_t count, cps_api_operation_types_t operation)
{
    phy_media_tbl_t         *mtbl;

    if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
            || (dn_phy_is_media_channel_valid(port, channel) == false)){
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if (operation == cps_api_oper_DELETE) {
        // get deault value and set it in phy
        mtbl->channel_data[channel].is_conf &= ~(1 << MEDIA_SUPPORTED_SPEED);
        memset(mtbl->channel_data[channel].supported_speed, 0,
                sizeof(mtbl->channel_data[channel].supported_speed));
        mtbl->channel_data[channel].supported_speed_count = 0;
    } else {

        memcpy(mtbl->channel_data[channel].supported_speed, sup_speed,
                sizeof(mtbl->channel_data[channel].supported_speed));

        mtbl->channel_data[channel].supported_speed_count = count;

        mtbl->channel_data[channel].is_conf |= (1 << MEDIA_SUPPORTED_SPEED);
    }


    if (dn_pas_phy_media_is_present(port) == true) {
        return dn_pas_media_phy_supported_speed_set(port, channel);
    }

    return true;
}


/*
 * dn_pas_media_high_power_mode_set is to handle the set request of
 * high power mode of the media.
 */

bool dn_pas_media_high_power_mode_set (uint_t port, bool mode)
{
    bool               ret = true;
    phy_media_tbl_t    *mtbl;
    bool               lr_mode = false;

    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if ((mtbl->res_data->category == PLATFORM_MEDIA_CATEGORY_SFP)
            || (mtbl->res_data->category ==  PLATFORM_MEDIA_CATEGORY_SFP_PLUS)) {
        return true;
    }

    lr_mode = (mode == true) ? false : true;

    if (pas_sdi_media_module_control(mtbl->res_hdl, SDI_MEDIA_LP_MODE,
                lr_mode) != STD_ERR_OK) {
        PAS_ERR("Failed to set media module LP mode, port %u, LP mode %s",
                port, lr_mode ? "on" : "off"
                );

        ret = false;
    }

    return ret;

}
/*
 * dn_pas_media_module_control is to handle set requests of media
 * module control attributes.
 */

bool dn_pas_media_module_control (uint_t port,
        sdi_media_module_ctrl_type_t type, bool enable)
{
    bool              ret = true;
    phy_media_tbl_t   *mtbl;

    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if (pas_sdi_media_module_control(mtbl->res_hdl, type,
                enable) != STD_ERR_OK) {
        PAS_ERR("Failed to set media module control, port %u, type %u, %s",
                port, type, enable ? "on" : "off"
                );

        ret = false;
    }

    return ret;
}

bool dn_pas_media_lockdown_handle (bool lockdown)
{
    uint_t                  max_count, port;
    phy_media_tbl_t         *mtbl;
    struct pas_config_media *cfg = dn_pas_config_media_get();
    bool                    ret = true;

    if (lockdown == cfg->lockdown) return true;

    cfg->lockdown = lockdown;

    max_count = dn_phy_media_count_get();

    for (port = PAS_MEDIA_START_PORT; port <= max_count; port++) {

        if (((mtbl = dn_phy_media_entry_get(port)) == NULL)
                || (mtbl->res_data->present == false)){
            continue;
        }

        if ((mtbl->res_data->qualified == false)
                && (dn_pas_is_media_unsupported(mtbl, lockdown ))){ /* Only print message if task is to lock */
            if (dn_pas_media_transceiver_state_set(port, lockdown) == false) {
                ret = false;
            }
        }
    }
    PAS_NOTICE("All unsupported media %slocked", (lockdown ? "" : "un"));
    return ret;
}

static bool  dn_pas_media_transceiver_state_set (uint_t port, bool lockdown)
{
    bool             ret = true;
    phy_media_tbl_t  *mtbl;
    uint_t           count;
    bool             state;

    mtbl = dn_phy_media_entry_get(port);
    STD_ASSERT(mtbl != NULL);

    for (count = PAS_MEDIA_CH_START; count < mtbl->channel_cnt; count++) {

        state = (lockdown == true) ? false : mtbl->channel_data[count].tgt_state;

        if (dn_pas_media_channel_state_set(port, count, state) == false){
            ret = false;
        }
    }

    if (ret) {
        mtbl->res_data->lockdown_state = lockdown;
    }
    else {
        PAS_ERR("Error %slocking media on port %u", (lockdown ? "" :"un" ), port);
    }
    return ret;
}

/*
 * dn_pas_media_is_key_attr return true if the attribute id is a keyid
 * otherwise it false.
 */

bool dn_pas_media_is_key_attr(BASE_PAS_OBJECTS_t objid,
        cps_api_attr_id_t attr_id)
{

    if ((objid != BASE_PAS_MEDIA_OBJ)
            && (objid != BASE_PAS_MEDIA_CHANNEL_OBJ)) {
        return false;
    }

    if (attr_id == CPS_API_ATTR_KEY_ID) return true;

    if (objid == BASE_PAS_MEDIA_OBJ) {
        if ((attr_id == BASE_PAS_MEDIA_SLOT)
                || (attr_id == BASE_PAS_MEDIA_PORT_MODULE_SLOT)
                || (attr_id == BASE_PAS_MEDIA_PORT)) {

            return true;
        }
    } else if (objid == BASE_PAS_MEDIA_CHANNEL_OBJ) {
        if ((attr_id == BASE_PAS_MEDIA_CHANNEL_SLOT)
                || (attr_id == BASE_PAS_MEDIA_CHANNEL_PORT_MODULE_SLOT)
                || (attr_id == BASE_PAS_MEDIA_CHANNEL_PORT)
                || (attr_id == BASE_PAS_MEDIA_CHANNEL_CHANNEL)) {

            return true;
        }
    }
    return false;
}

/*
 * dn_pas_media_populate_current_data is to populate all the attributes in
 * rollback_obj with datastore values by traversing the attribute list of
 * request object.
 */

bool dn_pas_media_populate_current_data (BASE_PAS_OBJECTS_t objid,
        cps_api_object_t req_obj, cps_api_object_t rollback_obj,
        uint_t port, uint_t channel)
{

    phy_media_tbl_t         *mtbl = NULL;
    phy_media_member_info_t const * memp = NULL;
    cps_api_object_it_t     it;
    cps_api_attr_id_t       attr_id;
    uint_t                  max_count;
    void                    *data = NULL;


    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        return false;
    }

    if (objid == BASE_PAS_MEDIA_OBJ) {
        memp = media_mem_info;
        max_count = ARRAY_SIZE(media_mem_info);
        data =  mtbl->res_data;

    } else if (objid == BASE_PAS_MEDIA_CHANNEL_OBJ) {

        if (dn_phy_is_media_channel_valid(port, channel) == false) {
            return false;
        }
        memp = media_ch_mem_info;
        max_count = ARRAY_SIZE(media_ch_mem_info);
        data = &(mtbl->channel_data[channel]);
    } else {
        return false;
    }

    cps_api_object_it_begin(req_obj, &it);

    while (cps_api_object_it_valid (&it)) {

        attr_id = cps_api_object_attr_id (it.attr);

        if (dn_pas_media_is_key_attr(objid, attr_id) == false) {

            dn_pas_media_obj_all_attr_add(memp, max_count, &attr_id, 1,
                    rollback_obj, data);
        }

        cps_api_object_it_next (&it);
    }

    return true;
}

/** Free code */
void dn_pas_phy_media_res_free(void)
{
    uint_t port, cnt;
    uint_t slot = PAS_MEDIA_MY_SLOT;
    char   res_key[PAS_RES_KEY_SIZE];

    for (port = PAS_MEDIA_START_PORT; port <= dn_phy_media_count_get();
            port++) {

        if (phy_media_tbl[port].channel_data != NULL) {
            for (cnt = PAS_MEDIA_CH_START; cnt < phy_media_tbl[port].channel_cnt;
                    cnt++) {
                dn_pas_res_removec(dn_pas_res_key_media_chan(res_key,
                                                             sizeof(res_key),
                                                             slot,
                                                             port,
                                                             cnt
                                                             )
                                   );
            }

            free(phy_media_tbl[port].channel_data);
            phy_media_tbl[port].channel_data = NULL;
            phy_media_tbl[port].channel_cnt = 0;
        }

        dn_pas_res_removec(dn_pas_res_key_media(res_key,
                                                sizeof(res_key),
                                                slot,
                                                port
                                                )
                           );

    }

    free(phy_media_tbl[PAS_MEDIA_START_PORT].res_data);
    free(phy_media_tbl);
    phy_media_tbl = NULL;
    phy_media_count = 0;
}
/*
 * dn_pas_media_wavelength_config_set is to set the user configured wavelength
 * in local data structure and write into the eeprom if media is present.
 */

bool dn_pas_media_wavelength_config_set (uint_t port, float value,
                                         cps_api_operation_types_t operation)
{
    phy_media_tbl_t         *mtbl;


    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if (operation == cps_api_oper_DELETE) {
        mtbl->res_data->target_wavelength = 0;
    } else {
        mtbl->res_data->target_wavelength = value;
    }

    if (dn_pas_phy_media_is_present(port) == true) {
        return dn_pas_media_wavelength_set(port);
    }
    return true;
}

/*
 * dn_pas_media_wavelength_set is to write the target wavelength in eeprom.
 */

bool dn_pas_media_wavelength_set (uint_t port)
{

    phy_media_tbl_t         *mtbl;

    if ((mtbl = dn_phy_media_entry_get(port)) == NULL) {
        PAS_ERR("Invalid port (%u)", port);

        return false;
    }

    if (mtbl->res_data->type
            == PLATFORM_MEDIA_TYPE_SFPPLUS_10GBASE_ZR_TUNABLE) {

        if (sdi_media_wavelength_set(mtbl->res_hdl, mtbl->res_data->target_wavelength)
                != STD_ERR_OK) {
            PAS_ERR("Failed to set wavelength, port %u, wavelength %g",
                    port, mtbl->res_data->target_wavelength
                    );

            return false;

        }
    }
    return true;
}

