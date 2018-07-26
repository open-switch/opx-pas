/*
 * Copyright (c) 2018 Dell EMC.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/**************************************************************************
* @file pas_media_info_map.cpp
*
* @brief This file contains source code for mapping media attributes.
**************************************************************************/



#include "private/pas_log.h"
#include "private/pas_media.h"
#include "private/pas_media_sdi_wrapper.h"
#include "private/pald.h"
#include "private/dn_pas.h"
#include "private/pas_event.h"
#include "private/pas_utils.h"
#include <stdlib.h>

#include <unordered_map>
#include <iostream>
#include <string.h>

#define MAX_MEDIA_INTERFACE_DISPLAY_STR_LEN 20

/* TODO: Optimize and compress functions. Eliminate repeated ocurrence of iterator declaration */


/* Callback mapping for media property discovery */
/* Maps transciever types to functions for identifying media properties for that type */
static std::unordered_map<int, pas_media_disc_cb_t> trans_type_to_media_disc_cb_map = {
    {PLATFORM_MEDIA_CATEGORY_SFP,          dn_pas_std_media_get_basic_properties_sfp},
    {PLATFORM_MEDIA_CATEGORY_SFP_PLUS,     dn_pas_std_media_get_basic_properties_sfp_plus},
    {PLATFORM_MEDIA_CATEGORY_SFP28,        dn_pas_std_media_get_basic_properties_sfp28},
    {PLATFORM_MEDIA_CATEGORY_QSFP,         dn_pas_std_media_get_basic_properties_qsfp_plus},
    {PLATFORM_MEDIA_CATEGORY_QSFP_PLUS,    dn_pas_std_media_get_basic_properties_qsfp_plus},
    {PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28, dn_pas_std_media_get_basic_properties_qsfp28_depop},
    {PLATFORM_MEDIA_CATEGORY_QSFP28,       dn_pas_std_media_get_basic_properties_qsfp28},
    {PLATFORM_MEDIA_CATEGORY_QSFP_DD,      dn_pas_std_media_get_basic_properties_qsfp28_dd},

    /* For fixed port media types */
    {PLATFORM_MEDIA_CATEGORY_FIXED,        dn_pas_std_media_get_basic_properties_fixed_port}
};

pas_media_disc_cb_t pas_media_get_disc_cb_from_trans_type (uint_t trans_type)
{
    auto it = trans_type_to_media_disc_cb_map.find(trans_type);
    return ( it == trans_type_to_media_disc_cb_map.end()) ? NULL : (pas_media_disc_cb_t)(it->second);
}

/* For SFP info mapping. */
typedef struct {
    uint_t wavelength;
    PLATFORM_MEDIA_INTERFACE_t media_if;
} pas_sfp_wavelength_media_if_t;

