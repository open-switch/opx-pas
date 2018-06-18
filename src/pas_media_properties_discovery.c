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
 * @file pas_media_properties_discovery.c
 *
 * @brief This file contains source code for obtaining media properties from MSA fields.
 ***************************************************************************/


#include "private/pas_log.h"
#include "private/pas_media.h"
#include "private/pas_media_sdi_wrapper.h"
#include "private/pas_data_store.h"
#include "private/pald.h"
#include "private/dn_pas.h"
#include "private/pas_event.h"
#include "private/pas_utils.h"
#include <stdlib.h>

/* All code in this file is based on MSA spec. There is no set pattern or logic to determining the values and relationships between them */

 /*
 * Obtain non-pluggable port properties
 */

bool dn_pas_std_media_get_basic_properties_fixed_port (phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t  media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_SINGLE;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;

    switch (mtbl->res_data->port_type) {
        case PLATFORM_PORT_TYPE_FIXED:
            media_interface = PLATFORM_MEDIA_INTERFACE_RJ45;
            break;
        case PLATFORM_PORT_TYPE_BACKPLANE:
            media_interface = PLATFORM_MEDIA_INTERFACE_BACKPLANE;
            break;
        default:
            break;
    }

    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;

    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);
}

/*
 * Obtain QSFP28-DD properties
 */

bool dn_pas_std_media_get_basic_properties_qsfp28_dd(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t  media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = MEDIA_INTERFACE_LANE_COUNT_DEFAULT;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;



    uint_t options = mtbl->res_data->options;

    switch ( (options >> 24) & 0xFF ) {
        case PAS_MEDIA_QSFP28_DD_ID_2SR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_DOUBLE_DENSITY;
            break;

        case PAS_MEDIA_QSFP28_DD_ID_2SR4_AOC:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_AOC;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_DOUBLE_DENSITY;
            break;

        case PAS_MEDIA_QSFP28_DD_ID_2CWDM4:
            media_interface = PLATFORM_MEDIA_INTERFACE_CWDM;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_DOUBLE_DENSITY;
            break;

        case PAS_MEDIA_QSFP28_DD_ID_2PSM4_IR:
            media_interface = PLATFORM_MEDIA_INTERFACE_PSM;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_IR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_DOUBLE_DENSITY;
            break;


        case PAS_MEDIA_QSFP28_DD_ID_2CR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_DOUBLE_DENSITY;
            break;

        default:
            PAS_ERR("Error identifying QSFP28-DD media on port(s) %s", mtbl->port_str);
            break;
    }
    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;

    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);
}

/*
 * Obtain QSFP28 properties
 */

