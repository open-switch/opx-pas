/*
 * Copyright (c) 2018 Dell Inc.
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
 * @file pas_host_system.h
 * @brief This file contains host system handling function declarations.
 ********************************************************************/


#ifndef __PAS_HOST_SYSTEM_H
#define __PAS_HOST_SYSTEM_H


#include "std_type_defs.h"
#include "ds_common_types.h"
#include "private/pas_res_structs.h"
#include "cps_api_operation.h"


/* dn_host_system_entry_get is used to get entry
 * for hostsystem device on the system
 */

pas_host_system_t * dn_host_system_rec_get(void);

/*
 * dn_pas_set_host_system_booted is used to update the host-system 
 * boot state to peer
 */

bool dn_pas_set_host_system_booted(void);

/* 
 * dn_pas_host_system_init is used to initialize host system module
 */

bool dn_pas_host_system_init (void);

/*
 * dn_pas_host_system_attr_add is used to add host-system attributes
 * to an CPS object.
 */
bool dn_pas_host_system_attr_add (cps_api_object_t obj);

#endif