static std::unordered_map<std::string, pas_sfp_wavelength_media_if_t> sfp_vpn_to_media_if_map = {
    {"FTRJ-1519-7D-CSC",  (pas_sfp_wavelength_media_if_t){ 1550, PLATFORM_MEDIA_INTERFACE_ZX}},
    {"FTLF1519P1BCL",     (pas_sfp_wavelength_media_if_t){ 1550, PLATFORM_MEDIA_INTERFACE_ZX}},
    {"FTLF1519P1WCL",     (pas_sfp_wavelength_media_if_t){ 1550, PLATFORM_MEDIA_INTERFACE_ZX}},
    {"FWDM-1619-7D-47",   (pas_sfp_wavelength_media_if_t){ 1470, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-49",   (pas_sfp_wavelength_media_if_t){ 1490, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-51",   (pas_sfp_wavelength_media_if_t){ 1510, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-53",   (pas_sfp_wavelength_media_if_t){ 1530, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-55",   (pas_sfp_wavelength_media_if_t){ 1550, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-57",   (pas_sfp_wavelength_media_if_t){ 1570, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-59",   (pas_sfp_wavelength_media_if_t){ 1590, PLATFORM_MEDIA_INTERFACE_CWDM}},
    {"FWDM-1619-7D-61",   (pas_sfp_wavelength_media_if_t){ 1610, PLATFORM_MEDIA_INTERFACE_CWDM}}
};

static std::unordered_map<uint_t, int> sfp_id_to_media_if_map = {
    {0x01, PLATFORM_MEDIA_INTERFACE_SX},
    {0x02, PLATFORM_MEDIA_INTERFACE_LX},
    {0x04, PLATFORM_MEDIA_INTERFACE_CX},
    {0x08, PLATFORM_MEDIA_INTERFACE_BASE_T},
    {0x10, PLATFORM_MEDIA_INTERFACE_LX},
    {0x20, PLATFORM_MEDIA_INTERFACE_FX},
    {0x40, PLATFORM_MEDIA_INTERFACE_BX10},
    {0x80, PLATFORM_MEDIA_INTERFACE_PX}
};

bool pas_media_get_sfp_info_from_part_no (char* part_no, uint_t* wavelength, PLATFORM_MEDIA_INTERFACE_t* media_if)
{
    std::string s = std::string(part_no);
    auto it = sfp_vpn_to_media_if_map.find(s);
    if (it == sfp_vpn_to_media_if_map.end()) {
        return false;
    }
    *wavelength = it->second.wavelength;
    *media_if = it->second.media_if;
    return true;
}

PLATFORM_MEDIA_INTERFACE_t pas_media_get_sfp_media_if_from_id (uint_t id)
{
    auto it = sfp_id_to_media_if_map.find(id);
    return ( it == sfp_id_to_media_if_map.end()) ? PLATFORM_MEDIA_INTERFACE_UNKNOWN : (PLATFORM_MEDIA_INTERFACE_t)(it->second);
}


/* Maps media category to basic attributes as well as display string */

typedef struct {
    const char* display_string;
    uint_t channel_count;
    bool supports_breakout;
} transceiver_type_info_t;

static std::unordered_map<int, transceiver_type_info_t> transceiver_type_info_map = {
    {PLATFORM_MEDIA_CATEGORY_SFP,          (transceiver_type_info_t) {"SFP", 1, false}},
    {PLATFORM_MEDIA_CATEGORY_SFP_PLUS,     (transceiver_type_info_t) {"SFP+", 1, false}},
    {PLATFORM_MEDIA_CATEGORY_SFP28,        (transceiver_type_info_t) {"SFP28", 1, false}},
    {PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28, (transceiver_type_info_t) {"QSFP28-DEPOP", 2, true}},
    {PLATFORM_MEDIA_CATEGORY_QSFP,         (transceiver_type_info_t) {"QSFP+", 4, true}},
    {PLATFORM_MEDIA_CATEGORY_QSFP_PLUS,    (transceiver_type_info_t) {"QSFP+", 4, true}},
    {PLATFORM_MEDIA_CATEGORY_QSFP28,       (transceiver_type_info_t) {"QSFP28", 4, true}},
    {PLATFORM_MEDIA_CATEGORY_QSFP_DD,      (transceiver_type_info_t) {"QSFP28-DD", 8, true}},

    {PLATFORM_MEDIA_CATEGORY_FIXED,      (transceiver_type_info_t) {"FIXED", 1, false}},
};


const char* pas_media_get_transceiver_type_display_string (pas_media_transceiver_type trans_type)
{
    auto it = transceiver_type_info_map.find(trans_type);
    return ( it == transceiver_type_info_map.end()) ? NULL : it->second.display_string;
}

uint_t pas_media_get_transceiver_type_channel_count (pas_media_transceiver_type trans_type)
{
    auto it = transceiver_type_info_map.find(trans_type);
    return ( it == transceiver_type_info_map.end()) ? 1 : it->second.channel_count;
}

bool pas_media_get_transceiver_type_is_breakout_supported (pas_media_transceiver_type trans_type)
{
    auto it = transceiver_type_info_map.find(trans_type);
    return ( it == transceiver_type_info_map.end()) ? false : it->second.supports_breakout;
}

/* Maps the breakout mode to the far end and near end channel count */

typedef struct {
    uint_t   near_end_channel_count;
    uint_t   far_end_channel_count;
} pas_media_breakout_info_t;

static std::unordered_map<int, pas_media_breakout_info_t> breakout_info_map = {
    {BASE_CMN_BREAKOUT_TYPE_NO_BREAKOUT,  (pas_media_breakout_info_t){1,1}},
    {BASE_CMN_BREAKOUT_TYPE_BREAKOUT_1X1, (pas_media_breakout_info_t){1,1}},
    {BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, (pas_media_breakout_info_t){1,2}},
    {BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, (pas_media_breakout_info_t){2,2}},
    {BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, (pas_media_breakout_info_t){1,4}},
    {BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X2, (pas_media_breakout_info_t){2,4}},
    {BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, (pas_media_breakout_info_t){2,8}}
};

uint_t pas_media_map_get_breakout_near_end_val (BASE_CMN_BREAKOUT_TYPE_t brk)
{
    auto it = breakout_info_map.find(brk);
    return ( it == breakout_info_map.end()) ? 1 : it->second.near_end_channel_count;
}
uint_t pas_media_map_get_breakout_far_end_val (BASE_CMN_BREAKOUT_TYPE_t brk)
{
    auto it = breakout_info_map.find(brk);
    return ( it == breakout_info_map.end()) ? 1 : it->second.far_end_channel_count;
}

/* Speed enum to uint map. Useful for performing operations on speed enums */

typedef struct {
    uint_t                  value_in_mpbs;
    BASE_IF_PHY_MODE_TYPE_t phy_mode;
} pas_media_speed_info_t;


static std::unordered_map<int, pas_media_speed_info_t> speed_info_map = {
    {BASE_IF_SPEED_0MBPS,      (pas_media_speed_info_t){0, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_10MBPS,     (pas_media_speed_info_t){10, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_100MBPS,    (pas_media_speed_info_t){100, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_1GIGE,      (pas_media_speed_info_t){1000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_1GFC,       (pas_media_speed_info_t){1000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_2GFC,       (pas_media_speed_info_t){2000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_4GIGE,      (pas_media_speed_info_t){4000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_4GFC,       (pas_media_speed_info_t){4000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_8GFC,       (pas_media_speed_info_t){8000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_10GIGE,     (pas_media_speed_info_t){10000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_16GFC,      (pas_media_speed_info_t){16000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_20GIGE,     (pas_media_speed_info_t){20000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_25GIGE,     (pas_media_speed_info_t){25000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_32GFC,      (pas_media_speed_info_t){32000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_40GIGE,     (pas_media_speed_info_t){40000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_50GIGE,     (pas_media_speed_info_t){50000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_64GFC,      (pas_media_speed_info_t){64000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_100GIGE,    (pas_media_speed_info_t){100000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_128GFC,     (pas_media_speed_info_t){128000, BASE_IF_PHY_MODE_TYPE_FC}},
    {BASE_IF_SPEED_200GIGE,    (pas_media_speed_info_t){200000, BASE_IF_PHY_MODE_TYPE_ETHERNET}},
    {BASE_IF_SPEED_400GIGE,    (pas_media_speed_info_t){400000, BASE_IF_PHY_MODE_TYPE_ETHERNET}}
};

uint_t pas_media_map_get_speed_as_uint_mbps (BASE_IF_SPEED_t speed)
{
    auto it = speed_info_map.find(speed);
    return ( it == speed_info_map.end()) ? 0 : it->second.value_in_mpbs;
}
BASE_IF_PHY_MODE_TYPE_t pas_media_map_get_phy_mode_from_speed (BASE_IF_SPEED_t speed)
{
    auto it = speed_info_map.find(speed);
    return (BASE_IF_PHY_MODE_TYPE_t)(( it == speed_info_map.end()) ? 0 : it->second.phy_mode);
}

/* This maps defined media interface enums to properties specific to that enum */

typedef struct _media_interface_prop_t {
    PLATFORM_MEDIA_CONNECTOR_TYPE_t             connector_type_expected;
    PLATFORM_MEDIA_CABLE_TYPE_t                 cable_type_expected;
    char                                        disp_string[MAX_MEDIA_INTERFACE_DISPLAY_STR_LEN];
} media_interface_prop_t;


static std::unordered_map<int, media_interface_prop_t> media_interface_prop_map = {
    {PLATFORM_MEDIA_INTERFACE_UNKNOWN, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, ""}},
    {PLATFORM_MEDIA_INTERFACE_BASE_T, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_RJ45, PLATFORM_MEDIA_CABLE_TYPE_RJ45, "T"}},
    {PLATFORM_MEDIA_INTERFACE_FX, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "FX"}},
    {PLATFORM_MEDIA_INTERFACE_BIDI, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "BIDI"}},
    {PLATFORM_MEDIA_INTERFACE_BX10, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "BX10"}},
    {PLATFORM_MEDIA_INTERFACE_BX40, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "BX40"}},
    {PLATFORM_MEDIA_INTERFACE_BX80, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "BX80"}},
    {PLATFORM_MEDIA_INTERFACE_CR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_COPPER_PIGTAIL, PLATFORM_MEDIA_CABLE_TYPE_DAC, "CR"}},
    {PLATFORM_MEDIA_INTERFACE_SR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "SR"}},
    {PLATFORM_MEDIA_INTERFACE_LR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "LR"}},
    {PLATFORM_MEDIA_INTERFACE_ER, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "ER"}},
    {PLATFORM_MEDIA_INTERFACE_ZR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "ZR"}},
    {PLATFORM_MEDIA_INTERFACE_USR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "USR"}},
    {PLATFORM_MEDIA_INTERFACE_CWDM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "CWDM"}},
    {PLATFORM_MEDIA_INTERFACE_DWDM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "DWDM"}},
    {PLATFORM_MEDIA_INTERFACE_SWDM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "SWDM"}},
    {PLATFORM_MEDIA_INTERFACE_SX, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "SX"}},
    {PLATFORM_MEDIA_INTERFACE_LX, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "LX"}},
    {PLATFORM_MEDIA_INTERFACE_ZX, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "ZX"}},
    {PLATFORM_MEDIA_INTERFACE_LM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "LM"}},
    {PLATFORM_MEDIA_INTERFACE_LRM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "LRM"}},
    {PLATFORM_MEDIA_INTERFACE_PSM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "PSM"}},
    {PLATFORM_MEDIA_INTERFACE_PLR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "PLR"}},
    {PLATFORM_MEDIA_INTERFACE_SM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "SM"}},
    {PLATFORM_MEDIA_INTERFACE_SW, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "SW"}},
    {PLATFORM_MEDIA_INTERFACE_LW, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "LW"}},
    {PLATFORM_MEDIA_INTERFACE_RJ45, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_RJ45, PLATFORM_MEDIA_CABLE_TYPE_RJ45, "RJ45"}},
    {PLATFORM_MEDIA_INTERFACE_ELECTRICAL_LOOPBACK, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_RJ45, PLATFORM_MEDIA_CABLE_TYPE_RJ45, "ELECTRICAL LOOPBACK"}},
    {PLATFORM_MEDIA_INTERFACE_BACKPLANE, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_BACKPLANE, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, "BACKPLANE"}},
    {PLATFORM_MEDIA_INTERFACE_ESR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "ESR"}},
    {PLATFORM_MEDIA_INTERFACE_CX, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "CX"}},
    {PLATFORM_MEDIA_INTERFACE_PX, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "PX"}},
};