bool dn_pas_std_media_get_basic_properties_qsfp28(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = MEDIA_INTERFACE_LANE_COUNT_DEFAULT;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;
    PLATFORM_EXT_SPEC_COMPLIANCE_CODE_t ext_spec_code =
            PLATFORM_EXT_SPEC_COMPLIANCE_CODE_NOT_APPLICABLE;

    uint_t options = mtbl->res_data->options;
    uint_t transmitter_code = mtbl->res_data->device_tech >> PAS_QSFP_TRANS_TECH_OFFSET;
    uint_t length_sfm_km = mtbl->res_data->length_sfm_km;
    uint_t length_om4 = mtbl->res_data->length_cable;

    switch ((options >> QSFP28_OPTION1_BIT_SHIFT) &
            (QSFP28_OPTION1_BIT_MASK)) {
        case QSFP_100GBASE_AOC:
        case PAS_MEDIA_QSFP28_ID2_SR4_AOC:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_AOC;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            break;
        case QSFP_100GBASE_SR4:
            media_interface = ((length_om4 == 0x96)
                          ? PLATFORM_MEDIA_INTERFACE_ESR
                          : PLATFORM_MEDIA_INTERFACE_SR);
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            break;
        case PAS_MEDIA_QSFP28_ID_ER4:
            media_interface = PLATFORM_MEDIA_INTERFACE_ER;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = ((length_sfm_km == 0x1E)
                          ? PLATFORM_MEDIA_INTERFACE_QUALIFIER_LITE
                          : PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER);
            break;
        case QSFP_100GBASE_LR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_LR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        case QSFP_100GBASE_CWDM4:
            media_interface = PLATFORM_MEDIA_INTERFACE_CWDM;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;            
            break;
        case QSFP_100GBASE_PSM4_IR:
            media_interface = PLATFORM_MEDIA_INTERFACE_PSM;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_IR;
            break;
        case PAS_MEDIA_QSFP28_ID_CR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_ACC;
           break;
        case PAS_MEDIA_QSFP28_ID_SWDM4:
            media_interface = PLATFORM_MEDIA_INTERFACE_SWDM;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case PAS_MEDIA_QSFP28_ID_BIDI:
            media_interface = PLATFORM_MEDIA_INTERFACE_BIDI;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case PAS_MEDIA_QSFP28_ID_DWDM2:
            media_interface = PLATFORM_MEDIA_INTERFACE_DWDM;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_DOUBLE;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        /* Special DAC cases */
        case PAS_MEDIA_QSFP28_ID_CR4_CA_L:
            ext_spec_code = PLATFORM_EXT_SPEC_COMPLIANCE_CODE_25GBASE_CR_CA_L;
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case PAS_MEDIA_QSFP28_ID_CR4_CA_S:
            ext_spec_code = PLATFORM_EXT_SPEC_COMPLIANCE_CODE_25GBASE_CR_CA_S;
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case PAS_MEDIA_QSFP28_ID_CR4_CA_N:
            ext_spec_code = PLATFORM_EXT_SPEC_COMPLIANCE_CODE_25GBASE_CR_CA_N;
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        default:
            break;
    }

    if (media_interface == PLATFORM_MEDIA_INTERFACE_UNKNOWN) {
        switch (transmitter_code){
            case QSFP_COPPER_UNEQ:
            case QSFP_COPPER_PASSIVE_EQ:
            case QSFP_COPPER_NEAR_FAR_EQ:
            case QSFP_COPPER_FAR_EQ:
            case QSFP_COPPER_NEAR_EQ:
            case QSFP_COPPER_LINEAR_ACTIVE: 
                media_interface = PLATFORM_MEDIA_INTERFACE_CR;
                media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            default:
                break;
        }
    }

    media_info->ext_spec_code_25g_dac = ext_spec_code;
    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;
    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);
}

/*
 * Obtain QSFP+ properties
 */

bool dn_pas_std_media_get_basic_properties_qsfp_plus(phy_media_tbl_t *mtbl,  dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = MEDIA_INTERFACE_LANE_COUNT_DEFAULT;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;

    uint_t transmitter_code = mtbl->res_data->device_tech >> PAS_QSFP_TRANS_TECH_OFFSET;

    sdi_media_transceiver_descr_t *trans_desc = (sdi_media_transceiver_descr_t *) (mtbl->res_data->transceiver);

    switch (trans_desc->qsfp_descr.sdi_qsfp_eth_1040g_code) {
        case QSFP_40GBASE_LR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_LR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        case QSFP_40GBASE_SR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        case QSFP_100GBASE_CR4:
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        case QSFP_40G_ACTIVE_CABLE:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_AOC;
            break;
        default:
            break;
    }

    if (media_interface == PLATFORM_MEDIA_INTERFACE_UNKNOWN) {
        switch (transmitter_code){
            case QSFP_COPPER_UNEQ:
            case QSFP_COPPER_PASSIVE_EQ:
            case QSFP_COPPER_NEAR_FAR_EQ:
            case QSFP_COPPER_FAR_EQ:
            case QSFP_COPPER_NEAR_EQ:
            case QSFP_COPPER_LINEAR_ACTIVE:
                media_interface = PLATFORM_MEDIA_INTERFACE_CR;
                media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
                break;
            default:
                break;
        }
    }

    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;
    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);

}

/*
 * Obtain SFP+ properties
 */

