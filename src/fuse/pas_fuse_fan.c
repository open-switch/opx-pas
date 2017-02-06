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
 * file : pas_fuse_fan.c
 * brief: pas daemon interface layer to fan SDI API
 * date : 04/2015
 *
 */

#include "private/pas_fuse_handlers.h"

/*
 * PAS Daemon fan read interface
 */
int dn_pas_fuse_fan_read(
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

    /** switch on filetype */
    switch (node->fuse_filetype) {

        /** speed */
        case FUSE_FAN_FILETYPE_SPEED:
            {
                uint_t speed;
                if (STD_ERR_OK ==
                        sdi_fan_speed_get(node->fuse_resource_hdl, &speed)) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i RPM", "Speed", (int) speed);
                }
                break;
            }

        /** fault status */
        case FUSE_FAN_FILETYPE_ALERT:
            {
                bool alert = false;

                if (STD_ERR_OK ==
                        sdi_fan_status_get(node->fuse_resource_hdl, &alert)) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                           "%-25s : %s", "Alert", alert ? "on" : "off");
                }
                break;
            }

        /** alias */
        case FUSE_FAN_FILETYPE_ALIAS:
            {
                const char *alias_name;
        
                if (NULL != (alias_name = sdi_resource_alias_get(node->fuse_resource_hdl))) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                            "%-25s : %s", "Alias", (char *) alias_name);
                }
                break;
            }


        /** diagnostic mode */
        case FUSE_FAN_FILETYPE_DIAG_MODE:
            {

                dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                        "%-25s : %s", "Diagnostic Mode", (dn_pald_diag_mode_get() ? "up" : "down"));
                break;
            }

        default:
            {    
                if(node->fuse_filetype >= FUSE_FAN_FILETYPE_MIN && 
                   node->fuse_filetype < FUSE_FAN_FILETYPE_MAX) {
                    
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


/** PAS Daemon fan write interface */
int dn_pas_fuse_fan_write(
        dev_node_t *node, 
        const char *buf, 
        size_t     size,
        off_t      offset
        )
{
    int    res   = -ENOTSUP;
    uint_t speed = 0;
    uint_t state = 0;


    /** check for node validity */
    if ((NULL == node) || (NULL == buf)) {
    
        return res;
    }

    switch (node->fuse_filetype) {
        /** speed */
        case FUSE_FAN_FILETYPE_SPEED:
            {
                if (dn_pas_fuse_atoui(buf, &speed)) {
                    if (STD_ERR_OK ==
                            sdi_fan_speed_set(node->fuse_resource_hdl, speed)) {
                        res = size;
                    }
                }
                break;
            }

        /** diagnostic mode */
        case FUSE_FAN_FILETYPE_DIAG_MODE:
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
                if(node->fuse_filetype >= FUSE_FAN_FILETYPE_MIN && 
                   node->fuse_filetype < FUSE_FAN_FILETYPE_MAX) {
                    
                    res = -EPERM;
                } else {
                    
                    res = -ENOENT;
                }
                break;
            }
    }

    return res;
}