PLATFORM_MEDIA_CONNECTOR_TYPE_t pas_media_get_media_interface_connector_type_expected (PLATFORM_MEDIA_INTERFACE_t media_if)
{
    auto it = media_interface_prop_map.find(media_if);
    return ( it == media_interface_prop_map.end()) ? PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN : (PLATFORM_MEDIA_CONNECTOR_TYPE_t)(it->second.connector_type_expected);
}

PLATFORM_MEDIA_CABLE_TYPE_t pas_media_get_media_interface_cable_type_expected (PLATFORM_MEDIA_INTERFACE_t media_if)
{
    auto it = media_interface_prop_map.find(media_if);
    return ( it == media_interface_prop_map.end()) ? PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN : (PLATFORM_MEDIA_CABLE_TYPE_t)(it->second.cable_type_expected);
}

const char* pas_media_get_media_interface_disp_string (PLATFORM_MEDIA_INTERFACE_t media_if)
{
    auto it = media_interface_prop_map.find(media_if);
    return ( it == media_interface_prop_map.end()) ? NULL : it->second.disp_string;
}

/* This maps defined media interface qualifier enums to properties specific to that enum */

static std::unordered_map<int, media_interface_prop_t> media_interface_qualifier_prop_map = {
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, ""}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER,  (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, ""}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_IR,  (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "IR"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_LR, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "LR"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_1490NM, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "1490NM"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_30M, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, "30M"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_UP, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "UP"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_DOWN, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "DOWN"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_ACC, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_COPPER_PIGTAIL, PLATFORM_MEDIA_CABLE_TYPE_ACC, "ACC"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_NOF, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, "NOF"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_TUNABLE, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, PLATFORM_MEDIA_CABLE_TYPE_FIBER, "TUNABLE"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_LITE, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, "LITE"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_FIXED, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN, PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN, "FIXED"}},
    {PLATFORM_MEDIA_INTERFACE_QUALIFIER_AOC, (media_interface_prop_t) {PLATFORM_MEDIA_CONNECTOR_TYPE_NON_SEPARABLE, PLATFORM_MEDIA_CABLE_TYPE_AOC, "AOC"}}
};