bool dn_pas_std_media_get_basic_properties_sfp_plus(phy_media_tbl_t *mtbl,  dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t  media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_SINGLE;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;

    sdi_media_transceiver_descr_t *trans_desc = (sdi_media_transceiver_descr_t *)(mtbl->res_data->transceiver);
    uint_t wavelength = mtbl->res_data->wavelength;
    uint_t length_cable_cm = 100 * (mtbl->res_data->length_cable);

    media_capability_t cap;

    if (false == dn_pas_media_set_capability_values(
             &cap,
             BASE_IF_SPEED_10GIGE,
             BASE_CMN_BREAKOUT_TYPE_BREAKOUT_1X1,
             BASE_IF_SPEED_10GIGE,
             BASE_IF_PHY_MODE_TYPE_ETHERNET)) {
        return false;
    }

    switch (trans_desc->sfp_descr.sdi_sfp_eth_10g_code) {
        case SFP_10GBASE_SR:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case SFP_10GBASE_LR:
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            switch (wavelength){
                case PAS_MEDIA_BX_UP_WAVELENGTH_ID:
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX10;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UP;
                    break;
                case PAS_MEDIA_BX_DOWN_WAVELENGTH_ID:
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX10;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_DOWN;
                    break;
                default:
                    media_interface = PLATFORM_MEDIA_INTERFACE_LR;
                    break;
            }
            break;
        case SFP_10GBASE_LRM:
            media_interface = PLATFORM_MEDIA_INTERFACE_LRM;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        case SFP_10GBASE_ER:
            switch (wavelength){
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
                case PAS_MEDIA_BX_UP_WAVELENGTH_ID:
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX40;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UP;
                    break;
                case PAS_MEDIA_BX_DOWN_WAVELENGTH_ID:
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX40;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_DOWN;
                    break;
                default:
                    media_interface = PLATFORM_MEDIA_INTERFACE_ER;
                    break;
            }
            break;    
        default:
            break;
    }

    if (media_interface == PLATFORM_MEDIA_INTERFACE_UNKNOWN) {
        /* DAC */
        switch (trans_desc->sfp_descr.sdi_sfp_plus_cable_technology) {
            case SFP_PLUS_PASSIVE_CABLE:
                media_interface = PLATFORM_MEDIA_INTERFACE_CR;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
                if (length_cable_cm > 500) {
                    PAS_ERR("Invalid passive DAC cable length in port(s) %s ", mtbl->port_str);
                }
                break;
            case SFP_PLUS_ACTIVE_CABLE:
                media_interface = PLATFORM_MEDIA_INTERFACE_SR;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_AOC;
                break;
            default:
                break;
        }
    }

    if (media_interface == PLATFORM_MEDIA_INTERFACE_UNKNOWN) {
        /* FC */
        switch (trans_desc->sfp_descr.sdi_sfp_fc_media & 0x04){
            case PAS_MEDIA_SFP_PLUS_ID_FC_LW:
                media_interface = PLATFORM_MEDIA_INTERFACE_LW;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
                cap.phy_mode   = BASE_IF_PHY_MODE_TYPE_FC;
                cap.media_speed = dn_pas_max_fc_supported_speed(trans_desc->sfp_descr.sdi_sfp_fc_speed);
                cap.breakout_speed = cap.media_speed;
                break;
            case PAS_MEDIA_SFP_PLUS_ID_FC_SW:
                media_interface = PLATFORM_MEDIA_INTERFACE_SW;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
                cap.phy_mode   = BASE_IF_PHY_MODE_TYPE_FC;
                cap.media_speed = dn_pas_max_fc_supported_speed(trans_desc->sfp_descr.sdi_sfp_fc_speed);
                cap.breakout_speed = cap.media_speed;
                break;
            default:
                PAS_ERR("Error identifying FC media on port(s) %s", mtbl->port_str);
            break;
        }
    }
    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;
    media_info->capability_list[0] = cap;
    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);
}

/*
 * Obtain SFP28 properties
 */

bool dn_pas_std_media_get_basic_properties_sfp28(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t  media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_SINGLE;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;

    PLATFORM_EXT_SPEC_COMPLIANCE_CODE_t ext_spec_code =
            PLATFORM_EXT_SPEC_COMPLIANCE_CODE_NOT_APPLICABLE;

    uint_t ext_transceiver = mtbl->res_data->ext_transceiver;
    uint_t length_om3      = mtbl->res_data->length_om3;
    uint_t length_sfm_km   = mtbl->res_data->length_sfm_km;
    uint_t len_cable       = mtbl->res_data->length_cable;

    switch ( ext_transceiver ) {
        case PAS_MEDIA_SFP28_ID_SR:
            media_interface = PLATFORM_MEDIA_INTERFACE_SR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            if ((length_om3 == 0x7) && (len_cable == 0xA)) {
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            } else if ((length_om3 == 0x3) && (len_cable == 0x4)) {
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NOF;
            } else if ((length_om3 == 0x14) && (len_cable == 0x1E)) {
                media_interface = PLATFORM_MEDIA_INTERFACE_ESR;
            }
            break;

        case PAS_MEDIA_SFP28_ID_LR:
            if (length_sfm_km == 0x3){
                media_interface = PLATFORM_MEDIA_INTERFACE_LR;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            } else if (length_om3 == 0x2) {
                media_interface = PLATFORM_MEDIA_INTERFACE_LR;
                media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_LITE;
            }
            break;

        case PAS_MEDIA_SFP28_ID_CR_CA_L:
            ext_spec_code = PLATFORM_EXT_SPEC_COMPLIANCE_CODE_25GBASE_CR_CA_L;
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case PAS_MEDIA_SFP28_ID_CR_CA_S:
            ext_spec_code = PLATFORM_EXT_SPEC_COMPLIANCE_CODE_25GBASE_CR_CA_S;
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;
        case PAS_MEDIA_SFP28_ID_CR_CA_N:
            ext_spec_code = PLATFORM_EXT_SPEC_COMPLIANCE_CODE_25GBASE_CR_CA_N;
            media_interface = PLATFORM_MEDIA_INTERFACE_CR;
            media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
            break;

        default:
            PAS_ERR("Error identifying SFP28 media on port(s) %s", mtbl->port_str);
            break;
    }

    media_info->ext_spec_code_25g_dac = ext_spec_code;
    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;
    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);
}

