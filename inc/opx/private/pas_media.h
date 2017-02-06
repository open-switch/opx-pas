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
 * @file pas_media.h
 * @brief This file contains the structure definitions of all the
 *        physical media handling and function declarations.
 ********************************************************************/


#ifndef __PAS_PHY_MEDIA_H
#define __PAS_PHY_MEDIA_H


#include "std_type_defs.h"
#include "dell-base-pas.h"
#include "private/pas_res_structs.h"
#include "sdi_entity.h"
#include "sdi_media.h"
#include "cps_api_events.h"

enum {
    /* Will removed once get my slot function is ready */
    //TODO need to remove PAS_MEDIA_MY_SLOT and use function call
    PAS_MEDIA_MY_SLOT               =  1,
    PAS_MEDIA_NO_ALARM              =  0,
    PAS_MIN_ID                      =  1,
    PAS_MEDIA_INVALID_PORT_MODULE   =  0,
    PAS_MEDIA_INVALID_PORT          =  0,
    PAS_MEDIA_START_PORT            =  1, /* port starts with 1 */
    PAS_MEDIA_CH_START              =  0, /* Channel numbering starts with 1 */
    PAS_MEDIA_INST_KEY_LEN          =  2, /* Key Length, instance part of media obj */
    PAS_MEDIA_CHANNEL_INST_KEY_LEN  =  3, /* Key Length, instance part of channel obj */
    PAS_MEDIA_MEMBER_START          =  0, /* Pas media member starts from 0 */
    PAS_MEDIA_QSFP_INVALID_ID       =  0xffff,  /* Invalid ID */
    PAS_PRODUCT_ID_MAGIC0           =  0x0F,
    PAS_PRODUCT_ID_MAGIC1           =  0x10,
    PAS_PRODUCT_ID_QSFP28_MAGIC0    =  0xDF, /* QSFP28 Magic 0 */
    QSFP_PROTO_4x10GBASE            =  0x2,
    QSFP_PROTO_4x1GBASET            =  0x2,
    QSFP_PROTO_4x25GBASE            =  0x2,
    QSFP_PROTO_2x50GBASE            =  0x3,
    MEDIA_DIST_DONT_CARE            =  PAS_MEDIA_QSFP_INVALID_ID, /* Distance never be 0xffff */
    MEDIA_PROT_DONT_CARE            =  PAS_MEDIA_QSFP_INVALID_ID, /* Protocol never be 0xffff */
    MEDIA_LENGTH_DONT_CARE          =  PAS_MEDIA_QSFP_INVALID_ID, /* Cable length never be 0xffff */
    PAS_MEDIA_INVALID_ID            =  0xffffffff,
    PAS_MEDIA_MON_ALL_FLAGS         =  0xff,
    PAS_MEDIA_CH_STATUS_FLAGS       =  0xf,
    PAS_QSFP_TRANS_TECH_OFFSET      =  4,
    PAS_SFP_INVALID_GIGE_CODE       =  0x0,
    SFP_GIGE_XCVR_CODE_OFFSET       =  0x3,
    QSFP28_OPTION1_BIT_MASK         =  0xff,
    QSFP28_OPTION1_BIT_SHIFT        =  24
};

/*
 * QSFP device technology list.
 */

typedef enum {
    QSFP_TRANS_TECH_OPTICAL_START,
    QSFP_850nm_VCSEL = QSFP_TRANS_TECH_OPTICAL_START,
    QSFP_1310nm_VCSEL,
    QSFP_1550nm_VCSEL,
    QSFP_1310nm_FP,
    QSFP_1310nm_DFB,
    QSFP_1550nm_DFB,
    QSFP_1310nm_EML,
    QSFP_1550nm_EML,
    QSFP_OTHERS,
    QSFP_1490nm_DFB,
    QSFP_TRANS_TECH_COPPER_START,
    QSFP_COPPER_UNEQ = QSFP_TRANS_TECH_COPPER_START,
    QSFP_COPPER_PASSIVE_EQ,
    QSFP_COPPER_NEAR_FAR_EQ,
    QSFP_COPPER_FAR_EQ,
    QSFP_COPPER_NEAR_EQ,
    QSFP_COPPER_LINEAR_ACTIVE,
    QSFP_TRANS_TECH_UNKNOWN
} pas_qsfp_tx_tech_t;


/*
 * phy_media_tbl_t is to hold sdi handle, resource data address
 * and chaneel info per port and will used for faster access.
 */

typedef struct _phy_media_tbl_t {
    sdi_resource_hdl_t     res_hdl;
    uint_t                 fp_port;
    pas_media_t            *res_data;
    uint_t                 channel_cnt;
    pas_media_channel_t    *channel_data;
} phy_media_tbl_t;

/*
 * phy_media_member_info_t is used to hold each member information of
 * media resouce structure and attribute id and type of data.
 */

typedef struct _phy_media_member_info_t {
    uint_t                       ident;
    uint_t                       offset;
    uint_t                       size;
    cps_api_object_ATTR_TYPE_t   type;
} phy_media_member_info_t;

/*
 * sdi id to PAS id map
 */

typedef struct _sdi_to_pas_map_t {
    uint_t  sdi_id;
    uint_t  pas_id;
} sdi_to_pas_map_t;

/*
 * Media type map table.
 */

typedef struct _media_type_map_t {
    uint_t                wave_len; /* input wave length*/
    uint_t                distance; /* input distance */
    uint_t                protocol; /* input protocol */
    uint_t                cable_length; /* cable length */
    PLATFORM_MEDIA_TYPE_t optics_type; /* media type */
} media_type_map_t;

/*
 * SFP Media type map by vendor PN name
 */

