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
static PLATFORM_MEDIA_TYPE_t dn_pas_sfp_media_type_find (pas_media_t *res_data);
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
    {0x12, PLATFORM_MEDIA_CATEGORY_CXP28},
    {0x18, PLATFORM_MEDIA_CATEGORY_QSFP_DD}
};

uint_t dn_pas_media_convert_speed_to_num (BASE_IF_SPEED_t speed);

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
    {SDI_MEDIA_SPEED_100G, BASE_IF_SPEED_100GIGE},
    {SDI_MEDIA_SPEED_20G, BASE_IF_SPEED_20GIGE},
    {SDI_MEDIA_SPEED_50G, BASE_IF_SPEED_50GIGE},
    {SDI_MEDIA_SPEED_200G, BASE_IF_SPEED_200GIGE},
    {SDI_MEDIA_SPEED_400G, BASE_IF_SPEED_400GIGE},
    {SDI_MEDIA_SPEED_4GFC, BASE_IF_SPEED_4GFC},
    {SDI_MEDIA_SPEED_8GFC, BASE_IF_SPEED_8GFC},
    {SDI_MEDIA_SPEED_16GFC, BASE_IF_SPEED_16GFC},
    {SDI_MEDIA_SPEED_32GFC, BASE_IF_SPEED_32GFC}
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
    {PLATFORM_MEDIA_CATEGORY_CXP28, 4},
    {PLATFORM_MEDIA_CATEGORY_QSFP_DD, 8},
    {PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28, 2}
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
    {1, 0, 5, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFPPLUS_4X16_16GBASE_FC_SW},
    {1, 0, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFPPLUS_64GBASE_FC_SW4},
    {1, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_SR4},
    {2, 0, 5, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFPPLUS_4X16_16GBASE_FC_LW},
    {2, 0, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFPPLUS_64GBASE_FC_LW4},
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
    {9, 2, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_1M},
    {9, 9, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4_2M},
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
    {9, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4},
    {10, 4, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_AR_QSFP_40GBASE_CR4},
    {10, 5, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP_40GBASE_AOC},
    {10, 8, MEDIA_PROT_DONT_CARE, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP_40GBASE_AOC},
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
    {1, 0, 4, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_4X32_32GBASE_FC_SW},
    {1, 0, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFP28_128GBASE_FC_SW4},
    {1, 1, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_QSFP28_100GBASE_SR4},
    {2, 0, 4, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_4X32_32GBASE_FC_LW},
    {2, 0, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFP28_128GBASE_FC_LW4},
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

static const media_type_map_t media_depop_qsfp28_type_tbl [] = {
    {9, 2, 4, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2_1M},
    {9, 3, 4, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2_3M},
    {9, 9, 4, MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2_2M},
    {9, MEDIA_DIST_DONT_CARE, 4, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2}
};

static const media_type_map_t media_qsfp28_dd_type_tbl [] = {
    {1, 1, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_SR4},
    {4, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CWDM4},
    {5, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_PSM4_IR},
    {8, 1, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_LPBK},
    {9, 0, 1, 1,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_1M},
    {9, 0, 1, 2,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_2M},
    {9, 0, 1, 3,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_3M},
    {9, 0, 1, 5,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_5M},
    {9, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4},
    {9, 0, 2, 1,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_1M},
    {9, 0, 2, 2,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_2M},
    {9, 0, 2, 3,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_3M},
    {9, 0, 2, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4},
    {9, 0, 3, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR1},
    {9, 0, 3, 1,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_1M},
    {9, 0, 3, 2,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_2M},
    {9, 0, 3, 3,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_3M},
    {9, 7, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_HALFM},
    {9, 8, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_1_HALFM},
    {9, 8, 2, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_1_HALFM},
    {9, 8, 3, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_1_HALFM},
    {9, 9, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_2_HALFM},
    {10, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_2SR4_AOC},
    {10, 0, 2, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_SR4_AOC},
    {10, 0, 3, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_2SR4_AOC}
};

static const media_type_map_t media_sfp28_type_tbl [] = {
    {1, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR},
    {1, 0, 5, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_32GBASE_FC_SW},
    {1, 5, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR_NOF},
    {1, 6, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_ESR},
    {2, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_LR},
    {2, 0, 5, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_32GBASE_FC_LW},
    {2, 4, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_LR_LITE},
    {8, 1, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_LPBK},
    {10, 0, 1, 1, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_1M},
    {10, 0, 1, 2, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_2M},
    {10, 0, 1, 3, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_3M},
    {10, 0, 4, 1, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_1M},
    {10, 0, 4, 2, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_2M},
    {10, 0, 4, 3, PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_3M},
    {10, 11, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_HALFM},
    {10, 11, 4, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1_HALFM},
    {11, 0, 1, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR_AOCXXM},
    {11, 0, 4, MEDIA_LENGTH_DONT_CARE,
        PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR_AOCXXM}
};