/*
 * Obtain QSFP28-DEPOP properties
 */

bool dn_pas_std_media_get_basic_properties_qsfp28_depop(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t  media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = MEDIA_INTERFACE_LANE_COUNT_DEFAULT;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;


    media_interface = PLATFORM_MEDIA_INTERFACE_CR;
    media_interface_lane_count = PAS_MEDIA_INTERFACE_LANE_COUNT_DOUBLE;
    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;

    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;    
    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);

}

/*
 * Obtain SFP properties
 */

bool dn_pas_std_media_get_basic_properties_sfp(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info)
{
    PLATFORM_MEDIA_INTERFACE_t  media_interface = PLATFORM_MEDIA_INTERFACE_UNKNOWN;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN;
    uint_t media_interface_lane_count = 1;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t media_interface_prefix = PAS_MEDIA_INTERFACE_PREFIX_NORMAL;

    uint_t pas_id, wavelen;
    char* part_no = (char*)(mtbl->res_data->vendor_pn);
    uint_t sdi_id = mtbl->res_data->transceiver[SFP_GIGE_XCVR_CODE_OFFSET];

    uint_t op_wavelength = mtbl->res_data->wavelength;
    uint_t length_sfm_km   = mtbl->res_data->length_sfm_km;

    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER;
    if (pas_media_get_sfp_info_from_part_no(part_no, &wavelen, &media_interface) == false){
        pas_id = pas_media_get_sfp_media_if_from_id(sdi_id);
        if (pas_id != PLATFORM_MEDIA_INTERFACE_UNKNOWN) {
            media_interface = pas_id;
            if ((pas_id == PLATFORM_MEDIA_INTERFACE_LX)
                    && (op_wavelength == 1550)
                    && (length_sfm_km <= 80)) {

                media_interface = PLATFORM_MEDIA_INTERFACE_ZX;
            } else if (pas_id == PLATFORM_MEDIA_INTERFACE_BX10) {
                if ((op_wavelength == 0x051E)
                        && (length_sfm_km == 0xA)) {
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX10;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UP;
                } else if ((op_wavelength == 0x051E)
                        && (length_sfm_km == 0x28)) {
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX40;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UP;
                } else if ((op_wavelength == 0x05D2)
                        && (length_sfm_km == 0x50)) {
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX80;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_UP;
                } else if ((op_wavelength == 0x05D2)
                        && (length_sfm_km == 0xA)) {
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX10;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_DOWN;
                } else if ((op_wavelength == 0x05D2)
                        && (length_sfm_km == 0x28)) {
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX40;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_DOWN;
                } else if ((op_wavelength == 0x060E)
                        && (length_sfm_km == 0x50)) {
                    media_interface = PLATFORM_MEDIA_INTERFACE_BX80;
                    media_interface_qualifier = PLATFORM_MEDIA_INTERFACE_QUALIFIER_DOWN;
                }
            }

        } else if ((op_wavelength != 0xFFFF)
                && (op_wavelength != 0x0)) {
            if (op_wavelength < 1000) {
                media_interface = PLATFORM_MEDIA_INTERFACE_SX;
            } else if (op_wavelength < 1350) {
                media_interface = PLATFORM_MEDIA_INTERFACE_LX;
            }
        } else {
            media_interface = PLATFORM_MEDIA_INTERFACE_ZX;
        }
    }

    media_info->media_interface = media_interface;
    media_info->media_interface_qualifier = media_interface_qualifier;
    media_info->media_interface_lane_count = media_interface_lane_count;
    media_info->media_interface_prefix = media_interface_prefix;
    return (media_interface != PLATFORM_MEDIA_INTERFACE_UNKNOWN);
}
