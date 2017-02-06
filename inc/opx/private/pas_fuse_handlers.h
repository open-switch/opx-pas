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
 * file : pas_fuse_handlers.h
 * brief: pas daemon interface layer to SDI API
 * date : 04/2015
 *
 */

#ifndef __PAS_FUSE_HANDLERS_H
#define __PAS_FUSE_HANDLERS_H

#include "pas_fuse_common.h"
#include "sdi_entity_info.h"
#include "sdi_led.h"
#include "sdi_fan.h"
#include "sdi_media.h"
#include "sdi_thermal.h"
#include "sdi_entity.h"
#include "sdi_nvram.h"
#include "private/pald.h"

/** PAS Daemon diag mode read interface */
int dn_pas_fuse_diag_mode_read(dev_node_t * node, char *buf,
        size_t size, off_t offset);


/** PAS Daemon diag mode write interface */
int dn_pas_fuse_diag_mode_write(dev_node_t * node, const char *buf,
        size_t size, off_t offset);


/** PAS Daemon thermal_sensor read interface */
int dn_pas_fuse_thermal_sensor_read(dev_node_t * node, char *buf,
        size_t size, off_t offset);


/** PAS Daemon thermal_sensor write interface */
int dn_pas_fuse_thermal_sensor_write(dev_node_t * node, const char *buf,
        size_t size, off_t offset);


/** PAS Daemon entity_info read interface */
int dn_pas_fuse_media_read(dev_node_t * node, char *buf,
        size_t size, off_t offset);


/** PAS Daemon entity_info write interface */
int dn_pas_fuse_media_write(dev_node_t * node, const char *buf,
        size_t size, off_t offset);


/** PAS Daemon led read interface */
int dn_pas_fuse_led_read(dev_node_t * node, char *buf, size_t size, off_t offset);


/** PAS Daemon led write interface */
int dn_pas_fuse_led_write(dev_node_t * node, const char *buf, size_t size,
        off_t offset);


/** PAS Daemon fan read interface */
int dn_pas_fuse_fan_read(dev_node_t * node, char *buf, size_t size, off_t offset);


/** PAS Daemon fan write interface */
int dn_pas_fuse_fan_write(dev_node_t * node, const char *buf, size_t size,
        off_t offset);


/** PAS Daemon display_led read interface */
int dn_pas_fuse_display_led_read(dev_node_t * node, char *buf,
        size_t size, off_t offset);


/** PAS Daemon display_led write interface */
int dn_pas_fuse_display_led_write(dev_node_t * node, const char *buf,
        size_t size, off_t offset);


/** PAS Daemon entity_info read interface */
int dn_pas_fuse_entity_info_read(dev_node_t * node, char *buf,
        size_t size, off_t offset);


/** PAS Daemon entity_info write interface */
int dn_pas_fuse_entity_info_write(dev_node_t * node, const char *buf,
        size_t size, off_t offset);


/** PAS Daemon nvram read interface */
int dn_pas_fuse_nvram_read(dev_node_t * node, char *buf,
        size_t size, off_t offset);


/** PAS Daemon nvram write interface */
int dn_pas_fuse_nvram_write(dev_node_t * node, const char *buf,
        size_t size, off_t offset);


#endif // __PAS_FUSE_HANDLERS_H
