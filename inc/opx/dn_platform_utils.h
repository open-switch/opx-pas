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

/** ************************************************************************
 *
 * \file dn_platform_utils.h
 *
 * API definition for platform common functions.
 */

#ifndef __DN_PLATFORM_UTILS_H
#define __DN_PLATFORM_UTILS_H

#include "std_type_defs.h"     

#ifdef __cplusplus
extern "C" {
#endif



/** ************************************************************************
 *
 * \brief Return current unit number
 *
 * Return the unit number of the card the current CPU is running on.
 * Unit numbers start at 1, and are consecutive, up to a platform-dependent
 * limit.
 *
 * \param[out] unit Pointer to where to return slot number
 *
 * \returns Boolean; true <=> successful
 */

bool dn_platform_unit_id_get(uint_t *unit);

/** ************************************************************************
 *
 * \brief Set current unit number
 *
 * Set the unit number of the card the current CPU is running on.
 * Unit numbers start at 1, and are consecutive, up to a platform-dependent
 * limit.
 *
 * \param[in] unit number to set for the card/stack unit
 *
 * \returns Boolean; true <=> successful
 */

bool dn_platform_unit_id_set(uint_t unit);



#ifdef __cplusplus
}
#endif

#endif /* !defined(__DN_PLATFORM_UTILS_H) */
