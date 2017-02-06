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
 * file : pas_fuse_entity_info.c
 * brief: pas daemon interfthe layer to entity_info SDI API
 * date : 04/2015
 *
 */


#include "private/pas_fuse_handlers.h"

/*
 * PAS Daemon entity_info read interfthe
 */
int dn_pas_fuse_entity_info_read(
        dev_node_t *node,
        char       *buf, 
        size_t     size, 
        off_t      offset
        )
{
    int               res                               = -ENOTSUP;
    size_t            len                               = 0;
    char              trans_buf[FUSE_FILE_DEFAULT_SIZE] = { 0 };
    memset(buf, 0, size);
    sdi_entity_info_t entity_info;

    /** check for node & buffer validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    if (STD_ERR_OK ==
            sdi_entity_info_read(node->fuse_resource_hdl, &entity_info)) {

        res = size;

    } else {

        return res;
    }
    
    switch (node->fuse_filetype) {

        /** alias */
        case FUSE_ENTITY_INFO_FILETYPE_ALIAS:
            {

                const char *alias_name;
                if (NULL != (alias_name = sdi_resource_alias_get(node->fuse_resource_hdl))) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res, 
                            "%-25s : %s", "Alias", (char *) alias_name);
                }
                break;
            }

        /** product name file read */
        case FUSE_ENTITY_INFO_FILETYPE_PROD_NAME:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                   "%-25s : %s", "Product Name", (entity_info.prod_name[0] != '\0' ? entity_info.prod_name : "N/A"));
            break;

        /** ppid file read */
        case FUSE_ENTITY_INFO_FILETYPE_PPID:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                   "%-25s : %s", "Product PPID", (entity_info.ppid[0] != '\0' ? entity_info.ppid : "N/A"));
            break;

        /** hardware revision file read */
        case FUSE_ENTITY_INFO_FILETYPE_HW_REVISION:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                   "%-25s : %s", "Hardware Rev", (entity_info.hw_revision[0] != '\0' ? entity_info.hw_revision : "N/A"));
            break;

        /** platform name file read */
        case FUSE_ENTITY_INFO_FILETYPE_PLATFORM_NAME:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                   "%-25s : %s", "Platform Name", (entity_info.platform_name[0] != '\0' ? entity_info.platform_name : "N/A"));
            break;

        /** vendor name file read */
        case FUSE_ENTITY_INFO_FILETYPE_VENDOR_NAME:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                     "%-25s : %s", "Vendor Name", (entity_info.vendor_name[0] != '\0' ? entity_info.vendor_name : "N/A"));
            break;
       
        /** vendor name file read */
        case FUSE_ENTITY_INFO_FILETYPE_SERVICE_TAG:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                     "%-25s : %s", "Service Tag", (entity_info.service_tag[0] != '\0' ? entity_info.service_tag : "N/A"));
            break;
        
        /** airflow file read */
        case FUSE_ENTITY_INFO_FILETYPE_AIRFLOW:
            dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                    "%-25s : %s", "Airflow",
                    (SDI_PWR_AIR_FLOW_NORMAL == entity_info.air_flow ? "Normal" : "Reverse"));
            break;

        /** mac size file read */
        case FUSE_ENTITY_INFO_FILETYPE_MAC_SIZE:
            {
                if(entity_info.mac_size != 0) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i", "MAC Size", entity_info.mac_size);
                } else {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %s", "MAC Size", "N/A");
                }
                break;
            }

        /** number of fans file read */
        case FUSE_ENTITY_INFO_FILETYPE_NUM_FANS:
            { 
                if(entity_info.num_fans != 0) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i", "Number of Fans", entity_info.num_fans);
                } else {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %s", "Number of Fans", "N/A");
                }
                break;
            }

        /** maximum speed file read */
        case FUSE_ENTITY_INFO_FILETYPE_MAX_SPEED:
            {
                if(entity_info.max_speed != 0) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i RPM", "Max Fan Speed", entity_info.max_speed);
                } else {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %s", "Max Fan Speed", "N/A");
                }
                break;
            }

        /** power rating file read */
        case FUSE_ENTITY_INFO_FILETYPE_POWER_RATING:
            {       
                if(entity_info.power_rating != 0) {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %i W", "Power Rating", entity_info.power_rating);
                } else {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %s", "Power Rating", "N/A");
                }
                break;
            }

        /** base mac file read */
        case FUSE_ENTITY_INFO_FILETYPE_BASE_MAC:
            {
                if(node->fuse_entity_type == SDI_ENTITY_SYSTEM_BOARD) {
                    snprintf(trans_buf, FUSE_FILE_DEFAULT_SIZE, 
                            "%-25s : %02x : %02x : %02x : %02x : %02x : %02x",
                            "Base MAC",
                            entity_info.base_mac[0],
                            entity_info.base_mac[1],
                            entity_info.base_mac[2],
                            entity_info.base_mac[3],
                            entity_info.base_mac[4],
                            entity_info.base_mac[5]);

                    len = strlen(trans_buf) + 1;
                    res = len;
                } else {

                    dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                           "%-25s : %s", "Base MAC", "N/A");
                }
                break;
            }


        /** diagnostic mode */
        case FUSE_ENTITY_INFO_FILETYPE_DIAG_MODE:
            {

                dn_pas_fuse_print(trans_buf, FUSE_FILE_DEFAULT_SIZE, &len, &res,
                        "%-25s : %s", "Diagnostic Mode", (dn_pald_diag_mode_get() ? "up" : "down"));
                break;
            }

        default:
            {    
                if(node->fuse_filetype >= FUSE_ENTITY_INFO_FILETYPE_MIN && 
                   node->fuse_filetype < FUSE_ENTITY_INFO_FILETYPE_MAX) {
                    
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




/*
 * PAS Daemon entity_info write interfthe
 */
int dn_pas_fuse_entity_info_write(
        dev_node_t *node, 
        const char *buf,
        size_t     size, 
        off_t      offset
        )
{
    int    res   = -ENOTSUP;
    uint_t state = 0;


    /** check for node validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    /** switch on filetype */
    switch (node->fuse_filetype) {

        /** diagnostic mode */
        case FUSE_ENTITY_INFO_FILETYPE_DIAG_MODE:
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
                if(node->fuse_filetype >= FUSE_ENTITY_INFO_FILETYPE_MIN && 
                   node->fuse_filetype < FUSE_ENTITY_INFO_FILETYPE_MAX) {
                    
                    res = -EPERM;
                } else {
                    
                    res = -ENOENT;
                }
                break;
            }
    }

    return res;
}