PLATFORM_MEDIA_CONNECTOR_TYPE_t pas_media_get_media_interface_qualifier_connector_type_expected (PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_if_qual)
{
    auto it = media_interface_qualifier_prop_map.find(media_if_qual);
    return ( it == media_interface_qualifier_prop_map.end()) ? PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN : (PLATFORM_MEDIA_CONNECTOR_TYPE_t)(it->second.connector_type_expected);
}

PLATFORM_MEDIA_CABLE_TYPE_t pas_media_get_media_interface_qualifier_cable_type_expected (PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_if_qual)
{
    auto it = media_interface_qualifier_prop_map.find(media_if_qual);
    return ( it == media_interface_qualifier_prop_map.end()) ? PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN : (PLATFORM_MEDIA_CABLE_TYPE_t)(it->second.cable_type_expected);
}

const char* pas_media_get_media_interface_qualifier_disp_string (PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_if_qual)
{
    auto it = media_interface_qualifier_prop_map.find(media_if_qual);
    return ( it == media_interface_qualifier_prop_map.end()) ? NULL : it->second.disp_string;
}

/* This maps defined MSA connector values to platform enums  */

static std::unordered_map<int, int> media_connector_enum_map = {
    {SDI_MEDIA_CONNECTOR_LC, PLATFORM_MEDIA_CONNECTOR_TYPE_LC},
    {SDI_MEDIA_CONNECTOR_MU, PLATFORM_MEDIA_CONNECTOR_TYPE_MU},
    {SDI_MEDIA_CONNECTOR_OPTICAL_PIGTAIL, PLATFORM_MEDIA_CONNECTOR_TYPE_OPTICAL_PIGTAIL},
    {SDI_MEDIA_CONNECTOR_MPO_1X12, PLATFORM_MEDIA_CONNECTOR_TYPE_MPO_1X12},
    {SDI_MEDIA_CONNECTOR_MPO_2X16, PLATFORM_MEDIA_CONNECTOR_TYPE_MPO_2X16},
    {SDI_MEDIA_CONNECTOR_COPPER_PIGTAIL, PLATFORM_MEDIA_CONNECTOR_TYPE_COPPER_PIGTAIL},
    {SDI_MEDIA_CONNECTOR_RJ45, PLATFORM_MEDIA_CONNECTOR_TYPE_RJ45},
    {SDI_MEDIA_CONNECTOR_NON_SEPARABLE, PLATFORM_MEDIA_CONNECTOR_TYPE_NON_SEPARABLE},
    {SDI_MEDIA_CONNECTOR_MXC_2X16, PLATFORM_MEDIA_CONNECTOR_TYPE_MXC_2X16},
};

