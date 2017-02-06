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
* @file pas_media_sdi_wrapper.h
*
* @brief This file contains source code of pas media sdi wrapper functions.
**************************************************************************/

#include "sdi_entity.h"
#include "sdi_media.h"
#include "private/pald.h"

static inline t_std_error pas_sdi_media_presence_get (
        sdi_resource_hdl_t resource_hdl, bool *presence)
{
    t_std_error    ret;

    ret = sdi_media_presence_get(resource_hdl, presence);

    return ret;
}

static inline t_std_error pas_sdi_media_module_monitor_status_get(
        sdi_resource_hdl_t resource_hdl, uint_t flags, uint_t *status)
{
    t_std_error    ret;

    ret = sdi_media_module_monitor_status_get(resource_hdl, flags, status);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;

}

static inline t_std_error pas_sdi_media_channel_monitor_status_get (
        sdi_resource_hdl_t resource_hdl, uint_t channel, 
        uint_t flags, uint_t *status)
{
    t_std_error    ret;

    ret = sdi_media_channel_monitor_status_get(resource_hdl, channel,
            flags, status);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_channel_status_get (
        sdi_resource_hdl_t resource_hdl,
        uint_t channel, uint_t flags, uint_t *status)
{

    t_std_error    ret;

    ret = sdi_media_channel_status_get(resource_hdl, channel, 
            flags, status);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_tx_control (
        sdi_resource_hdl_t resource_hdl, uint_t channel, bool enable)
{
    t_std_error    ret;

    ret = sdi_media_tx_control(resource_hdl, channel, enable);

    return ret;
}

static inline t_std_error pas_sdi_media_tx_control_status_get (
        sdi_resource_hdl_t resource_hdl, uint_t channel, bool *status)
{
    t_std_error    ret;

    ret = sdi_media_tx_control_status_get(resource_hdl, channel, status);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_speed_get (
        sdi_resource_hdl_t resource_hdl, sdi_media_speed_t *speed)
{
    t_std_error    ret;

    ret = sdi_media_speed_get(resource_hdl, speed);

    return ret;
}


static inline t_std_error pas_sdi_media_is_dell_qualified (
        sdi_resource_hdl_t resource_hdl, bool *status)
{
    t_std_error    ret;

    ret = sdi_media_is_dell_qualified(resource_hdl, status);

    return ret;
}

static inline t_std_error pas_sdi_media_parameter_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_param_type_t param_type, uint_t *value)
{
    t_std_error    ret;

    ret = sdi_media_parameter_get(resource_hdl, param_type, value);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_vendor_info_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_vendor_info_type_t vendor_info_type,
        char *vendor_info, size_t buf_size)
{
    t_std_error    ret;

    ret = sdi_media_vendor_info_get(resource_hdl, vendor_info_type,
            vendor_info, buf_size);

    return ret;
}

static inline t_std_error pas_sdi_media_transceiver_code_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_transceiver_descr_t *transceiver_info)
{
    t_std_error    ret;


    ret = sdi_media_transceiver_code_get(resource_hdl, transceiver_info);

    return ret;
}

static inline t_std_error pas_sdi_media_dell_product_info_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_dell_product_info_t *info)
{
    t_std_error    ret;

    ret = sdi_media_dell_product_info_get(resource_hdl, info);

    return ret;
}

static inline t_std_error pas_sdi_media_threshold_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_threshold_type_t threshold_type, float *value)
{
    t_std_error    ret;

    ret = sdi_media_threshold_get(resource_hdl, threshold_type, value);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_module_control (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_module_ctrl_type_t ctrl_type, bool enable)
{
    t_std_error    ret;

    ret = sdi_media_module_control(resource_hdl, ctrl_type, enable);

    return ret;
}

static inline t_std_error pas_sdi_media_module_control_status_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_module_ctrl_type_t ctrl_type, bool *status)
{
    t_std_error    ret;

    ret = sdi_media_module_control_status_get(resource_hdl, ctrl_type, status);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_module_monitor_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_module_monitor_t monitor, float *value)
{
    t_std_error    ret;

    ret = sdi_media_module_monitor_get(resource_hdl, monitor, value);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_channel_monitor_get (
        sdi_resource_hdl_t resource_hdl, uint_t channel,
        sdi_media_channel_monitor_t monitor, float *value)
{
    t_std_error    ret;

    ret = sdi_media_channel_monitor_get(resource_hdl, channel, monitor, value);

    ret = ((ret == STD_ERR_OK) || (STD_ERR_EXT_PRIV(ret) == EOPNOTSUPP))
           ? STD_ERR_OK : ret;

    return ret;
}

static inline t_std_error pas_sdi_media_led_set (sdi_resource_hdl_t resource_hdl,
        uint_t channel, sdi_media_speed_t speed)
{
    t_std_error    ret;

    ret = sdi_media_led_set(resource_hdl, channel, speed);

    return ret;
}

static inline t_std_error pas_sdi_media_feature_support_status_get (
        sdi_resource_hdl_t resource_hdl,
        sdi_media_supported_feature_t *feature_support)
{
    t_std_error   ret;

    ret = sdi_media_feature_support_status_get(resource_hdl, feature_support);

    return ret;
}
