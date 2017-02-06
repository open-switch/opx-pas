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
 * @file pas_media_utils.c
 *
 * @brief This file contains source code of physical media handling.
 ***************************************************************************/

#include "private/pas_media.h"
#include "private/pas_log.h"
#include "private/pas_config.h"
#include "sdi_media.h"
#include <stdlib.h>

#define ARRAY_SIZE(a)         (sizeof(a)/sizeof(a[0]))

static uint_t dn_pas_media_pas_id_get (const sdi_to_pas_map_t *pmap,
        uint_t count, uint_t id);
/*
 * identifier to pas media category map table.
 */

static const sdi_to_pas_map_t id_to_category [] = {
    {0x03, PLATFORM_MEDIA_CATEGORY_SFP_PLUS},
    {0x0b, PLATFORM_MEDIA_CATEGORY_SFP_PLUS},
    {0x0c, PLATFORM_MEDIA_CATEGORY_QSFP},
    {0x0d, PLATFORM_MEDIA_CATEGORY_QSFP_PLUS},
    {0x0e, PLATFORM_MEDIA_CATEGORY_CXP},
    {0x11, PLATFORM_MEDIA_CATEGORY_QSFP28},
    {0x12, PLATFORM_MEDIA_CATEGORY_CXP28}
};

/*
 * SDI speed types to BASE interface speed types map table.
 */

static const sdi_to_pas_map_t speed_to_capability [] = {
    {SDI_MEDIA_SPEED_10M, BASE_IF_SPEED_10MBPS},
    {SDI_MEDIA_SPEED_100M, BASE_IF_SPEED_100MBPS},
    {SDI_MEDIA_SPEED_1G, BASE_IF_SPEED_1GIGE},
    {SDI_MEDIA_SPEED_10G, BASE_IF_SPEED_10GIGE},
    {SDI_MEDIA_SPEED_25G, BASE_IF_SPEED_25GIGE},
    {SDI_MEDIA_SPEED_40G, BASE_IF_SPEED_40GIGE},
    {SDI_MEDIA_SPEED_100G, BASE_IF_SPEED_100GIGE}
};

/*
 * Media category to channel count map table.
 */

static const sdi_to_pas_map_t channel_count [] = {
    {PLATFORM_MEDIA_CATEGORY_SFP, 1},
    {PLATFORM_MEDIA_CATEGORY_SFP_PLUS, 1},
    {PLATFORM_MEDIA_CATEGORY_SFP28, 1},
    {PLATFORM_MEDIA_CATEGORY_CXP, 1},
    {PLATFORM_MEDIA_CATEGORY_QSFP, 4},
    {PLATFORM_MEDIA_CATEGORY_QSFP_PLUS, 4},
    {PLATFORM_MEDIA_CATEGORY_QSFP28, 4},
    {PLATFORM_MEDIA_CATEGORY_CXP28, 4}
};

/*
 * PAS media type to SDI media type map table.
 */

static sdi_to_pas_map_t media_type_map [] = {
    {QSFP_4X1_1000BASE_T, PLATFORM_MEDIA_TYPE_AR_4X1_1000BASE_T}
};

/*
 * Wave length, protocol and distance to media type map table
 */

