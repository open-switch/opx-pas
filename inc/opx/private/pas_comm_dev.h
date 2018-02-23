/*
 * Copyright (c) 2017 Dell Inc.
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

/*********************************************************************
 * @file pas_comm_dev.h
 * @brief This file contains the structure definitions of the
 *        comm dev handling and function declarations.
 ********************************************************************/


#ifndef __PAS_COMM_DEV_H
#define __PAS_COMM_DEV_H


#include "std_type_defs.h"
#include "ds_common_types.h"
#include "private/pas_res_structs.h"
#include "cps_api_operation.h"



/*
 * dn_pas_comm_dev_init is used to initialize communication device
 */

bool dn_pas_comm_dev_init (void);

/*
 * dn_comm_dev_mb_poll is used to poll communication device for
 * mail-box messages
 */

void dn_comm_dev_poll (void);

/*
 * dn_pas_comm_dev_notify is used by comm dev module to send
 * event when a msg is read from communication device mail-box
 */

bool dn_pas_comm_dev_notify(cps_api_object_t obj);

/*
 * dn_comm_dev_rec_get is used to get comm-dev object pointer
 * for further access.
 */

pas_comm_dev_t * dn_comm_dev_rec_get(void);

/*
 * dn_pas_comm_dev_temp_set is used for setting/updating temperature
 * sensor dat in comm dev register
 */

void dn_pas_comm_dev_temp_set (int temp);

/*
 * dn_pas_comm_dev_comm_msg_write is to write mail-box message to
 * communication device.
 */

bool dn_pas_comm_dev_comm_msg_write (uint8_t * data, uint_t size);

/*
 * dn_pas_comm_dev_attr_add is used to add comm-dev cps attributes to an
 * object.
 */

bool dn_pas_comm_dev_attr_add (cps_api_object_t obj);

/*
 * dn_pas_comm_dev_host_sw_version_set is to set host-system
 * software version info in comm-dev registers.
 */

bool dn_pas_comm_dev_host_sw_version_set (char *sw_ver);

/*
 * dn_pas_comm_dev_platform_info_get is used to read platform info
 * from communication device.
 */

bool dn_pas_comm_dev_platform_info_get(void);

/*
 * dn_pas_comm_dev_messaging_enable is used to enable/disable messaging
 * from communication device.
 */

bool dn_pas_comm_dev_messaging_enable(bool enable);

#endif