static const media_type_map_t media_sfpp_type_tbl [] = {
    {0, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_SFPPLUS_8GBASE_FC_SW},
    {1, MEDIA_DIST_DONT_CARE, 6,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_SFPPLUS_16GBASE_FC_SW},
    {1, MEDIA_DIST_DONT_CARE, MEDIA_PROT_DONT_CARE,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_AR_SFPPLUS_10GBASE_SR},
    {2, MEDIA_DIST_DONT_CARE, 6,
        MEDIA_LENGTH_DONT_CARE, PLATFORM_MEDIA_TYPE_SFPPLUS_16GBASE_FC_LW},
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

static const media_type_to_breakout_map_t media_type_to_breakout_tbl[] = {
    {PLATFORM_MEDIA_TYPE_QSFPPLUS_4X16_16GBASE_FC_SW,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_64GFC,
            BASE_IF_SPEED_16GFC},
    {PLATFORM_MEDIA_TYPE_QSFPPLUS_4X16_16GBASE_FC_LW,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_64GFC,
            BASE_IF_SPEED_16GFC},
    {PLATFORM_MEDIA_TYPE_QSFP28_4X32_32GBASE_FC_SW,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_128GFC,
            BASE_IF_SPEED_32GFC},
    {PLATFORM_MEDIA_TYPE_QSFP28_4X32_32GBASE_FC_LW,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_128GFC,
            BASE_IF_SPEED_32GFC},


    {PLATFORM_MEDIA_TYPE_AR_4X1_1000BASE_T,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_4GIGE,
            BASE_IF_SPEED_4GIGE},


    {PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_HALFM,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_40GIGE,
            BASE_IF_SPEED_10GIGE},
    {PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_1M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_40GIGE,
            BASE_IF_SPEED_10GIGE},
    {PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_3M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_40GIGE,
            BASE_IF_SPEED_10GIGE},
    {PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_5M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_40GIGE,
            BASE_IF_SPEED_10GIGE},
    {PLATFORM_MEDIA_TYPE_AR_4X10_10GBASE_CR1_7M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_40GIGE,
            BASE_IF_SPEED_10GIGE},
    {PLATFORM_MEDIA_TYPE_4X_10GBASE_SR_AOCXXM,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_40GIGE,
            BASE_IF_SPEED_10GIGE},


    {PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_HALFM,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_1M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_2M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_3M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1_4M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_25GIGE},


    /* Is this 2x1 or 2x2 ?*/
    /* Free side value is 0x50 which is 2x2 by MSA spec */
    {PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_50GIGE},
    {PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_HALFM,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_50GIGE},
    {PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_1M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_50GIGE},
    {PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_2M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_50GIGE},
    {PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_3M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_50GIGE},
    {PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2_4M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2, BASE_IF_SPEED_100GIGE,
            BASE_IF_SPEED_50GIGE},


    /* Is this 2x1 or 2x2 ?*/
    /* Is octopus 8x2 or two 4x1 ? */
    /* Is media speed 200 or 100 ? */
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_1M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_100GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_2M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_100GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_3M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_100GIGE},

    {PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_100GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_CR4_1_HALFM,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_100GIGE},

    {PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_1M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_2M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_3M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_25GIGE},

    {PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_1_HALFM,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_25GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_CR4_1M,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_25GIGE},

    {PLATFORM_MEDIA_TYPE_QSFP28_DD_2X100GBASE_SR4_AOC,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_100GIGE},
    {PLATFORM_MEDIA_TYPE_QSFP28_DD_8X25GBASE_2SR4_AOC,
        BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2, BASE_IF_SPEED_200GIGE,
            BASE_IF_SPEED_25GIGE}
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
        } else if (category == PLATFORM_MEDIA_CATEGORY_QSFP_DD) {
            optics_type = dn_pas_media_type_find(media_qsfp28_dd_type_tbl,
                    ARRAY_SIZE(media_qsfp28_dd_type_tbl), wave_len,
                    distance, protocol, cable_length);
        } else if (category == PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28) {
            optics_type = dn_pas_media_type_find(media_depop_qsfp28_type_tbl,
                    ARRAY_SIZE(media_depop_qsfp28_type_tbl), wave_len,
                    distance, protocol, cable_length);
        }

    }  /* If qsfp is not 0xffff */
    return(optics_type);
}

