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

/*
 * filename: pas_fuse_main.c
 */

#define FUSE_USE_VERSION 26

#include "private/pald.h"
#include "private/pas_fuse_handlers.h"
#include "private/pas_fuse_main.h"
#include "private/pas_fuse_common.h"
#include "private/pas_log.h"
#include "std_error_codes.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))


/*
 * FUSE plugin callback for getattr
 */
static int dn_pas_fuse_device_getattr(
        const char  *path, 
        struct stat *stbuf
        )
{
    int            res    = 0;
    dev_node_t     node;

    dn_pas_fuse_realtime_parser(&node, (char *) path);

    if (node.valid != false) { 

        dn_pas_fuse_get_mode(&node, &stbuf->st_mode);
        dn_pas_fuse_get_nlink(&node, &stbuf->st_nlink);
        dn_pas_fuse_get_fspec_size(&node, &stbuf->st_size);
    
    } else {
        
        res = -ENOENT;
    }
    return res;
}


/*
 * FUSE plugin callback for readdir
 */
static int dn_pas_fuse_device_readdir(
        const char            *path, 
        void                  *buf,
        fuse_fill_dir_t       filler, 
        off_t                 offset,
        struct fuse_file_info *file_info
        )
{
    int           res       = 0;
    struct        stat st   = { 0 };
    dev_node_t    node;

    dn_pas_fuse_realtime_parser(&node, (char *) path);

    if (node.valid == false) { 
        
        return -ENOENT;
    }
    
    /** check if folder exists */
    if ((dn_pas_fuse_device_getattr(path, &st) != 0) || ((st.st_mode & S_IFDIR) == 0)) {
        
        res = -ENOENT;
    } else {
        
        dn_pas_fuse_get_subdir_list((void *) &node, buf, filler);
    }

    return res;
}


/*
 * FUSE plugin callback for truncate (change or reset size of file)
 */
static int dn_pas_fuse_device_truncate(
        const char *path, 
        off_t      offset
        )
{
    return offset;
}


/*
 * Method to check if fuse file/directory node was present
 */
static int dn_pas_fuse_device_open(
        const char *path, 
        struct     fuse_file_info *fi
        )
{
    int           res = 0;
    dev_node_t    node;

    dn_pas_fuse_realtime_parser(&node, (char *) path);

    if (node.valid == false) {
        
        res = -ENOENT;
    }

    else if (dn_pas_fuse_check_device_permission(fi->flags, node.st_mode) != true) {
        
        res = -EACCES;
    }

    return res;
}


/*
 * Method to read a file/directory, based on the resource type redirecting to appropriate read handlers
 */
static int dn_pas_fuse_device_read(
        const char            *path, 
        char                  *buf, 
        size_t                size,
        off_t                 offset, 
        struct fuse_file_info *fi
        )
{
    int           res   = 0;
    dev_node_t    node;

    dn_pas_fuse_realtime_parser(&node, (char *) path);

    if (NULL == path || 
        0 != strncmp(node.path, path, strlen(path))){
        
        return -ENOENT;
    }

    if (node.st_mode & S_IFREG) {

        /* treat this as a regular file, call read based on class type */
        switch (node.fuse_resource_type) {

            case SDI_RESOURCE_FAN:
                res = dn_pas_fuse_fan_read(&node, buf, size, offset);
                break;

            case SDI_RESOURCE_TEMPERATURE:
                res = dn_pas_fuse_thermal_sensor_read(&node, buf, size, offset);
                break;

            case SDI_RESOURCE_LED:
                res = dn_pas_fuse_led_read(&node, buf, size, offset);
                break;

            case SDI_RESOURCE_DIGIT_DISPLAY_LED:
                res = dn_pas_fuse_display_led_read(&node, buf, size, offset);
                break;

            case SDI_RESOURCE_ENTITY_INFO:
                res = dn_pas_fuse_entity_info_read(&node, buf, size, offset);
                break;

            case SDI_RESOURCE_MEDIA:
                res = dn_pas_fuse_media_read(&node, buf, size, offset);
                break;
            
            case SDI_RESOURCE_NVRAM:
                return  dn_pas_fuse_nvram_read(&node, buf, size, offset);

            default:

                if(node.fuse_filetype == FUSE_DIAG_MODE_FILETYPE) {
                    
                    res = dn_pas_fuse_diag_mode_read(&node, buf, size, offset);
                    break;
                }

                res = -ENOENT;
                break;
        }

    } else {
        
        res = -ENOENT;
    }

    /* on successful read, append linefeed (assume null terminated string?) */
    if (-ENOENT != res && res < size) {
        
        strncat(buf, "\n", size - strlen(buf));
        res = strlen(buf);
    }

    return res;
}


