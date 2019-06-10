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


#ifndef  __PAS_MEDIA_VENDOR_H_
#define __PAS_MEDIA_VENDOR_H_

#include "std_type_defs.h"
#include "dell-base-pas.h"
#include "sdi_entity.h"
#include "sdi_media.h"

#ifdef __cplusplus
extern "C" {
#endif



#define MEDIA_MGR_INHERIT_USE_MSA       0xffff
#define MEDIA_MGR_NO_ENUM_OVERRIDE      0xffff
#define PAS_MEDIA_QSFP_INVALID_ID       0xffff
#define MEDIA_DIST_DONT_CARE            PAS_MEDIA_QSFP_INVALID_ID /* Distance never be 0xffff */
#define MEDIA_PROT_DONT_CARE            PAS_MEDIA_QSFP_INVALID_ID /* Protocol never be 0xffff */
#define MEDIA_LENGTH_DONT_CARE          PAS_MEDIA_QSFP_INVALID_ID /* Cable length never be 0xffff */

typedef struct {
    PLATFORM_MEDIA_INTERFACE_t            media_interface;
    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t  media_interface_qualifier;
    uint_t                                media_interface_lane_count;
    uint_t                                media_interface_prefix;
    int                                   cable_length_cm;
    int                                   ext_spec_code_25g_dac;

    /* default capability */
    BASE_IF_SPEED_t              media_speed;
    BASE_CMN_BREAKOUT_TYPE_t     breakout_mode;
    BASE_IF_SPEED_t              breakout_speed;
    BASE_IF_PHY_MODE_TYPE_t      phy_mode;

    /* This enum is used for lagacy reasons. It is provided to upper applications that still rely on an enum for media info */
    int                                   enum_override;
} dn_pas_media_vendor_basic_media_info_t;


/*  Prototypes for functions to be supplied by vendor-specific media handling plug-in
 */

bool dn_pas_media_vendor_get_info_from_proprietary_type (PLATFORM_MEDIA_TYPE_t type,
                                              dn_pas_media_vendor_basic_media_info_t *prop_info,
                                              bool                         *is_fake_enum,
                                              int                          *prop_len);

PLATFORM_MEDIA_TYPE_t dn_pas_media_vendor_get_media_type(sdi_resource_hdl_t resource_hdl, bool* qualified);

/* Check if media is qualified */
bool dn_pas_media_vendor_is_qualified(sdi_resource_hdl_t res_hdl, bool *qualified);

/* Read vendor-specific information from media adapter */
t_std_error dn_pas_media_vendor_product_info_get(sdi_resource_hdl_t res_hdl, uint8_t *buf);


#ifdef __cplusplus
}
#endif
#endif