/*
 * dn_pas_max_fc_supported_speed is to derive the max supported optics speed
 * based on MSA fields.
 */

static BASE_IF_SPEED_t dn_pas_max_fc_supported_speed(uint8_t sfp_fc_speed)
{
    if(sfp_fc_speed & 0x08) {
        return BASE_IF_SPEED_32GFC;
    } else if(sfp_fc_speed & 0x20) {
        return BASE_IF_SPEED_16GFC;
    } else if(sfp_fc_speed & 0x40) {
        return BASE_IF_SPEED_8GFC;
    } else if(sfp_fc_speed & 0x10) {
        return BASE_IF_SPEED_4GFC;
    }
    return BASE_IF_SPEED_0MBPS;
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
            case QSFP28_BRKOUT_CR_CAS:
            case QSFP28_BRKOUT_CR_CAN:
                if (res_data->free_side_dev_prop == 0x40) {
                    return PLATFORM_MEDIA_TYPE_4X25_25GBASE_CR1;
                } else if (res_data->free_side_dev_prop == 0x50) {
                    return PLATFORM_MEDIA_TYPE_2X50_50GBASE_CR2;
                }
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
        } else if (trans_desc->sfp_descr.sdi_sfp_fc_media == 0x01) {
            switch(dn_pas_max_fc_supported_speed(trans_desc->sfp_descr.sdi_sfp_fc_speed)) {
                case BASE_IF_SPEED_8GFC:
                    res_data->capability = BASE_IF_SPEED_8GFC;
                    return PLATFORM_MEDIA_TYPE_SFPPLUS_8GBASE_FC_LW;
                case BASE_IF_SPEED_16GFC:
                    res_data->capability = BASE_IF_SPEED_16GFC;
                    return PLATFORM_MEDIA_TYPE_SFPPLUS_16GBASE_FC_LW;
                default:
                    return PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
            }
        } else if (trans_desc->sfp_descr.sdi_sfp_fc_media & 0x04) {
            switch(dn_pas_max_fc_supported_speed(trans_desc->sfp_descr.sdi_sfp_fc_speed)) {
                case BASE_IF_SPEED_8GFC:
                    res_data->capability = BASE_IF_SPEED_8GFC;
                    return PLATFORM_MEDIA_TYPE_SFPPLUS_8GBASE_FC_SW;
                case BASE_IF_SPEED_16GFC:
                    res_data->capability = BASE_IF_SPEED_16GFC;
                    return PLATFORM_MEDIA_TYPE_SFPPLUS_16GBASE_FC_SW;
                default:
                    return PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
            }
        }
    } else if (res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD) {
        /* \todo add extra support for handling QSFP-DD media types. */
        uint_t length = res_data->length_cable;
        /* This uses the information in the "options' field when the qualifier string fails*/
        /* Relevant data is upper byte of 32 bits */
        switch((char)( (res_data->options >> 24) & 0xFF) ){
            /* 2xCR4 */
            case 0x0B:
                switch(length){
                    case 0x01:
                        return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_1M;
                    case 0x02:
                        return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_2M;
                    case 0x03:
                        return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_3M;
                    case 0x05:
                        return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4_5M;
                    default:
                        return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CR4;
                }
            break;
            /* 2xSR4 */
            case 0x02:
                    return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_SR4;
            break;

            /* 2xLR4 */
            case 0x03:
                return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_2SR4_AOC;
            break;

            /* 2xCWDM4 */
            case 0x06:
                return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_CWDM4;
            break;

            /* 2xPSM4 */
            case 0x07:
                return PLATFORM_MEDIA_TYPE_QSFP28_DD_200GBASE_PSM4_IR;
            break;

            /* Unkown or not yet supported*/
            default:
                return PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
        }
    } else if (res_data->category == PLATFORM_MEDIA_CATEGORY_SFP28) {
        if (res_data->ext_transceiver == 0x2) {
            if ((res_data->length_om3 == 0x7)
                    && (res_data->length_cable == 0xA)) {
                return PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR;
            } else if ((res_data->length_om3 == 0x3)
                    && (res_data->length_cable == 0x4)) {
                return PLATFORM_MEDIA_TYPE_SFP28_25GBASE_SR_NOF;
            } else if ((res_data->length_om3 == 0x14)
                    && (res_data->length_cable == 0x1E)) {
                return PLATFORM_MEDIA_TYPE_SFP28_25GBASE_ESR;
            }
        } else if (res_data->ext_transceiver == 0x3) {
            if (res_data->length_sfm_km == 0xA) {
                return PLATFORM_MEDIA_TYPE_SFP28_25GBASE_LR;
            } else if (res_data->length_sfm_km == 0x2) {
                return PLATFORM_MEDIA_TYPE_SFP28_25GBASE_LR_LITE;
            }
        } else if ((res_data->ext_transceiver == 0xB)
                || (res_data->ext_transceiver == 0xC)
                || (res_data->ext_transceiver == 0xD)) {
            return PLATFORM_MEDIA_TYPE_SFP28_25GBASE_CR1;
        }
    } else if (res_data->category == PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28) {
        return PLATFORM_MEDIA_TYPE_QSFPPLUS_50GBASE_CR2;
    }

    return (PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN);
}