static const media_type_map_t media_qsfp_type_tbl [] = {
    {1, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_SR4},
    {2, 1, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_LR4},
    {2, 2, 1, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFP_40GBASE_ER4},
    {2, 2, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_LR4},
    {3, 3, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_LR4},
    {4, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_LM4},
    {5, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_PSM4_LR},
    {6, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP_40GBASE_SM4},
    {7, 1, 1, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFP_40GBASE_BIDI},
    {8, 2, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_1M},
    {9, 6, QSFP_PROTO_4x10GBASE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_HALFM},
    {9, 2, QSFP_PROTO_4x10GBASE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_1M},
    {9, 3, QSFP_PROTO_4x10GBASE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_3M},
    {9, 4, QSFP_PROTO_4x10GBASE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_5M},
    {9, 7, QSFP_PROTO_4x10GBASE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_7M},
    {9, 9, QSFP_PROTO_4x10GBASE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP_4X10_10GBASE_CR1_2M},
    {9, 6, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_HALFM},
    {9, 2, 4, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2_1M},
    {9, 2, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_1M},
    {9, 9, 4, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2_2M},
    {9, 9, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_2M},
    {9, 3, 4, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2_3M},
    {9, 3, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_3M},
    {9, 4, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_5M},
    {9, 7, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_7M},
    {9, 5, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_10M},
    {9, 8 , MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_50M},
    {9, MEDIA_DIST_DONT_CARE, 4,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2},
    {9, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4},
    {10, 5, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_SR4},
    {10, 4, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4},
    {10, 8, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_SR4},
    {10, MEDIA_DIST_DONT_CARE, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_4X_10GBASE_SR_AOCXXM},
    {12, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_SR4_EXT},
    {13, 2, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_PSM4_1490NM_1M},
    {13, 3, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_PSM4_1490NM_3M},
    {13, 4, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_PSM4_1490NM_5M},
    {13, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_PSM4_1490NM},
    {14, MEDIA_DIST_DONT_CARE, QSFP_PROTO_4x1GBASET,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_4X1_1000BASE_T}
};

static const media_type_map_t media_qsfp28_type_tbl [] = {
    {1, 1, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_SR4},
    {2, 1, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_LR4},
    {2, 4, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_LR4_LITE},
    {4, 0, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CWDM4},
    {5, 0, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_PSM4_IR},
    {6, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_SWDM4},
    {9, 0, 1,
        1, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_1M},
    {9, 0, 1,
        2, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_2M},
    {9, 0, 1,
        3, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_3M},
    {9, 0, 1,
        4, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_4M},
    {9, 0, 1,
        5, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_5M},
    {9, 0, 1,
        7, PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_CR4_7M},
    {9, 0, 1,
        10, PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_CR4_10M},
    {9, 0, 1,
        50, PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_CR4_50M},
    {9, 0, 1,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4},
    {9, 0, 2,
        1, PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_1M},
    {9, 0, 2,
        2, PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_2M},
    {9, 0, 2,
        3, PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_3M},
    {9, 0, 2,
        4, PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_4M},
    {9, 0, 2,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1},
    {9, 0, 3,
        1, PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_1M},
    {9, 0, 3,
        2, PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_2M},
    {9, 0, 3,
        3, PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_3M},
    {9, 0, 3,
        4, PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_4M},
    {9, 0, 3,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2},
    {9, 6, 1,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_HALFM},
    {9, 6, 2,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_HALFM},
    {9, 6, 3,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_HALFM},
    {10, 0, 1,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_AOC},
    {13, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_100GBASE_PSM4_PIGTAIL}
};


static const media_type_map_t media_sfp28_type_tbl [] = {
    {1, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR},
    {10, 0, 1, 1, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_1M},
    {10, 0, 1, 2, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_2M},
    {10, 0, 1, 3, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_3M},
    {10, 0, 4, 1, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_1M},
    {10, 0, 4, 2, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_2M},
    {10, 0, 4, 3, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_3M},
    {10, 11, 4, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_HALFM}
};

static const media_type_map_t media_sfpp_type_tbl [] = {
    {1, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_SR},
    {2, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_LR},
    {3, 4, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_ER},
    {3, 5, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_ZR},
    {4, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CX4},
    {5, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_T},
    {6, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_LRM},
    {7, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_DWDM},
    {9, 1, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_USR},
    {10, 6, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU1M},
    {10, 7, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU3M},
    {10, 8, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU5M},
    {10, 9, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU10M},
    {10, 11, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CUHALFM},
    {10, 12, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU2M},
    {10, 13, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU7M},
    {11, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_SFPPLUS_10GBASE_SR_AOCXXM},
    {12, 9, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_ACU10M},
    {12, 10, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_ACU15M},
    {13, 5, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFPPLUS_10GBASE_ZR_TUNABLE}
};

