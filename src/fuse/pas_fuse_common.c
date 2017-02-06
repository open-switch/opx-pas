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
 * file  : pas_fuse_common_sdi.c
 * brief : Dell Networking PAS (Physical Access Layer) SDI (System Device
 *         Interface) Layer and API common functions used by SDI interfaces.
 * date  : 03-2015
 *
 */

#include "private/pas_fuse_common.h"
#include "private/pas_fuse_handlers.h"
#include "private/pas_log.h"

#include <stdio.h>
#include <stdarg.h>


/*
 * Retrieve the fields and print them into a buffer
 */
void dn_pas_fuse_print(
        char   *buf,
        uint_t size, 
        size_t *len, 
        int    *res, 
        char   *format, 
        ...) 
{
    va_list args;
    va_start(args, format);

    vsnprintf(buf, size, format, args);
    va_end(args);

    /* Account for the trailing null terminator */
    *len = strlen(buf) + 1;
    *res = *len;
}


/*
 * Method to get the mode of the directory/file
 */
int dn_pas_fuse_get_mode(
        dev_node_t *node, 
        mode_t     *mode
        )
{
    if (NULL != node && NULL != mode) {
        
        *mode = node->st_mode;
        return 0;
    } 
    return -ENOENT;
}


/*
 * Method to retrieve the number of hard links to the directory
 */
int dn_pas_fuse_get_nlink(
        dev_node_t *node, 
        nlink_t    *nlink
        )
{
    if (NULL != node && NULL != nlink) {
        
        *nlink = node->st_nlink;
         return 0;
    }
    return -ENOENT;
}


/*
 * Method to get a fuse parameter
 */
int dn_pas_fuse_get_fspec_size(
        dev_node_t *node, 
        off_t      *size
        )
{
    if (NULL != node && NULL != size && NULL != node->get_st_size) {
        
        *size = node->get_st_size(node);
        return 0;
    }
    return -ENOENT;
}


/*
 * Method to check for device permissions
 */
bool dn_pas_fuse_check_device_permission(
        int    flags, 
        mode_t st_mode
        )
{
    if (((flags & O_ACCMODE) == O_RDONLY) && 
        ((st_mode == FUSE_FILE_MODE_READ) || 
        (st_mode == FUSE_FILE_MODE_RW))) {
        
        return true;
    }

    if (((flags & O_ACCMODE) == O_RDWR) && 
        (st_mode == FUSE_FILE_MODE_RW)) {
        
        return true;
    }

    if (((flags & O_ACCMODE) == O_WRONLY) && 
        ((st_mode == FUSE_FILE_MODE_WRITE) || 
        (st_mode == FUSE_FILE_MODE_RW))) {
        
        return true;
    }
    return false;
}


/*
 * Method to calculate size
 */
size_t dn_pas_fuse_calc_size(
        size_t len, 
        size_t size, 
        off_t  offset
        )
{
    if (offset + size > len) {

        return (len - offset);
    }
    return (size);
}


/*
 * Method for array to integer conversion  
 */
bool dn_pas_fuse_atoui(
        const char *buf, 
        uint_t     *uval
        )
{
    long int long_size  = 0;
    char     *endbuf    = NULL;

    if (NULL == buf || NULL == uval) {

        return false;
    }

    /** error checking purposes */ 
    errno = 0;

    long_size = strtoul(buf, &endbuf, 10);
 
    /* check if converted to long properly */
    if (ERANGE == errno || (*endbuf != '\0' && *endbuf != '\n')) {
            
        return false;
    }

    *uval = (uint_t) long_size;
    return true;
}