PLATFORM_MEDIA_TYPE_t dn_pas_media_type_get (pas_media_t *res_data)
{
    PLATFORM_MEDIA_TYPE_t     op_type = PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN;
    sdi_media_transceiver_descr_t *ptr = NULL;

    /* read programmed product Id */

    ptr = (sdi_media_transceiver_descr_t *) &(res_data->transceiver);

    if ((res_data->category == PLATFORM_MEDIA_CATEGORY_SFP_PLUS)
        && (ptr->sfp_descr.sdi_sfp_eth_10g_code == PAS_SFP_INVALID_GIGE_CODE)
        && (ptr->sfp_descr.sdi_sfp_eth_10g_code == PAS_SFP_INVALID_GIGE_CODE)
        && (ptr->sfp_descr.sdi_sfp_eth_1g_code != PAS_SFP_INVALID_GIGE_CODE)
        && (ptr->sfp_descr.sdi_sfp_plus_cable_technology == PAS_SFP_INVALID_GIGE_CODE)) {
        
        /* Must also not be 4,8,16,32GFC.
           Anything higher will never get to this portion of code anyways so no need to check */
        switch (dn_pas_max_fc_supported_speed(ptr->sfp_descr.sdi_sfp_fc_speed)){
        case BASE_IF_SPEED_4GFC:
        case BASE_IF_SPEED_8GFC:
        case BASE_IF_SPEED_16GFC:
        case BASE_IF_SPEED_32GFC:
            break;
            
        default:
            /* Has to be SFP at this point */
            res_data->category = PLATFORM_MEDIA_CATEGORY_SFP;
            res_data->qualified = true;
            return dn_pas_sfp_media_type_find(res_data);
        }
    }

    op_type = dn_pas_product_id_to_optics_type(res_data->category, PAS_MEDIA_QSFP_INVALID_ID, res_data->length_cable);
    
    if(op_type == PLATFORM_MEDIA_TYPE_SFPPLUS_8GBASE_FC_SW) {
        
        if(res_data->wavelength == 1310 && res_data->length_sfm_km == 10) {
            op_type = PLATFORM_MEDIA_TYPE_SFPPLUS_8GBASE_FC_LW;
        }
    }

    if (op_type == PLATFORM_MEDIA_TYPE_AR_POPTICS_UNKNOWN) {
        /* Look at the optics serial ID EEPROM for further
         * information about optics type */
        op_type = dn_pas_std_optics_type_get(res_data);
        PAS_NOTICE("Non-qualified media adapater");
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
                || (res_data->ext_transceiver == 0xD)
                || (res_data->ext_transceiver == 0x1)
                || (res_data->ext_transceiver == 0x2)
                || (res_data->ext_transceiver == 0x3)) {
            category = PLATFORM_MEDIA_CATEGORY_SFP28;
        }
    } else if (category == PLATFORM_MEDIA_CATEGORY_QSFP28) {
        if (res_data->free_side_dev_prop == 0x1C) {
            category = PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28;
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
 * dn_pas_media_capability_get is to get the media capability
 *
 * When non separable media with fc features is detected, eth speed is chosen
 * This will be changed when multi-mode support us enabled 
 */

BASE_IF_SPEED_t dn_pas_media_capability_get (phy_media_tbl_t *mtbl)
{
    pas_media_t *res_data = mtbl->res_data;
    BASE_IF_SPEED_t capability = res_data->capability;
    uint8_t fc_speed = 0;
    sdi_media_transceiver_descr_t *trans_desc =
        (sdi_media_transceiver_descr_t *) res_data->transceiver;

    switch (res_data->category) {
        case PLATFORM_MEDIA_CATEGORY_SFP:
            capability = BASE_IF_SPEED_1GIGE;
            if ((trans_desc->sfp_descr.sdi_sfp_fc_technology
                        != PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->sfp_descr.sdi_sfp_fc_media
                        != PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->sfp_descr.sdi_sfp_fc_speed;
            }
            break;
        case PLATFORM_MEDIA_CATEGORY_SFP_PLUS:
            capability = BASE_IF_SPEED_10GIGE;
            if ((trans_desc->sfp_descr.sdi_sfp_fc_technology
                        != PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->sfp_descr.sdi_sfp_fc_media
                        != PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->sfp_descr.sdi_sfp_fc_speed;
            }
            break;
        case PLATFORM_MEDIA_CATEGORY_SFP28:
            capability = BASE_IF_SPEED_25GIGE;
            if ((trans_desc->sfp_descr.sdi_sfp_fc_technology
                        != PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->sfp_descr.sdi_sfp_fc_media
                        != PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->sfp_descr.sdi_sfp_fc_speed;
            }
            break;
        case PLATFORM_MEDIA_CATEGORY_QSFP:
        case PLATFORM_MEDIA_CATEGORY_QSFP_PLUS:
            capability = BASE_IF_SPEED_40GIGE;
            if ((trans_desc->qsfp_descr.sdi_qsfp_fc_technology
                        == PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->qsfp_descr.sdi_qsfp_fc_media
                        == PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->qsfp_descr.sdi_qsfp_fc_speed;
            }
            break;
        case PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28:
            capability = BASE_IF_SPEED_50GIGE;
            if ((trans_desc->qsfp_descr.sdi_qsfp_fc_technology
                        == PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->qsfp_descr.sdi_qsfp_fc_media
                        == PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->qsfp_descr.sdi_qsfp_fc_speed;
            }
            break;
        case PLATFORM_MEDIA_CATEGORY_QSFP28:
            capability = BASE_IF_SPEED_100GIGE;
            if ((trans_desc->qsfp_descr.sdi_qsfp_fc_technology
                        == PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->qsfp_descr.sdi_qsfp_fc_media
                        == PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->qsfp_descr.sdi_qsfp_fc_speed;
            }
            break;
        case PLATFORM_MEDIA_CATEGORY_QSFP_DD:
            capability = BASE_IF_SPEED_200GIGE;
            if ((trans_desc->qsfp_descr.sdi_qsfp_fc_technology
                        == PAS_SFP_INVALID_GIGE_CODE)
                    && (trans_desc->qsfp_descr.sdi_qsfp_fc_media
                        == PAS_SFP_INVALID_GIGE_CODE)) {
                if (!dn_pas_media_is_connector_separable(mtbl)){
                    PAS_TRACE("Dual mode media on port %u. Will use ethernet",
                    mtbl->fp_port);
                    break;
                }
                fc_speed = trans_desc->qsfp_descr.sdi_qsfp_fc_speed;
            }
            break;
        default:
            break;
    }

    if (fc_speed) {
        capability = dn_pas_max_fc_supported_speed(fc_speed);
    }
    return capability;
}

/*
 * dn_pas_is_media_unsupported is to identify media needs to be supported or not
 * with the option of logging why the media is not supported
 */

bool dn_pas_is_media_unsupported (phy_media_tbl_t *mtbl, bool log_msg)
{
    /* If the media is separable (optics) and speed > 10G *
       Need to convert speeds to numbers so as to perform math/comparison operations */
    bool result = false;
    bool separable = false;
    uint speed_in_num = 0;
    uint reference_speed_in_num
               = dn_pas_media_convert_speed_to_num(BASE_IF_SPEED_10GIGE);

    separable = dn_pas_media_is_connector_separable(mtbl);
    speed_in_num = dn_pas_media_convert_speed_to_num(mtbl->res_data->capability);

    result = (separable) && (speed_in_num > reference_speed_in_num);
    
    return result;
}

BASE_IF_PHY_MODE_TYPE_t dn_pas_media_get_phy_mode_from_speed (BASE_IF_SPEED_t speed)
{
    switch (speed) {
        case BASE_IF_SPEED_1GFC:
        case BASE_IF_SPEED_2GFC:
        case BASE_IF_SPEED_4GFC:
        case BASE_IF_SPEED_8GFC:
        case BASE_IF_SPEED_16GFC:
        case BASE_IF_SPEED_32GFC:
        case BASE_IF_SPEED_64GFC:
        case BASE_IF_SPEED_128GFC:
           return BASE_IF_PHY_MODE_TYPE_FC;
        default:
           return BASE_IF_PHY_MODE_TYPE_ETHERNET;
    }
}

BASE_IF_SPEED_t dn_pas_media_convert_num_to_speed (
                    uint_t num, BASE_IF_PHY_MODE_TYPE_t phy_mode)
{
    switch (num) {
        case 0:
           return BASE_IF_SPEED_0MBPS;
        case 1:
           return (phy_mode == BASE_IF_PHY_MODE_TYPE_ETHERNET)
                   ? BASE_IF_SPEED_1GIGE
                   : BASE_IF_SPEED_1GFC;
        case 2:
           return BASE_IF_SPEED_2GFC;
        case 4:
           return (phy_mode == BASE_IF_PHY_MODE_TYPE_ETHERNET)
                   ? BASE_IF_SPEED_4GIGE
                   : BASE_IF_SPEED_4GFC;
        case 8:
           return BASE_IF_SPEED_8GFC;
        case 10:
           return BASE_IF_SPEED_10GIGE;
        case 16:
           return BASE_IF_SPEED_16GFC;
        case 20:
           return BASE_IF_SPEED_20GIGE;
        case 25:
           return BASE_IF_SPEED_25GIGE;
        case 32:
           return BASE_IF_SPEED_32GFC;
        case 40:
           return BASE_IF_SPEED_40GIGE;
        case 50:
           return BASE_IF_SPEED_50GIGE;
        case 64:
           return BASE_IF_SPEED_64GFC;
        case 100:
           return BASE_IF_SPEED_100GIGE;
        case 128:
           return BASE_IF_SPEED_128GFC;
        case 200:
           return BASE_IF_SPEED_200GIGE;
        case 400:
           return BASE_IF_SPEED_400GIGE;
        default:
           PAS_ERR("Invalid number to speed conversion. Willl abort");
           return BASE_IF_SPEED_0MBPS;
    }
}


uint_t dn_pas_media_convert_speed_to_num (BASE_IF_SPEED_t speed)
{
    switch (speed) {
        case BASE_IF_SPEED_0MBPS:
           return 0;
        case BASE_IF_SPEED_1GIGE:
        case BASE_IF_SPEED_1GFC:
           return 1;
        case BASE_IF_SPEED_2GFC:
           return 2;
        case BASE_IF_SPEED_4GFC:
        case BASE_IF_SPEED_4GIGE:
           return 4;
        case BASE_IF_SPEED_8GFC:
           return 8;
        case BASE_IF_SPEED_10GIGE:
           return 10;
        case BASE_IF_SPEED_16GFC:
           return 16;
        case BASE_IF_SPEED_20GIGE:
           return 20;
        case BASE_IF_SPEED_25GIGE:
           return 25;
        case BASE_IF_SPEED_32GFC:
           return 32;
        case BASE_IF_SPEED_40GIGE:
           return 40;
        case BASE_IF_SPEED_50GIGE:
           return 50;
        case BASE_IF_SPEED_64GFC:
           return 64;
        case BASE_IF_SPEED_100GIGE:
           return 100;
        case BASE_IF_SPEED_128GFC:
           return 100;
        case BASE_IF_SPEED_200GIGE:
           return 200;
        case BASE_IF_SPEED_400GIGE:
           return 400;
        default:
           PAS_ERR("Invalid speed to number conversion. Willl abort");
           return 0;
    }
}

bool dn_pas_media_is_physical_breakout_valid (pas_media_t* res_data)
{ 
    if ((res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_PLUS)
       || (res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP28)
     /*|| (res_data->category == PLATFORM_MEDIA_CATEGORY_QSFP_DD) //Not yet supported  */
       || (res_data->category == PLATFORM_MEDIA_CATEGORY_DEPOP_QSFP28)) {
        return true;
    }
    return false;
}

BASE_IF_SPEED_t dn_pas_media_resolve_breakout_speed(BASE_IF_SPEED_t media_speed, 
                    BASE_CMN_BREAKOUT_TYPE_t brk)
{
    uint_t divisor = 1;
    BASE_IF_PHY_MODE_TYPE_t phy_mode = dn_pas_media_get_phy_mode_from_speed(
                                           media_speed);

    switch (brk) {
        case BASE_CMN_BREAKOUT_TYPE_BREAKOUT_8X2:
            divisor = 8;
            break;
        case BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X2:
        case BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1:
            divisor = 4;
            break;
        case BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2:
        case BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1:
            divisor = 2;
            break;
        case BASE_CMN_BREAKOUT_TYPE_BREAKOUT_1X1:
        default:
            divisor = 1;
    }
    return dn_pas_media_convert_num_to_speed(
               dn_pas_media_convert_speed_to_num(media_speed) / divisor, phy_mode);
}

media_type_to_breakout_map_t* dn_pas_media_find_breakout_info(
                                   PLATFORM_MEDIA_TYPE_t type,
                                   const media_type_to_breakout_map_t* tbl,
                                   size_t len)
{
    size_t count = 0;
    while (count < len) {
        if (tbl[count].type == type){
            return (media_type_to_breakout_map_t*) &tbl[count];
        }
        count++;
    }
    return NULL;
}

/* This uses a media_speed and media table to assign other breakout info*/

BASE_CMN_BREAKOUT_TYPE_t dn_pas_media_get_default_breakout_info(
                                       media_capability_t* cap,
                                       phy_media_tbl_t *mtbl)
{
    BASE_CMN_BREAKOUT_TYPE_t brk = BASE_CMN_BREAKOUT_TYPE_BREAKOUT_UNKNOWN;
    BASE_IF_SPEED_t media_speed = BASE_IF_SPEED_0MBPS;
    BASE_IF_SPEED_t brk_speed = BASE_IF_SPEED_0MBPS;
    media_type_to_breakout_map_t* brk_map = NULL;

    char * port_str = mtbl->port_str;

    if (mtbl == NULL) {
        PAS_ERR("Null value for media table entry, ports: %s", port_str);
        STD_ASSERT(mtbl != NULL);
    }

    /* Non pluggable ports currently do not have physical breakout*/
    if (!dn_pas_is_port_pluggable(mtbl->fp_port)) {
        return BASE_CMN_BREAKOUT_TYPE_NO_BREAKOUT;
    }

    if (mtbl->res_data == NULL) {
        PAS_ERR("Null value for media table eeprom data, ports: %s", port_str);
        STD_ASSERT(mtbl->res_data != NULL);
    }

    brk_speed = media_speed = mtbl->res_data->capability;

    if (!mtbl->res_data->present){
        return BASE_CMN_BREAKOUT_TYPE_NO_BREAKOUT;
    }

    if (!dn_pas_media_is_physical_breakout_valid(mtbl->res_data)) {
        /* For cases where a breakout is not physically possible */
        /* Hence channel count = 1 */

        if (false == dn_pas_media_set_capability_values(
                          cap,
                          media_speed,
                          BASE_CMN_BREAKOUT_TYPE_NO_BREAKOUT,
                          brk_speed,
             dn_pas_media_get_phy_mode_from_speed(media_speed)))
        {
            PAS_ERR("Could not set capability, port(s): %s" , mtbl->port_str); 
        }
        return cap->breakout_mode;
    }

    if (media_speed == BASE_IF_SPEED_0MBPS) {
        PAS_ERR("Cannot obtain media speed on ports: %s", port_str);
        return brk;
    }

    /* try to find info by look up table first */
    brk_map = dn_pas_media_find_breakout_info(mtbl->res_data->type,
                  media_type_to_breakout_tbl,
                  ARRAY_SIZE(media_type_to_breakout_tbl));
    if (brk_map != NULL){
        if (false == dn_pas_media_set_capability_values(cap,
                                           brk_map->media_speed,
                                           brk_map->breakout_mode,
                                           brk_map->breakout_speed,
                  dn_pas_media_get_phy_mode_from_speed(brk_map->media_speed))
        ){
            PAS_ERR("Capability set failed, ports(s): %s", mtbl->port_str);
        }
        return brk_map->breakout_mode;
    } 

    switch (mtbl->res_data->free_side_dev_prop) {
        case 0x10:
            brk = BASE_CMN_BREAKOUT_TYPE_BREAKOUT_1X1;
            break;
        case 0x40:
            brk = BASE_CMN_BREAKOUT_TYPE_BREAKOUT_4X1;
            break;
        case 0x50:
            brk = BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X2;
            break;
        case 0x60:
            brk = BASE_CMN_BREAKOUT_TYPE_BREAKOUT_2X1;
            break;

        /* Yet to be properly understood. Need to revisit */
        case 0x20:
        case 0x30:
        /* States which we currently do not support; or if the EEPROM is not programmed*/
        default:
            brk = BASE_CMN_BREAKOUT_TYPE_BREAKOUT_UNKNOWN;
    }
    /* Divides media_speed by breakout to get the breakout speed */

    brk_speed = dn_pas_media_resolve_breakout_speed(media_speed, brk);

    if (brk_speed == BASE_IF_SPEED_0MBPS) {
        PAS_ERR("Cannot obtain breakout accurate speed on ports: %s", port_str);
    }


    dn_pas_media_set_capability_values(cap,
                                       media_speed,
                                       brk,
                                       brk_speed,
              dn_pas_media_get_phy_mode_from_speed(media_speed));

   return brk;
}

bool dn_pas_media_set_capability_values(media_capability_t* cap,
                                        BASE_IF_SPEED_t          media_speed,
                                        BASE_CMN_BREAKOUT_TYPE_t breakout_mode,
                                        BASE_IF_SPEED_t          breakout_speed,
                                        BASE_IF_PHY_MODE_TYPE_t  phy_mode)
{
    if (cap == NULL) {
        PAS_ERR("Attempt to set null capability struct");
        return false;
    }
    cap->media_speed = media_speed;
    cap->breakout_mode = breakout_mode;
    cap->breakout_speed = breakout_speed;
    cap->phy_mode = phy_mode;
    return true;
}

bool dn_pas_media_validate_capability(media_capability_t cap)
{
    return (cap.media_speed != BASE_IF_SPEED_0MBPS) &
           (cap.phy_mode != 0);
}

/* Populates the capability struct array and returns number of capabilities*/

uint_t dn_pas_media_construct_media_capabilities(phy_media_tbl_t *mtbl)
{
    media_capability_t default_capability = {0};
    STD_ASSERT(mtbl != NULL);

    /* Function must have been visited earlier. Reset values */
    if (mtbl->res_data->media_capability_count != 0) {
        mtbl->res_data->media_capability_count = 0;
        mtbl->res_data->default_media_capability_index = 0;
    }

    /* For default: set known values  */
    if (dn_pas_media_set_capability_values(
             &default_capability,
             mtbl->res_data->capability,
             PAS_MEDIA_DEFAULT_BREAKOUT_MODE,
             BASE_IF_SPEED_0MBPS,
             dn_pas_media_get_phy_mode_from_speed(mtbl->res_data->capability))
       ){
        /* Resolve breakout */
        dn_pas_media_get_default_breakout_info(&default_capability, mtbl);
        /* Set default index and increment count */
        mtbl->res_data->default_media_capability_index =
             mtbl->res_data->media_capability_count++;
        mtbl->res_data->media_capabilities[
             mtbl->res_data->default_media_capability_index]
                  = default_capability;
    }else{
        PAS_ERR("Could not set capability, port(s): %s" , mtbl->port_str);
    }

    if (dn_pas_media_validate_capability(default_capability) == false){
        PAS_ERR("Capability construction error");
    }

    /* To do other capabilities  in future */

    return mtbl->res_data->media_capability_count;
}

bool dn_pas_media_is_connector_separable(phy_media_tbl_t *mtbl)
{
    sdi_media_transceiver_descr_t *trans_desc =
        (sdi_media_transceiver_descr_t *) mtbl->res_data->transceiver;

    /* Pluggable ports are not considered separable 
       Check for 200/100G AOC may be programmed as using optical connector, which is usually considered separable
       Check for 40G AOC for same reason
       Check for non separable connector
       Check for DAC connector */
    return (dn_pas_is_port_pluggable(mtbl->fp_port)
        && (((mtbl->res_data->options >> QSFP28_OPTION1_BIT_SHIFT)
           &(QSFP28_OPTION1_BIT_MASK)) != QSFP_100GBASE_AOC)
        && (trans_desc->qsfp_descr.sdi_qsfp_eth_1040g_code
           != QSFP_40G_ACTIVE_CABLE )
        && (mtbl->res_data->connector != SDI_MEDIA_CONNECTOR_NON_SEPARABLE)
        && (mtbl->res_data->connector != SDI_MEDIA_CONNECTOR_COPPER_PIGTAIL));
}

sdi_media_fw_rev_t pas_media_fw_rev_get (pas_media_t *res_data)
{
    sdi_media_fw_rev_t rev = SDI_MEDIA_FW_REV0;

    if (res_data) {
        uint8_t val = res_data->vendor_specific[5] & 0x0f;
        switch(val) {
            case 0:
                rev = SDI_MEDIA_FW_REV0;
                break;
            case 1:
                rev = SDI_MEDIA_FW_REV1;
                break;
            default:
                rev = SDI_MEDIA_FW_REV1;
                break;
        }
    }
    return rev;
}