static const sfp_vpn_to_type_map_t media_sfp_vpn_type_tbl [] = {
    {"FTRJ-1519-7D-CSC", 1550, PLATFORM_MEDIA_TYPE_SFP_ZX},
    {"FTLF1519P1BCL", 1550, PLATFORM_MEDIA_TYPE_SFP_ZX},
    {"FTLF1519P1WCL", 1550, PLATFORM_MEDIA_TYPE_SFP_ZX},
    {"FWDM-1619-7D-47", 1470, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-49", 1490, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-51", 1510, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-53", 1530, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-55", 1550, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-57", 1570, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-59", 1590, PLATFORM_MEDIA_TYPE_SFP_CWDM},
    {"FWDM-1619-7D-61", 1610, PLATFORM_MEDIA_TYPE_SFP_CWDM}
};

static const sdi_to_pas_map_t  media_sfp_gige_type_tbl [] = {
    {0x01, PLATFORM_MEDIA_TYPE_SFP_SX},
    {0x02, PLATFORM_MEDIA_TYPE_SFP_LX},
    {0x04, PLATFORM_MEDIA_TYPE_SFP_CX},
    {0x08, PLATFORM_MEDIA_TYPE_SFP_T},
    {0x10, PLATFORM_MEDIA_TYPE_SFP_LX},
    {0x20, PLATFORM_MEDIA_TYPE_SFP_FX},
    {0x40, PLATFORM_MEDIA_TYPE_SFP_BX10},
    {0x80, PLATFORM_MEDIA_TYPE_SFP_PX}
};

/*
 * dn_pas_media_type_config_get function return's config entry
 * for the specified media type if its present in media type config
 * table otherwise return's NULL.
 */

pas_media_type_config * dn_pas_media_type_config_get (PLATFORM_MEDIA_TYPE_t type)
{
    uint_t            index;
    struct pas_config_media  *cfg = dn_pas_config_media_get();


    for (index = 0; index < cfg->media_count; index++) {
        if (type == cfg->media_type_config[index].media_type) {
            return &cfg->media_type_config[index];
        }
    }
    return NULL;
}

/*
 * Validate media type is supported in this front panel port.
 * LR media is supported in top row only.
 */

bool dn_pas_is_media_type_supported_in_fp (uint_t port,
                                           PLATFORM_MEDIA_TYPE_t type,
                                           bool *disable, bool *lr_mode,
                                           bool *supported)
{
    struct pas_config_media *cfg = dn_pas_config_media_get();
    pas_media_type_config *media_type_cfg = dn_pas_media_type_config_get(type);
    bool  ret = true;

    *disable = false;
    *lr_mode = true;

    if ((cfg != NULL) && (media_type_cfg != NULL)) {

        if (media_type_cfg->supported == false) {
            *supported = false;
            return false;
        }

        if ((cfg->lr_restriction == true)
                && ((port % 2) == 0)) {

            if (media_type_cfg->media_lr == true) {
                *disable = media_type_cfg->media_disable;
                ret = false;
            }

            *lr_mode = false;

        } else {
            *lr_mode = media_type_cfg->media_lr;
        }
    }
    return ret;
}