PLATFORM_MEDIA_CONNECTOR_TYPE_t pas_media_get_media_connector_enum (sdi_media_connector_t conn)
{
    auto it = media_connector_enum_map.find(conn);
    return ( it == media_connector_enum_map.end()) ? PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN : (PLATFORM_MEDIA_CONNECTOR_TYPE_t)(it->second);
}

/* Map of connector types to whether or not they are separable */

static std::unordered_map<int, bool> media_connector_separable_map = {
    {PLATFORM_MEDIA_CONNECTOR_TYPE_LC, true},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_MU, true},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_OPTICAL_PIGTAIL, true},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_MPO_1X12, true},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_MPO_2X16, true},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_COPPER_PIGTAIL, false},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_RJ45, true},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_NON_SEPARABLE, false},
    {PLATFORM_MEDIA_CONNECTOR_TYPE_MXC_2X16, true},
};

bool pas_media_is_media_connector_separable (PLATFORM_MEDIA_CONNECTOR_TYPE_t conn, bool *result)
{
    auto it = media_connector_separable_map.find(conn);
    if ( it == media_connector_separable_map.end()) {
        return false;
    }
    *result = (bool)(it->second);
    return true;
}

static std::unordered_map<int, std::string> qsa_enum_to_str_map = {
    {PLATFORM_QSA_ADAPTER_UNKNOWN,      "(QSA?)"},
    {PLATFORM_QSA_ADAPTER_NO_QSA,       "\0"},
    {PLATFORM_QSA_ADAPTER_QSA,          "(QSA)"},
    {PLATFORM_QSA_ADAPTER_QSA28,        "(QSA28)"},
};

