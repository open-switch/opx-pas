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
 * file : pas_fuse_nvram.c
 * brief: pas daemon interface layer to nvram SDI API
 * date : 04/2015
 *
 */


#include "private/pas_fuse_handlers.h"
#define min(x,y) (((x)<=(y)) ? (x):(y))

/*
 * PAS Daemon nvram read interface
 */
int dn_pas_fuse_nvram_read(
        dev_node_t *node,
        char       *buf, 
        size_t     size, 
        off_t      offset
        )
{
    int res = -ENOTSUP;
    memset(buf, 0, size);

    /** check for node & buffer validity */
    if ((NULL == node) || (NULL == buf)) {

        return res;
    }

    switch (node->fuse_filetype) {

        /** nvram eeprom data file read */
        case FUSE_NVRAM_FILETYPE_DATA:
            {
                uint_t  nvram_size = 0;

                if(STD_ERR_OK != sdi_nvram_size(node->fuse_resource_hdl, &nvram_size)) {
                    return res;
                }

                if (offset >= nvram_size) {
                    return 0;
                }

                uint_t read_size = min(nvram_size - offset, size); 

                if(STD_ERR_OK != sdi_nvram_read(node->fuse_resource_hdl, (uint8_t *) buf, offset, read_size)) {
                    return res;
                }

                res = read_size;
                break;
            }

        default:
            {    
                if(node->fuse_filetype >= FUSE_NVRAM_FILETYPE_MIN && 
                        node->fuse_filetype < FUSE_NVRAM_FILETYPE_MAX) {

                    res = -EPERM;
                } else {

                    res = -ENOENT;
                }
                break;
            }
    }
    return res;
}


/*
 * PAS Daemon nvram write interface
 */
int dn_pas_fuse_nvram_write(
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

        /** nvram */
        case FUSE_NVRAM_FILETYPE_DATA:
            {
                uint_t  nvram_size = 0;

                if(STD_ERR_OK != sdi_nvram_size(node->fuse_resource_hdl, &nvram_size)) {
                    return res;
                }

                if (offset >= nvram_size) {
                    return 0;
                }

                uint_t write_size = min(nvram_size - offset, size); 

                if(STD_ERR_OK != sdi_nvram_write(node->fuse_resource_hdl, (uint8_t *) buf, offset, write_size)) {
                    return res;
                }

                res = write_size;
                break;
            }

            /** diagnostic mode */
        case FUSE_NVRAM_FILETYPE_DIAG_MODE:
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
                if(node->fuse_filetype >= FUSE_NVRAM_FILETYPE_MIN && 
                        node->fuse_filetype < FUSE_NVRAM_FILETYPE_MAX) {

                    res = -EPERM;
                } else {

                    res = -ENOENT;
                }
                break;
            }
    }

    return res;
}