static PLATFORM_MEDIA_TYPE_t dn_pas_sfp_media_type_find (pas_media_t *res_data)
{
    PLATFORM_MEDIA_TYPE_t optics_type = PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
    uint_t                index;
    uint_t                pas_id, sdi_id;

    for (index = 0; index < ARRAY_SIZE(media_sfp_vpn_type_tbl); index++) {

        if (strncmp(media_sfp_vpn_type_tbl[index].vendor_pn,
                    (char *)res_data->vendor_pn,
                    strlen(media_sfp_vpn_type_tbl[index].vendor_pn)) == 0) {
            break;
        }
    }

    if (index < ARRAY_SIZE(media_sfp_vpn_type_tbl)) {

        optics_type = media_sfp_vpn_type_tbl[index].type;

    } else {

        sdi_id = res_data->transceiver[SFP_GIGE_XCVR_CODE_OFFSET];

        pas_id = dn_pas_media_pas_id_get(media_sfp_gige_type_tbl,
                ARRAY_SIZE(media_sfp_gige_type_tbl), sdi_id);

        if (pas_id != PAS_MEDIA_INVALID_ID) {

            optics_type = pas_id;
            if ((pas_id == PLATFORM_MEDIA_TYPE_SFP_LX)
                    && (res_data->wavelength == 1550)
                    && (res_data->length_sfm_km <= 80)) {

                optics_type = PLATFORM_MEDIA_TYPE_SFP_ZX;
            } else if (pas_id == PLATFORM_MEDIA_TYPE_SFP_BX10) {
                if ((res_data->wavelength == 0x051E) 
                        && (res_data->length_sfm_km == 0xA)) {
                    optics_type = PLATFORM_MEDIA_TYPE_SFP_BX10_UP;
                } else if ((res_data->wavelength == 0x051E)
                        && (res_data->length_sfm_km == 0x28)) {
                    optics_type = PLATFORM_MEDIA_TYPE_SFP_BX40_UP;
                } else if ((res_data->wavelength == 0x05D2)
                        && (res_data->length_sfm_km == 0x50)) {
                    optics_type = PLATFORM_MEDIA_TYPE_SFP_BX80_UP;
                } else if ((res_data->wavelength == 0x05D2)
                        && (res_data->length_sfm_km == 0xA)) {
                    optics_type = PLATFORM_MEDIA_TYPE_SFP_BX10_DOWN;
                } else if ((res_data->wavelength == 0x05D2)
                        && (res_data->length_sfm_km == 0x28)) {
                    optics_type = PLATFORM_MEDIA_TYPE_SFP_BX40_DOWN;
                } else if ((res_data->wavelength == 0x060E)
                        && (res_data->length_sfm_km == 0x50)) {
                    optics_type = PLATFORM_MEDIA_TYPE_SFP_BX80_DOWN;
                } 
            }

        } else if ((res_data->wavelength != 0xFFFF)
                && (res_data->wavelength != 0x0)) {

            if (res_data->wavelength < 1000) {

                optics_type = PLATFORM_MEDIA_TYPE_SFP_SX;

            } else if (res_data->wavelength < 1350) {

                optics_type = PLATFORM_MEDIA_TYPE_SFP_LX;

            }
        } else {

            optics_type = PLATFORM_MEDIA_TYPE_SFP_ZX;
        }
    }

    return optics_type;
}


/*
 * dn_pas_media_type_find finds the media type based on
 * wave length, distance, protocol and the cable length.
 */

static PLATFORM_MEDIA_TYPE_t dn_pas_media_type_find (const media_type_map_t *media_type_tbl,
        const uint_t size, uint_t wave_len, uint_t distance, uint_t protocol, uint_t cable_length) {

    PLATFORM_MEDIA_TYPE_t optics_type = PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
    uint_t                index;

    for (index = 0; index < size; index++) {
        if (wave_len == media_type_tbl[index].wave_len){
            if (media_type_tbl[index].distance == distance ||
                media_type_tbl[index].distance ==  MEDIA_DIST_DONT_CARE) {

                if (media_type_tbl[index].protocol == protocol ||
                    media_type_tbl[index].protocol == MEDIA_PROT_DONT_CARE) {
                    if (media_type_tbl[index].cable_length == cable_length ||
                        media_type_tbl[index].cable_length == MEDIA_LENGTH_DONT_CARE) {
                        optics_type = media_type_tbl[index].optics_type;
                        break;
                    }
                }
            }
        }
    }

    return optics_type;
}

/*
 * dn_pas_media_pas_id_get returns mapped PAS ID for requested
 * ID.
 */

static uint_t dn_pas_media_pas_id_get (const sdi_to_pas_map_t *pmap,
        uint_t count, uint_t id)
{
    uint_t index;

    if ((pmap == NULL) || (count == 0)) {
        return PAS_MEDIA_INVALID_ID;
    }

    for (index = 0; index < count; index++) {
        if (pmap[index].sdi_id == id) {
            return pmap[index].pas_id;
        }
    }

    return PAS_MEDIA_INVALID_ID;
}

