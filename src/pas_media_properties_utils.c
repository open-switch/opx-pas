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
 * @file pas_media_properties_utils.c
 *
 * @brief This file contains source code for utilities involved in qualifying media
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

/* Function to construct the display string from a series of attibutes */
/* Consumer services will add extra string if QSA detected  */

static void pas_media_construct_display_string (dn_pas_basic_media_info_t*
                                                   media_info)
{
    char far_end_str[10] = {0};
    char speed_str[20] = {0};
    char length_str[10] = {0};
    char media_if_str[20] = {0};
    char prefix_str[5] = {0};
    char postfix_str[5] = {0};

    const char fc_str[] = " FC";
    const char* trans_str =
            pas_media_get_transceiver_type_display_string(media_info->transceiver_type);
    const char* media_interface_str =
            pas_media_get_media_interface_disp_string (media_info->media_interface);
    const char* media_interface_qualifier_str =
            pas_media_get_media_interface_qualifier_disp_string(
                   media_info->media_interface_qualifier);

    uint_t far_end = pas_media_map_get_breakout_far_end_val(
            media_info->capability_list[0].breakout_mode);
    uint_t brk_speed = pas_media_map_get_speed_as_uint_mbps(
            media_info->capability_list[0].breakout_speed);
    char brk_speed_unit = 'M';
    uint_t post_fix = (media_info->media_interface_prefix
            * media_info->media_interface_lane_count)
                       / ( (far_end < 1) ? 1 : far_end);

    if (brk_speed > 1000) {
        brk_speed_unit = 'G';
        brk_speed /= 1000;
    }

    if (far_end > 1){
        snprintf (far_end_str, sizeof(far_end_str), "%ux", far_end);
    }
    if (media_info->media_interface_prefix > 1){
        snprintf (prefix_str, sizeof(prefix_str), "%d",
              media_info->media_interface_prefix);
    }
    if (post_fix > 1) {
        snprintf (postfix_str, sizeof(postfix_str), "%d", post_fix);
    }

    if (media_interface_qualifier_str[0] == '\0'){
        snprintf (media_if_str, sizeof(media_if_str)," %s%s%s", prefix_str,
              media_interface_str, postfix_str);
    } else {
        snprintf (media_if_str, sizeof(media_if_str)," %s%s%s %s", prefix_str,
              media_interface_str, postfix_str, media_interface_qualifier_str);
    }

    if (brk_speed > 0){
        snprintf (speed_str, sizeof(speed_str)," %s%d%cBASE%s", far_end_str
              , brk_speed
              , brk_speed_unit 
              , (media_info->capability_list[0].phy_mode
                               == BASE_IF_PHY_MODE_TYPE_FC) ? fc_str : "");
    }
    if (media_info->cable_length_cm > 0) {
        snprintf(length_str, sizeof(length_str)," %.1fM",
                ((float)(media_info->cable_length_cm))/100.0);
        snprintf(media_info->display_string,
                sizeof(media_info->display_string),"%s%s%s%s",
                        trans_str, speed_str, media_if_str, length_str);
    }else {
        snprintf(media_info->display_string,
                sizeof(media_info->display_string),"%s%s%s",
                        trans_str, speed_str, media_if_str);
    }
}

/* Get the cable length in cm */
/* Need enhancement */

uint_t pas_media_get_cable_length_cm (phy_media_tbl_t *mtbl)
{
    return mtbl->res_data->length_cable * 100;
}

static void pas_media_resolve_autoneg (phy_media_tbl_t *mtbl)
{
    BASE_IF_SUPPORTED_AUTONEG_t an = BASE_IF_SUPPORTED_AUTONEG_OFF_SUPPORTED;

    /** Will add code for fetching from config **/

    /* For fixed ports */
    if (mtbl->res_data->port_type == PLATFORM_PORT_TYPE_FIXED) {
        mtbl->media_info.default_autoneg =
                BASE_IF_SUPPORTED_AUTONEG_ON_SUPPORTED;
        return;
    }

    /* For pluggable ports*/
    switch (mtbl->media_info.cable_type){
        case PLATFORM_MEDIA_CABLE_TYPE_DAC:
            switch (mtbl->media_info.capability_list[0].breakout_speed){
                case BASE_IF_SPEED_10GIGE:
                    an = BASE_IF_SUPPORTED_AUTONEG_ON_SUPPORTED;
                    break;
                default:
                    an = BASE_IF_SUPPORTED_AUTONEG_OFF_SUPPORTED;
                    break;
            }
            break;

        case PLATFORM_MEDIA_CABLE_TYPE_FIBER:
            switch (mtbl->media_info.transceiver_type){
                case PLATFORM_MEDIA_CATEGORY_SFP:
                    switch (mtbl->media_info.media_interface) {
                        case PLATFORM_MEDIA_INTERFACE_SX:
                        case PLATFORM_MEDIA_INTERFACE_LX:
                            an = BASE_IF_SUPPORTED_AUTONEG_ON_SUPPORTED;
                            break;
                        default:
                            an = BASE_IF_SUPPORTED_AUTONEG_OFF_SUPPORTED;
                            break;
                    }
                    break;
                default:
                    an = BASE_IF_SUPPORTED_AUTONEG_OFF_SUPPORTED;
                    break;
            }
            break;

        default:
            an = BASE_IF_SUPPORTED_AUTONEG_OFF_SUPPORTED;
            break;
    }
    mtbl->media_info.default_autoneg = an;
}

