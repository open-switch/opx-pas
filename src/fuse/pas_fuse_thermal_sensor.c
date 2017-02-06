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

/**
 * file : pas_fuse_thermal_sensor.c
 * brief: pas daemon interface layer to thermal_sensor SDI API
 * date : 04/2015
 *
 */

#include "private/pas_fuse_handlers.h"



/** PAS Daemon thermal_sensor write helper function */
static int dn_pas_fuse_thermal_sensor_threshold_set(
        dev_node_t      *node,
        const char      *buf,
        sdi_threshold_t threshold
        )
{
    uint_t temperature = 0;

    if (dn_pas_fuse_atoui(buf, &temperature)) {
        
        return sdi_temperature_threshold_set(node->fuse_resource_hdl,
                threshold, temperature);
    }

    return -EINVAL;
}



/** PAS Daemon thermal_sensor read interface */
int dn_pas_fuse_thermal_sensor_read(
        dev_node_t *node, 
        char       *buf,
        size_t     size, 
        off_t      offset
        )
{
    int    res                               = -ENOTSUP;
    size_t len                               = 0;
    char   trans_buf[FUSE_FILE_DEFAULT_SIZE] = { 0 };
    memset(buf, 0, size);
    
    /** check for node & buffer validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    /** switch on type of file to be read */
    switch (node->fuse_filetype) {

        /** alias */
        case FUSE_THERMAL_SENSOR_FILETYPE_ALIAS:
            {
                const char *alias_name;
                
                if (NULL != (alias_name = sdi_resource_alias_get(node->fuse_resource_hdl))) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                            "%-25s : %s", "Alias", (char *) alias_name);
                }
                break;
            }


        /** temperature value read */
        case FUSE_THERMAL_SENSOR_FILETYPE_TEMPERATURE:
            {
                int temperature;

                if (STD_ERR_OK ==
                        sdi_temperature_get(node->fuse_resource_hdl, &temperature)) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i deg C", "Temperature", temperature);
                }
                break;
            }


        /** low threshold value read */
        case FUSE_THERMAL_SENSOR_FILETYPE_LOW_THRESHOLD:
            {
                int threshold;

                if (STD_ERR_OK ==
                        sdi_temperature_threshold_get(node->fuse_resource_hdl,
                            SDI_LOW_THRESHOLD, &threshold)) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i deg C", "Low Threshold", threshold);
                }
                break;
            }


        /** high threshold value read */
        case FUSE_THERMAL_SENSOR_FILETYPE_HIGH_THRESHOLD:
            {
                int threshold;

                if (STD_ERR_OK ==
                        sdi_temperature_threshold_get(node->fuse_resource_hdl,
                            SDI_HIGH_THRESHOLD, &threshold)) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i deg C", "High Threshold", threshold);
                }
                break;
            }


        /** critical threshold value read */
        case FUSE_THERMAL_SENSOR_FILETYPE_CRITICAL_THRESHOLD:
            {
                int         threshold;
                t_std_error err;

                if ((err =
                        sdi_temperature_threshold_get(node->fuse_resource_hdl,
                            SDI_CRITICAL_THRESHOLD, &threshold)) == STD_ERR_OK) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i deg C", "Critical Threshold", threshold);

                } else if (STD_ERR_EXT_PRIV(err) == ENOTSUP) {

                    res = -ENOTSUP;
                }
                
                break;
            }


        /** fault status value read */
        case FUSE_THERMAL_SENSOR_FILETYPE_ALERT_ON:
            {
                bool alert = false;

                if (STD_ERR_OK ==
                        sdi_temperature_status_get(node->fuse_resource_hdl, &alert)) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                           "%-25s : %s", "Alert", alert ? "on" : "off");
                }
                break;
            }

        /** diagnostic mode */
        case FUSE_THERMAL_SENSOR_FILETYPE_DIAG_MODE:
            {

                dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                        "%-25s : %s", "Diagnostic Mode", (dn_pald_diag_mode_get() ? "up" : "down"));
                break;
            }

        default:
            {    
                if(node->fuse_filetype >= FUSE_THERMAL_SENSOR_FILETYPE_MIN && 
                   node->fuse_filetype < FUSE_THERMAL_SENSOR_FILETYPE_MAX) {
                    
                    res = -EPERM;
                } else {
                    
                    res = -ENOENT;
                }
                break;
            }
    }

    if (offset < len) {

        size = dn_pas_fuse_calc_size(len, size, offset);
        memcpy(buf, &trans_buf[offset], size);
        res = size;
    }

    return res;
}

/** PAS Daemon thermal_sensor write interface */
int dn_pas_fuse_thermal_sensor_write(
        dev_node_t *node, 
        const char *buf,
        size_t     size, 
        off_t offset
        )
{

    uint_t state = 0;
    int    res   = -ENOTSUP;

    /** check for node & buffer validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    /** switch on the type of file to write to */
    switch (node->fuse_filetype) {

        /** write low threshold value to file */
        case FUSE_THERMAL_SENSOR_FILETYPE_LOW_THRESHOLD:
            {

                if (STD_ERR_OK ==
                        dn_pas_fuse_thermal_sensor_threshold_set(node, buf, SDI_LOW_THRESHOLD)) {
                    res = size;
                }
                break;
            }


        /** write high threshold value to file */
        case FUSE_THERMAL_SENSOR_FILETYPE_HIGH_THRESHOLD:
            {

                if (STD_ERR_OK ==
                        dn_pas_fuse_thermal_sensor_threshold_set(node, buf, SDI_HIGH_THRESHOLD)) {
                    res = size;
                }
                break;
            }


        /** write critical threshold value to file */
        case FUSE_THERMAL_SENSOR_FILETYPE_CRITICAL_THRESHOLD:
            {
                res = -ENOTSUP;
                if (STD_ERR_OK ==
                        dn_pas_fuse_thermal_sensor_threshold_set(node, buf, SDI_CRITICAL_THRESHOLD))
                {
                    res = size;
                }
                break;
            }

        /** diagnostic mode */
        case FUSE_THERMAL_SENSOR_FILETYPE_DIAG_MODE:
            {
                if (dn_pas_fuse_atoui(buf, &state)) {

                    switch(state) {
                        case 1  : /** TODO: define variable and set to true */
                                  res = size;
                                  break;
                        
                        case 0  : /** TODO: define variable and set to false */
                                  res = size;
                                  break;
                        
                        default : res = -EINVAL;
                                  break;
                    }
                
                } else res = -EINVAL;
                
                break;
            }
        
        default:
            {    
                if(node->fuse_filetype >= FUSE_THERMAL_SENSOR_FILETYPE_MIN && 
                   node->fuse_filetype < FUSE_THERMAL_SENSOR_FILETYPE_MAX) {
                    
                    res = -EPERM;
                } else {
                    
                    res = -ENOENT;
                }
                break;
            }
    }

    return res;
}