/*
 * dn_pas_media_sdi_id_get returns mapped SDI ID for requested
 * ID.
 */

static uint_t dn_pas_media_sdi_id_get (const sdi_to_pas_map_t *pmap,
        uint_t count, uint_t id)
{
    uint_t index;

    if ((pmap == NULL) || (count == 0)) {
        return PAS_MEDIA_INVALID_ID;
    }

    for (index = 0; index < count; index++) {
        if (pmap[index].pas_id == id) {
            return pmap[index].sdi_id;
        }
    }

    return PAS_MEDIA_INVALID_ID;
}

/*
 * dn_pas_product_id_get to validate and get the product id of the media
 */

static uint16_t dn_pas_product_id_get (sdi_media_dell_product_info_t *info)
{
    uint16_t id = PAS_MEDIA_QSFP_INVALID_ID;

    /* check if vendor data contains F10 specific IDs */

    if (info == NULL) return id;

    if (((info->magic_key0 == PAS_PRODUCT_ID_MAGIC0) ||
            (info->magic_key0 == PAS_PRODUCT_ID_QSFP28_MAGIC0)) &&
            (info->magic_key1 == PAS_PRODUCT_ID_MAGIC1))
        /* now read the Qsfp product ID  */
    {
        id = (uint16_t)info->product_id[1];
        id |= (uint16_t)info->product_id[0] << 8;
    }

    return(id);
}

/*
 * dn_pas_product_id_to_optics_type is to derive the optics type based on
 * product id.
 */

static PLATFORM_MEDIA_TYPE_t dn_pas_product_id_to_optics_type (
        PLATFORM_MEDIA_CATEGORY_t category, uint16_t id, uint_t cable_length)
{
    uint16_t wave_len = 0, distance = 0, protocol = 0;
    PLATFORM_MEDIA_TYPE_t optics_type = PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;

    if (id != PAS_MEDIA_QSFP_INVALID_ID) {

        wave_len = (id >> 12) & 0xf;
        distance = (id >> 4) & 0xf;
        protocol = (id) & 0xf ;

        if ((category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS)
                || (category == PLATFORM_MEDIA_CATEGORY_QSFP)) {

            optics_type = dn_pas_media_type_find(media_qsfp_type_tbl,
                    ARRAY_SIZE(media_qsfp_type_tbl), wave_len,
                    distance, protocol, cable_length);

        } else if (category == PLATFORM_MEDIA_CATEGORY_QSFP28) {

            optics_type = dn_pas_media_type_find(media_qsfp28_type_tbl,
                    ARRAY_SIZE(media_qsfp28_type_tbl), wave_len,
                    distance, protocol, cable_length);


        } else if (category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS) {

            optics_type = dn_pas_media_type_find(media_sfpp_type_tbl,
                    ARRAY_SIZE(media_sfpp_type_tbl), wave_len,
                    distance, protocol, cable_length);
        } else if (category == PLATFORM_MEDIA_CATEGORY_SFP28) {

            optics_type = dn_pas_media_type_find(media_sfp28_type_tbl,
                    ARRAY_SIZE(media_sfp28_type_tbl), wave_len,
                    distance, protocol, cable_length);
        }

    }  /* If qsfp is not 0xffff */
    return(optics_type);
}