/* Derive the basic media info from info in EEPROM and maps */

static bool pas_media_populate_basic_media_info(
                dn_pas_basic_media_info_t* media_info, sdi_media_connector_t raw_conn)
{
    PLATFORM_MEDIA_CONNECTOR_TYPE_t
            conn1 = PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN,
            conn2 = PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN,
            conn_from_eeprom = PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN;
    PLATFORM_MEDIA_CABLE_TYPE_t cab1 = PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN,
                                cab2 = PLATFORM_MEDIA_CABLE_TYPE_UNKNOWN;
    bool no_qual = true;

    conn_from_eeprom = pas_media_get_media_connector_enum(raw_conn);
    conn1 = pas_media_get_media_interface_connector_type_expected(
            media_info->media_interface);
    cab1 = pas_media_get_media_interface_cable_type_expected(
            media_info->media_interface);

    /* Handle qualifer if needed */
    if ((media_info->media_interface_qualifier
                  != PLATFORM_MEDIA_INTERFACE_QUALIFIER_NO_QUALIFIER)
    && (media_info->media_interface_qualifier
                  != PLATFORM_MEDIA_INTERFACE_QUALIFIER_UNKNOWN)) {
        conn2 = pas_media_get_media_interface_qualifier_connector_type_expected(
                  media_info->media_interface_qualifier);
        if (conn2 != PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN){
            cab2 = pas_media_get_media_interface_qualifier_cable_type_expected(
                  media_info->media_interface_qualifier);
        }
        media_info->cable_type = cab2;
        no_qual = false;
    } else {
        media_info->cable_type = cab1;
    }

    media_info->connector_type = ((conn_from_eeprom
                == PLATFORM_MEDIA_CONNECTOR_TYPE_UNKNOWN)
                ? conn_from_eeprom
                : ((no_qual) ? conn1 : conn2));

    /* If no entry exists, use entry from media interface connector */
    if (!pas_media_is_media_connector_separable(media_info->connector_type,
                &(media_info->connector_separable))){
        if (!pas_media_is_media_connector_separable(conn2,
                &(media_info->connector_separable))){
                    if (!pas_media_is_media_connector_separable(conn1,
                           &(media_info->connector_separable))){
                        return false;
            }
        }
    }
    return true;
}

/* Function to get media properties including display string, connector, cable etc */

bool pas_media_get_media_properties(phy_media_tbl_t *mtbl)
{
    dn_pas_basic_media_info_t* media_info = &(mtbl->media_info);
    media_info->transceiver_type      = 0;
    media_info->connector_type        = 0;
    media_info->cable_type            = 0;
    media_info->media_interface       = 0;
    media_info->media_interface_qualifier = 0;
    media_info->media_interface_lane_count = 0;
    media_info->media_interface_prefix = 0;
    media_info->cable_length_cm      = 0;
    media_info->connector_separable  = false;
    media_info->default_autoneg = 0;
    media_info->default_fec = 0;
    pas_media_disc_cb_t disc_cb      = NULL;   
    bool ret                         = false;

    media_info->transceiver_type      = mtbl->res_data->category;
    sdi_media_connector_t raw_conn    = mtbl->res_data->connector;

    /* should be changed to derive capability manually */
    memcpy(&(media_info->capability_list[0]),
             &(mtbl->res_data->media_capabilities[0]),
                    sizeof(media_capability_t));

    /* Get the callback to handle given trans type */
    disc_cb = pas_media_get_disc_cb_from_trans_type(
                            media_info->transceiver_type);
    if (disc_cb == NULL) {
        PAS_ERR("FATAL: No media discovery implementation for media on port %u",
                           mtbl->fp_port);
        return false;
    }
    ret = disc_cb (mtbl, media_info);

    /* to do capability stuff */

    if (!pas_media_populate_basic_media_info(media_info, raw_conn)){
        //ERROR
        ret &= false;
    }
    media_info->cable_length_cm = (media_info->connector_separable) ? 0 : pas_media_get_cable_length_cm(mtbl);

    pas_media_construct_display_string(media_info);
    pas_media_resolve_autoneg(mtbl);

    if ((media_info->display_string)[0] == '\0') {
        ret &= false;
    }
    return ret;
}