const char* pas_media_get_qsa_string_from_enum (PLATFORM_QSA_ADAPTER_t qsa_type)
{
    auto it = qsa_enum_to_str_map.find(qsa_type);
    return ( it == qsa_enum_to_str_map.end()) ? NULL : (const char*)(it->second.c_str());
}

/*Code beyind this point is to ensure backwards compatibility */

/* This section assigns the PLATFORM_MEDIA_TYPE_t enum to new media, which previously do not have a type enum derivation from MSA */
/* This is done to maintain compatibility with other areas which have not migrated to string-based model yet */

static std::unordered_map<std::string, int> media_name_to_enum_type = {
    {"QSFP28 100GBASE BIDI", PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_BIDI},
    {"QSFP28 100GBASE ESR4", PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_ESR4},
    {"QSFP28 100GBASE SR4 NOF", PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_SR4_NOF},
    {"SFP+ 10GBASE BX10 UP", PLATFORM_MEDIA_TYPE_SFP_PLUS_10GBASE_BX10_UP},
    {"SFP+ 10GBASE BX40 UP", PLATFORM_MEDIA_TYPE_SFP_PLUS_10GBASE_BX40_UP},
    {"SFP+ 10GBASE BX10 DOWN", PLATFORM_MEDIA_TYPE_SFP_PLUS_10GBASE_BX10_DOWN},
    {"SFP+ 10GBASE BX40 DOWN", PLATFORM_MEDIA_TYPE_SFP_PLUS_10GBASE_BX40_DOWN},
    {"QSFP+ 40GBASE BIDI", PLATFORM_MEDIA_TYPE_QSFPPLUS_40GBASE_BIDI},
    {"QSFP28 100GBASE DWDM2", PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_DWDM2},
    {"QSFP28 100GBASE ER4 LITE", PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_ER4_LITE},
    {"QSFP28 100GBASE SWDM4", PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_SWDM4},
    {"QSFP+ 40GBASE SR4 AOC 1.0M", PLATFORM_MEDIA_TYPE_QSFPPLUS_40GBASE_SR4_AOC1M},

};

PLATFORM_MEDIA_TYPE_t  pas_media_get_enum_from_new_media_name (char* name)
{
    std::string s = std::string(name);
    auto it = media_name_to_enum_type.find(s);
    if (it == media_name_to_enum_type.end()) {
        return PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
    }

    return (PLATFORM_MEDIA_TYPE_t)(it->second);
}