static PLATFORM_MEDIA_TYPE_t dn_pas_std_optics_type_get (pas_media_t *res_data)
{
    uint8_t transmitter_code;
    sdi_media_transceiver_descr_t *trans_desc =
        (sdi_media_transceiver_descr_t *) res_data->transceiver;

    if (res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28) {
        switch ((res_data->options >> QSFP28_OPTION1_BIT_SHIFT) &
                (QSFP28_OPTION1_BIT_MASK)) {
            case QSFP_100GBASE_AOC:
                return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_AOC);
            case QSFP_100GBASE_SR4:
                return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_SR4);
            case QSFP_100GBASE_LR4:
                return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_LR4);
            case QSFP_100GBASE_CWDM4:
                return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CWDM4);
            case QSFP_100GBASE_PSM4_IR:
                return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_PSM4_IR);
            case QSFP_100GBASE_CR4:
                switch (res_data->length_cable) {
                    case 1:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_1M);
                    case 2:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_2M);
                    case 3:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_3M);
                    case 4:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_4M);
                    case 5:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_5M);
                    default:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4);
                }
            default:
                break;
        }

        transmitter_code = res_data->device_tech >> PAS_QSFP_TRANS_TECH_OFFSET;
        switch (transmitter_code) {
            case QSFP_COPPER_UNEQ:
            case QSFP_COPPER_PASSIVE_EQ:
            case QSFP_COPPER_NEAR_FAR_EQ:
            case QSFP_COPPER_FAR_EQ:
            case QSFP_COPPER_NEAR_EQ:
            case QSFP_COPPER_LINEAR_ACTIVE:
                switch (res_data->length_cable) {
                    case 1:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_1M);
                    case 2:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_2M);
                    case 3:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_3M);
                    case 4:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_4M);
                    case 5:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4_5M);
                    default:
                        return (PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_CR4);
                }
        }
    } else if ((res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS)
            || (res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP)) {
        switch (trans_desc->qsfp_descr.sdi_qsfp_eth_1040g_code) {

            case QSFP_40GBASE_LR4:
                return(PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_LR4);
            case QSFP_40GBASE_SR4:
                return(PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_SR4);
            case QSFP_40GBASE_CR4:
                return(PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4);
            case QSFP_40G_ACTIVE_CABLE:
                return (PLATFORM_MEDIA_TYPE_QSFP_40GBASE_AOC);
            default:
                break;
        }

        if ((res_data->options & QSFP28_OPTION1_BIT_MASK)
                == QSFP_40GBASE_ER4) {
            return (PLATFORM_MEDIA_TYPE_QSFP_40GBASE_ER4);
        }

        transmitter_code = res_data->device_tech >> PAS_QSFP_TRANS_TECH_OFFSET;

        switch (transmitter_code) {
            case QSFP_COPPER_UNEQ:
            case QSFP_COPPER_PASSIVE_EQ:
            case QSFP_COPPER_NEAR_FAR_EQ:
            case QSFP_COPPER_FAR_EQ:
            case QSFP_COPPER_NEAR_EQ:
            case QSFP_COPPER_LINEAR_ACTIVE:
                return (PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4);
            default:
                break;
        }
    } else if (res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS) {
        /* \todo add support for handling sfp plus media type */

        switch (trans_desc->sfp_descr.sdi_sfp_eth_10g_code) {
            case SFP_10GBASE_SR:
                return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_SR;
            case SFP_10GBASE_LR:
                return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_LR;
            case SFP_10GBASE_LRM:
                return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_LRM;
            case SFP_10GBASE_ER:
                return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_ER;
            default:
                break;
        }

        if (trans_desc->sfp_descr.sdi_sfp_plus_cable_technology
                == SFP_PLUS_PASSIVE_CABLE) {
            switch (res_data->length_cable) {
                case 1:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU1M;
                case 2:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU2M;
                case 3:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU3M;
                case 5:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU5M;
                default:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU1M;
            }
        } else if (trans_desc->sfp_descr.sdi_sfp_plus_cable_technology
                == SFP_PLUS_ACTIVE_CABLE) {
            switch (res_data->length_cable) {
                case 7:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU7M;
                case 10:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU10M;
                default:
                    return PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_CU7M;
            }
        }
    }

    return (PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN);
}

/*
 * dn_pas_media_type_get to handle the media type based on
 * sdi_media_dell_product_info_t struct.
 */