typedef struct _sfp_vpn_to_type_map_t {
    char                    vendor_pn[SDI_MEDIA_MAX_VENDOR_PART_NUMBER_LEN];
    uint_t                  wavelength;
    PLATFORM_MEDIA_TYPE_t   type;
} sfp_vpn_to_type_map_t;


/*
 * SFP Media type map by gige type
 */

typedef struct _sfp_gigetype_to_type_map_t {
    uint8_t               eth_1g_code;
    PLATFORM_MEDIA_TYPE_t type;
} sfp_gigetype_to_type_map_t;


/*
 * Function declarations of phy media.
 */

bool dn_pas_phy_media_init (void);


uint_t dn_pas_media_channel_count_get (PLATFORM_MEDIA_CATEGORY_t category);

PLATFORM_MEDIA_CATEGORY_t dn_pas_category_get (pas_media_t *res_data);

PLATFORM_MEDIA_TYPE_t dn_pas_media_type_get (pas_media_t *res_data);

void dn_pas_phy_media_poll (uint_t port, bool publish);

void dn_pas_phy_media_poll_all (void *arg);

uint_t dn_phy_media_count_get (void);

/*
 * dn_fixed_media_count_get returns fixed media count (non plugable port count).
 */
uint_t dn_fixed_media_count_get(void);
/*
 * dn_fixed_media_count_set updates fixed media count (non plugable port count).
 */
void dn_fixed_media_count_set (uint_t count);
/*
 * Convert front panel port to media index. Returns media index if its
 * a valid port else returns Zero.
 */
uint_t dn_port_to_media_id (uint_t port);
/*
 * Convert media index to front panel port. Return fornt panel port number
 * if media index is valid else returns Zero.
 */
uint_t dn_media_id_to_port (uint_t media_id);

phy_media_tbl_t * dn_phy_media_entry_get(uint_t port);

bool dn_pas_media_obj_all_attr_add (phy_media_member_info_t const *memp,
        uint_t max_count, cps_api_attr_id_t const *list, uint_t count,
        cps_api_object_t obj, void *res_data);

BASE_IF_SPEED_t dn_pas_capability_conv (sdi_media_speed_t speed);

sdi_media_speed_t dn_pas_to_sdi_capability_conv (BASE_IF_SPEED_t speed);

sdi_media_type_t dn_pas_to_sdi_type_conv (PLATFORM_MEDIA_TYPE_t type);

bool dn_pas_is_capability_10G_plus (BASE_IF_SPEED_t capability);

bool dn_pas_is_media_type_supported_in_fp (uint_t port,
        PLATFORM_MEDIA_TYPE_t type, bool *disable, bool *lr_mode,
        bool *supported);

pas_media_type_config * dn_pas_media_type_config_get (PLATFORM_MEDIA_TYPE_t type);

bool dn_pas_media_channel_state_set (uint_t port, uint_t channel,
        bool state);

bool dn_pas_channel_get (cps_api_qualifier_t qualifier,
        uint_t slot, uint_t port, uint_t channel,
        cps_api_get_params_t * param, cps_api_object_t req_obj);

bool dn_pas_port_channel_get (cps_api_qualifier_t qualifier, uint_t slot,
        uint_t port, cps_api_get_params_t *param, cps_api_object_t req_obj);
/*
 * _pas_media_channel_cdr_enable is to handle CDR enable/disable per channel.
 */
bool dn_pas_media_channel_cdr_enable (uint_t port, uint_t channel, bool enable);

/*
 * dn_pas_media_channel_cdr_get is to get the CDR state per channel.
 */
bool dn_pas_media_channel_cdr_get (uint_t port, uint_t channel, bool *enable);

bool dn_pas_media_channel_led_set(uint_t port, uint_t channel,
        BASE_IF_SPEED_t speed);

bool dn_pas_phy_media_is_present (uint_t port);


bool dn_pas_media_high_power_mode_set (uint_t port, bool mode);

cps_api_object_t dn_pas_media_data_publish (uint_t port,
        cps_api_attr_id_t const *list, uint_t count,
        bool handler);

bool dn_pas_media_populate_current_data (BASE_PAS_OBJECTS_t objid,
        cps_api_object_t req_obj, cps_api_object_t rollback_obj,
        uint_t port, uint_t channel);

bool dn_pas_media_is_key_attr(BASE_PAS_OBJECTS_t objid,
        cps_api_attr_id_t attr_id);

bool dn_pas_is_media_obj_empty (cps_api_object_t obj, BASE_PAS_OBJECTS_t objid);

void dn_pas_phy_media_res_free(void);

bool dn_pas_media_lockdown_handle (bool lockdown);

bool dn_pas_media_phy_autoneg_config_set (uint_t port, uint_t channel, bool autoneg,
        cps_api_operation_types_t operation);

bool dn_pas_media_phy_supported_speed_config_set (uint_t port, uint_t channel,
        BASE_IF_SPEED_t *sup_speed, uint_t count, cps_api_operation_types_t operation);

bool dn_pas_media_phy_autoneg_set (uint_t port, uint_t channel);

pas_media_phy_defaults * dn_pas_media_phy_config_entry_get(
                        PLATFORM_MEDIA_TYPE_t type);

bool dn_pas_media_phy_supported_speed_set (uint_t port, uint_t channel);

bool dn_pas_media_phy_interface_mode_set (uint_t port, uint_t channel);

/*
 * dn_pas_media_wavelength_set is to write the target wavelength in eeprom.
 */
bool dn_pas_media_wavelength_set (uint_t port);

/*
 * dn_pas_media_wavelength_config_set is to set the user configured wavelength
 * in local data structure and write into the eeprom if media is present.
 */
bool dn_pas_media_wavelength_config_set (uint_t port, float value,
        cps_api_operation_types_t operation);

#endif  //__PAS_DATA_STORE_H