/*
 * Method to write a file/directory, based on the resource type redirecting to appropriate write handlers
 */
static int dn_pas_fuse_device_write(
        const char            *path, 
        const char            *buf, 
        size_t                size,
        off_t                 offset, 
        struct fuse_file_info *fi
        )
{
    int           res       = 0;
    dev_node_t    node;

    dn_pas_fuse_realtime_parser(&node, (char *) path);

    if (NULL == path || 
        0 != strncmp(node.path, path, strlen(path))){
        
        return -ENOENT;
    }

    else if (node.st_mode & S_IFREG) {

        if (node.st_mode & S_IFREG) {

            switch (node.fuse_resource_type) {

                case SDI_RESOURCE_FAN:
                    res = dn_pas_fuse_fan_write(&node, buf, size, offset);
                    break;

                case SDI_RESOURCE_TEMPERATURE:
                    res = dn_pas_fuse_thermal_sensor_write(&node, buf, size, offset);
                    break;

                case SDI_RESOURCE_LED:
                    res = dn_pas_fuse_led_write(&node, buf, size, offset);
                    break;

                case SDI_RESOURCE_DIGIT_DISPLAY_LED:
                    res = dn_pas_fuse_display_led_write(&node, buf, size, offset);
                    break;

                case SDI_RESOURCE_ENTITY_INFO:
                    res = dn_pas_fuse_entity_info_write(&node, buf, size, offset);
                    break;

                case SDI_RESOURCE_MEDIA:
                    res = dn_pas_fuse_media_write(&node, buf, size, offset);
                    break;
            
                case SDI_RESOURCE_NVRAM:
                    res = dn_pas_fuse_nvram_write(&node, buf, size, offset);
                    break;

                default:
                    if(node.fuse_filetype == FUSE_DIAG_MODE_FILETYPE) {
                    
                        res = dn_pas_fuse_diag_mode_write(&node, buf, size, offset);
                        break;
                    }

                    res = -ENOENT;
                    break;
            }
        }
    }    
    return res;
}


/*
 * FUSE structure detailing what is supported beyond standard
 */
static struct fuse_operations fuse_device_oper = {

    .getattr  = dn_pas_fuse_device_getattr,
    .readdir  = dn_pas_fuse_device_readdir,
    .truncate = dn_pas_fuse_device_truncate,
    .open     = dn_pas_fuse_device_open,
    .read     = dn_pas_fuse_device_read,
    .write    = dn_pas_fuse_device_write,
};


t_std_error dn_pas_fuse_handler_thread(void *argument)
{
    int  res    = 0;
    char *fuse_argv[4];
    char buffer[1024];

    /* Set up args to fuse */

    fuse_argv[0] = dn_pald_progname_get();
    fuse_argv[1] = "-f";        /* Run in foreground */
    fuse_argv[2] = dn_pald_fuse_mount_dir_get();
    fuse_argv[3] = "-s";        /* Run in Single Threaded mode */
    
    snprintf(buffer, sizeof(buffer), "(umount -l %s; rm -rf %s; mkdir -p %s) >/dev/null 2>&1\n",
             fuse_argv[2], fuse_argv[2], fuse_argv[2]
             );
    if (system(buffer) != 0) {
        PAS_ERR("Failed to start FUSE");
        return (STD_ERR(PAS, FAIL, 0));
    }

    if (0 == res) {
        fuse_main(ARRAY_SIZE(fuse_argv), fuse_argv, &fuse_device_oper, NULL);
    }
    return STD_ERR_OK;
}