PLATFORM_MEDIA_TYPE_t dn_pas_media_type_get (pas_media_t *res_data)
{
    PLATFORM_MEDIA_TYPE_t     op_type = PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
    uint16_t                  id;
    sdi_media_transceiver_descr_t *ptr = NULL;

    /* read programmed product Id */

    id =  dn_pas_product_id_get((sdi_media_dell_product_info_t*)
            res_data->vendor_specific);

    ptr = (sdi_media_transceiver_descr_t *) &(res_data->transceiver);

    if ((id == PAS_MEDIA_QSFP_INVALID_ID)
            && (res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
            && (ptr->sfp_descr.sdi_sfp_eth_10g_code == PAS_SFP_INVALID_GIGE_CODE)
            && (ptr->sfp_descr.sdi_sfp_plus_cable_technology == PAS_SFP_INVALID_GIGE_CODE)) {

        res_data->category = PLATFORM_MEDIA_CATEGORY_SFP;
        return dn_pas_sfp_media_type_find(res_data);
    }

    op_type = dn_pas_product_id_to_optics_type(res_data->category, id, res_data->length_cable);

    if (op_type == PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN) {
        /* Look at the optics serial ID EEPROM for further
         * information about optics type */
        op_type = dn_pas_std_optics_type_get(res_data);
        PAS_NOTICE("Dell non-qualified media adapater");
    }
    return(op_type);
}

/*
 * dn_pas_media_channel_count_get is to get the channel count.
 */

uint_t dn_pas_media_channel_count_get (PLATFORM_MEDIA_CATEGORY_t category)
{

    uint_t count = 0;

    count = dn_pas_media_pas_id_get(channel_count,
            ARRAY_SIZE(channel_count), category);

    return (count == PAS_MEDIA_INVALID_ID)? 0 : count;

}

/*
 * dn_pas_category_get is to derive the media category based on
 * identifier field
 */

PLATFORM_MEDIA_CATEGORY_t dn_pas_category_get (pas_media_t *res_data)
{
    PLATFORM_MEDIA_CATEGORY_t category;

    category = dn_pas_media_pas_id_get(id_to_category,
            ARRAY_SIZE(id_to_category), res_data->identifier);


    if (category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS) {

        if ((res_data->ext_transceiver == 0xB)
                || (res_data->ext_transceiver == 0xC)
                || (res_data->ext_transceiver == 0xD)) {
            category = PLATFORM_MEDIA_CATEGORY_SFP28;
        }
    }

    return category;
}

/*
 * dn_pas_capability_conv is to convert the sdi capability to
 * pas capability type.
 */

BASE_IF_SPEED_t dn_pas_capability_conv (sdi_media_speed_t speed)
{

    return dn_pas_media_pas_id_get(speed_to_capability,
            ARRAY_SIZE(speed_to_capability), speed);
}


/*
 * dn_pas_to_sdi_capability_conv is to convert the PAS capability to
 * SDI speed.
 */

sdi_media_speed_t dn_pas_to_sdi_capability_conv (BASE_IF_SPEED_t speed)
{
    return dn_pas_media_sdi_id_get(speed_to_capability,
            ARRAY_SIZE(speed_to_capability), speed);
}

sdi_media_type_t dn_pas_to_sdi_type_conv (PLATFORM_MEDIA_TYPE_t type)
{
    sdi_media_type_t ctype = SDI_MEDIA_DEFAULT;
    uint_t           ret = PAS_MEDIA_INVALID_ID;

    ret = dn_pas_media_sdi_id_get(media_type_map,
                                  ARRAY_SIZE(media_type_map), type);

    if (ret != PAS_MEDIA_INVALID_ID) {
        ctype = ret;
    }

    return ctype;

}

/*
 * dn_pas_is_capability_10G_plus function return true if the
 * capability is more than 10G.
 */

bool dn_pas_is_capability_10G_plus (BASE_IF_SPEED_t capability)
{
    switch (capability) {
        case BASE_IF_SPEED_10MBPS:
        case BASE_IF_SPEED_100MBPS:
        case BASE_IF_SPEED_1GIGE:
        case BASE_IF_SPEED_10GIGE:
            return false;
        case BASE_IF_SPEED_25GIGE:
        case BASE_IF_SPEED_40GIGE:
        case BASE_IF_SPEED_100GIGE:
        default:
            return true;
    }
}

