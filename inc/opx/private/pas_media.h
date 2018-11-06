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
#include "private/pas_config.h"


#ifdef __cplusplus
extern "C" {
#endif


#define PAS_MEDIA_NO_QSA_STR             "\0"
#define PAS_MEDIA_UNKNOWN_MEDIA          "UNKNOWN MEDIA"
#define PAS_MEDIA_UNKNOWN_MEDIA_CATEGORY "UNKNOWN"
#define MAX_MEDIA_DISPLAY_STRING_LEN     70    /* MAx length of media disp string */
#define MAX_MEDIA_NAME_LEN               MAX_MEDIA_DISPLAY_STRING_LEN

/* prefix is a non standard term we're using to handle double density media. Might change term to "density" */
#define PAS_MEDIA_INTERFACE_PREFIX_SINGLE_DENSITY       1
#define PAS_MEDIA_INTERFACE_PREFIX_NORMAL               PAS_MEDIA_INTERFACE_PREFIX_SINGLE_DENSITY
#define PAS_MEDIA_INTERFACE_PREFIX_DOUBLE_DENSITY       2


/* Number of transmission lanes/channel */
#define PAS_MEDIA_INTERFACE_LANE_COUNT_SINGLE  1
#define PAS_MEDIA_INTERFACE_LANE_COUNT_DOUBLE  2
#define PAS_MEDIA_INTERFACE_LANE_COUNT_QUAD    4
#define MEDIA_INTERFACE_LANE_COUNT_DEFAULT     PAS_MEDIA_INTERFACE_LANE_COUNT_SINGLE   /* Default transceiver lane count */

/* Will start using "transceiver type" since "category' is ambiguous */
typedef PLATFORM_MEDIA_CATEGORY_t pas_media_transceiver_type;
typedef uint_t PLATFORM_MEDIA_INTERFACE_PREFIX_t;

/* This is a set of basic attributes needed to qualify a connected device. */
typedef struct dn_pas_basic_media_info
{
    pas_media_transceiver_type            transceiver_type;
    PLATFORM_MEDIA_CONNECTOR_TYPE_t       connector_type;
    PLATFORM_MEDIA_CABLE_TYPE_t           cable_type;
    PLATFORM_MEDIA_INTERFACE_t            media_interface;

    PLATFORM_MEDIA_INTERFACE_QUALIFIER_t  media_interface_qualifier;
    uint_t                                media_interface_lane_count;
    PLATFORM_MEDIA_INTERFACE_PREFIX_t     media_interface_prefix;
    uint_t                                cable_length_cm;
    bool                                  connector_separable;
    char                                  display_string[MAX_MEDIA_DISPLAY_STRING_LEN];
    char                                  media_name[MAX_MEDIA_NAME_LEN];
    media_capability_t                    capability_list[1];
    PLATFORM_EXT_SPEC_COMPLIANCE_CODE_t   ext_spec_code_25g_dac;
    BASE_IF_SUPPORTED_AUTONEG_t           default_autoneg;
    BASE_CMN_FEC_TYPE_t                   default_fec;
    char                                  transceiver_type_string[MAX_MEDIA_DISPLAY_STRING_LEN];
    PLATFORM_QSA_ADAPTER_t                qsa_adapter_type;

    /* This flag is used to note when QSA (which is optimized for SFP/SFP+) is used with SFP28. This is because SFP28 expects QSA28 and not QSA */
    bool                                  qsa28_expected;
}dn_pas_basic_media_info_t;

enum {
    /* Will removed once get my slot function is ready */
    //TODO need to remove PAS_MEDIA_MY_SLOT and use function call
    PAS_MEDIA_MY_SLOT               =  1,
    PAS_MEDIA_NO_ALARM              =  0,
    PAS_MIN_ID                      =  1,
    PAS_MEDIA_INVALID_PORT_MODULE   =  0,
    PAS_MEDIA_INVALID_PORT          =  0,
    PAS_MEDIA_DEFAULT_BREAKOUT_MODE =  BASE_CMN_BREAKOUT_TYPE_BREAKOUT_UNKNOWN,
    PAS_MEDIA_START_PORT            =  1, /* port starts with 1 */
    PAS_MEDIA_CH_START              =  0, /* Channel numbering starts with 1 */
    PAS_MEDIA_INST_KEY_LEN          =  2, /* Key Length, instance part of media obj */
    PAS_MEDIA_CHANNEL_INST_KEY_LEN  =  3, /* Key Length, instance part of channel obj */
    PAS_MEDIA_MEMBER_START          =  0, /* Pas media member starts from 0 */
    PAS_PRODUCT_ID_MAGIC0           =  0x0F,
    PAS_PRODUCT_ID_MAGIC1           =  0x10,
    PAS_PRODUCT_ID_QSFP28_MAGIC0    =  0xDF, /* QSFP28 Magic 0 */
    PAS_MEDIA_INVALID_ID            =  0xffffffff,
    PAS_MEDIA_MON_ALL_FLAGS         =  0xff,
    PAS_MEDIA_CH_STATUS_FLAGS       =  0xf,
    PAS_QSFP_TRANS_TECH_OFFSET      =  4,
    PAS_SFP_INVALID_GIGE_CODE       =  0x0,
    SFP_GIGE_XCVR_CODE_OFFSET       =  0x3,
    QSFP28_OPTION1_BIT_MASK         =  0xff,
    QSFP28_OPTION1_BIT_SHIFT        =  24
};

/* MSA IDs used to identify media types */
enum pas_media_discovery_ids {
    PAS_MEDIA_QSFP28_DD_ID_2SR4     = 0x02,
    PAS_MEDIA_QSFP28_DD_ID_2SR4_AOC = 0x03,
    PAS_MEDIA_QSFP28_DD_ID_2CWDM4   = 0x06,
    PAS_MEDIA_QSFP28_DD_ID_2PSM4_IR = 0x07,
    PAS_MEDIA_QSFP28_DD_ID_2CR4     = 0x08,

    PAS_MEDIA_QSFP28_ID_SR4_AOC     = QSFP_100GBASE_AOC,
    PAS_MEDIA_QSFP28_ID2_SR4_AOC    = 0x18,
    PAS_MEDIA_QSFP28_ID_SR4         = QSFP_100GBASE_SR4,
    PAS_MEDIA_QSFP28_ID_ER4         = 0x04,
    PAS_MEDIA_QSFP28_ID_LR4         = QSFP_100GBASE_LR4,
    PAS_MEDIA_QSFP28_ID_CWDM4       = QSFP_100GBASE_CWDM4,
    PAS_MEDIA_QSFP28_ID_PSM4_IR     = QSFP_100GBASE_PSM4_IR,
    PAS_MEDIA_QSFP28_ID_CR4         = 0x19,
    PAS_MEDIA_QSFP28_ID_SWDM4       = 0x20,
    PAS_MEDIA_QSFP28_ID_BIDI        = 0x21,
    PAS_MEDIA_QSFP28_ID_DWDM2       = 0x1A,
    PAS_MEDIA_QSFP28_ID_CR4_CA_L    = QSFP_100GBASE_CR4,
    PAS_MEDIA_QSFP28_ID_CR4_CA_S    = QSFP28_BRKOUT_CR_CAS,
    PAS_MEDIA_QSFP28_ID_CR4_CA_N    = QSFP28_BRKOUT_CR_CAN,

    PAS_MEDIA_BX_UP_WAVELENGTH_ID   = 0x04F6,
    PAS_MEDIA_BX_DOWN_WAVELENGTH_ID = 0x0532,

    PAS_MEDIA_SFP_PLUS_ID_FC_LW     = 0x01,
    PAS_MEDIA_SFP_PLUS_ID_FC_SW     = 0x04,

    PAS_MEDIA_SFP28_ID_AOC          = 0x01,
    PAS_MEDIA_SFP28_ID2_AOC         = 0x18,
    PAS_MEDIA_SFP28_ID_SR           = 0x02,
    PAS_MEDIA_SFP28_ID_LR           = 0x03,
    PAS_MEDIA_SFP28_ID_CR_CA_L      = QSFP_100GBASE_CR4,
    PAS_MEDIA_SFP28_ID_CR_CA_S      = QSFP28_BRKOUT_CR_CAS,
    PAS_MEDIA_SFP28_ID_CR_CA_N      = QSFP28_BRKOUT_CR_CAN
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
    BASE_IF_SPEED_t        max_port_speed;
    char                   port_str[PAS_MEDIA_PORT_STR_BUF_LEN];
    uint_t                 sub_port_ids[PAS_MEDIA_MAX_PORT_DENSITY]; /* actual list size is port_density*/
    uint_t                 port_density;
    pas_media_t            *res_data;
    uint_t                 channel_cnt;
    pas_media_channel_t    *channel_data;
    dn_pas_basic_media_info_t media_info;
    uint_t                 poll_cycles_to_skip;
    uint_t                 mod_holding_so_far;
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
 * Get breakout info from media type
 */
typedef struct _media_type_to_breakout_map_t {
    PLATFORM_MEDIA_TYPE_t    type;
    BASE_CMN_BREAKOUT_TYPE_t breakout_mode;
    BASE_IF_SPEED_t          media_speed;
    BASE_IF_SPEED_t          breakout_speed;
} media_type_to_breakout_map_t;

/* Callback function type for getting media info from transceiver types*/
typedef bool (*pas_media_disc_cb_t)(phy_media_tbl_t *, dn_pas_basic_media_info_t*);

/* Function which resolves appropriate callback from map */
pas_media_disc_cb_t pas_media_get_disc_cb_from_trans_type (uint_t trans_type);

/* Functions which resolve appropriate sfp info from map */
bool pas_media_get_sfp_info_from_part_no (char* part_no, uint_t* wavelength, PLATFORM_MEDIA_INTERFACE_t* media_if);
PLATFORM_MEDIA_INTERFACE_t pas_media_get_sfp_media_if_from_id (uint_t id);

/* Funcitons to get connector, cable and string info from map, based on media interface and qualifier */
PLATFORM_MEDIA_CONNECTOR_TYPE_t pas_media_get_media_interface_connector_type_expected (PLATFORM_MEDIA_INTERFACE_t media_if);
PLATFORM_MEDIA_CABLE_TYPE_t pas_media_get_media_interface_cable_type_expected (PLATFORM_MEDIA_INTERFACE_t media_if);
const char* pas_media_get_media_interface_disp_string (PLATFORM_MEDIA_INTERFACE_t media_if);

PLATFORM_MEDIA_CONNECTOR_TYPE_t pas_media_get_media_interface_qualifier_connector_type_expected (PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_if_qual);
PLATFORM_MEDIA_CABLE_TYPE_t pas_media_get_media_interface_qualifier_cable_type_expected (PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_if_qual);
const char* pas_media_get_media_interface_qualifier_disp_string (PLATFORM_MEDIA_INTERFACE_QUALIFIER_t media_if_qual);

/* Translation between SDI enum and platform enum*/
PLATFORM_MEDIA_CONNECTOR_TYPE_t pas_media_get_media_connector_enum (sdi_media_connector_t conn);

/* Get if connector is separable from connector type */
bool pas_media_is_media_connector_separable (PLATFORM_MEDIA_CONNECTOR_TYPE_t conn, bool *result);

/* Functions to get trans type info  */
const char* pas_media_get_transceiver_type_display_string (pas_media_transceiver_type trans_type);
uint_t pas_media_get_transceiver_type_channel_count (pas_media_transceiver_type trans_type);
bool pas_media_get_transceiver_type_is_breakout_supported (pas_media_transceiver_type trans_type);

/* Get far and near breakout values from breakout */
uint_t pas_media_map_get_breakout_near_end_val (BASE_CMN_BREAKOUT_TYPE_t brk);
uint_t pas_media_map_get_breakout_far_end_val (BASE_CMN_BREAKOUT_TYPE_t brk);

/* Get speed as integer in mbps */
uint_t pas_media_map_get_speed_as_uint_mbps (BASE_IF_SPEED_t speed);

/* Get phy mode form speed */
BASE_IF_PHY_MODE_TYPE_t pas_media_map_get_phy_mode_from_speed (BASE_IF_SPEED_t speed);

/* QSA str for display*/
const char* pas_media_get_qsa_string_from_enum (PLATFORM_QSA_ADAPTER_t qsa_type);

/* This assigns the PLATFORM_MEDIA_TYPE_t enum to new media, which previously do not have a type enum derivation from MSA */
PLATFORM_MEDIA_TYPE_t  pas_media_get_enum_from_new_media_name (char* name);

/* This function is used to override media names */
const char* pas_media_get_media_name_override_from_derived_name (char* name);

/* Functions for deriving media info from transceiver */
bool dn_pas_std_media_get_basic_properties_sfp(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_qsfp_plus(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_qsfp28_dd(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_qsfp28_depop(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_sfp_plus(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_sfp28(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_qsfp28(phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);
bool dn_pas_std_media_get_basic_properties_fixed_port (phy_media_tbl_t *mtbl, dn_pas_basic_media_info_t* media_info);

/* This derives the media properties including calling the discovery function to the trans type*/
bool pas_media_get_media_properties(phy_media_tbl_t *mtbl);

/* Use the fc speed code to the derive maximum supported fc speed */
BASE_IF_SPEED_t dn_pas_max_fc_supported_speed(uint8_t sfp_fc_speed);

/* Get the default brekaout mode from a phy media table */
BASE_CMN_BREAKOUT_TYPE_t dn_pas_media_get_default_breakout_info( media_capability_t* cap, phy_media_tbl_t *mtbl);
/*
 * Function declarations of phy media.
 */

bool dn_pas_phy_media_init (void);

bool dn_pas_is_port_pluggable(uint_t port);

char* dn_pas_media_generate_port_str(phy_media_tbl_t *mtbl);

bool dn_pas_media_set_capability_values(media_capability_t* cap,
                                        BASE_IF_SPEED_t          media_speed,
                                        BASE_CMN_BREAKOUT_TYPE_t breakout_mode,
                                        BASE_IF_SPEED_t          breakout_speed,
                                        BASE_IF_PHY_MODE_TYPE_t  phy_mode);

bool dn_pas_is_media_present(uint_t port);

PLATFORM_QSA_ADAPTER_t pas_media_get_qsa_adapter_type (phy_media_tbl_t *mtbl);

uint_t dn_pas_media_channel_count_get (PLATFORM_MEDIA_CATEGORY_t category);

PLATFORM_MEDIA_CATEGORY_t dn_pas_category_get (pas_media_t *res_data);

PLATFORM_MEDIA_TYPE_t dn_pas_media_type_get (pas_media_t *res_data);

uint_t dn_pas_media_construct_media_capabilities(phy_media_tbl_t *mtbl);

bool dn_pas_media_is_connector_separable(phy_media_tbl_t *mtbl);

BASE_IF_PHY_MODE_TYPE_t dn_pas_media_get_phy_mode_from_speed (BASE_IF_SPEED_t speed);

BASE_IF_SPEED_t dn_pas_media_convert_num_to_speed (
                    uint_t num, BASE_IF_PHY_MODE_TYPE_t phy_mode);

uint_t dn_pas_media_convert_speed_to_num (BASE_IF_SPEED_t speed);

void dn_pas_phy_media_poll (uint_t port, bool publish);

void dn_pas_phy_media_poll_all (void *arg);

uint_t dn_phy_media_count_get (void);

phy_media_tbl_t * dn_phy_media_entry_get(uint_t port);

bool dn_pas_media_obj_all_attr_add (phy_media_member_info_t const *memp,
        uint_t max_count, cps_api_attr_id_t const *list, uint_t count,
        cps_api_object_t obj, void *res_data);

BASE_IF_SPEED_t dn_pas_capability_conv (sdi_media_speed_t speed);

sdi_media_speed_t dn_pas_to_sdi_capability_conv (BASE_IF_SPEED_t speed);

sdi_media_type_t dn_pas_to_sdi_type_conv (PLATFORM_MEDIA_TYPE_t type);

bool dn_pas_is_media_unsupported (phy_media_tbl_t *mtbl, bool log_msg);

BASE_IF_SPEED_t dn_pas_media_capability_get (phy_media_tbl_t *mtbl);

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
 * dn_pas_media_channel_ext_rate_select to set extended rate select bits per channel.
 */

bool dn_pas_media_channel_ext_rate_select (uint_t port, uint_t channel, bool enable);

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
 * Enable/Disable Fiber/Serdes TX and RX, based on PHY link status.
 */
bool dn_pas_media_channel_serdes_control (uint_t port, uint_t channel);

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
/*
 * pas_media_fw_rev_get function to get firmware revision of the media.
 */
sdi_media_fw_rev_t pas_media_fw_rev_get (pas_media_t *res_data);


#ifdef __cplusplus
}
#endif
#endif  //__PAS_DATA_STORE_H

