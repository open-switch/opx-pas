/*
 * Copyright (c) 2018 Dell Inc.
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

/*  Prototypes for functions to be supplied by vendor-specific media handling plug-in
 */

/* Check if media is qualified */
bool dn_pas_media_vendor_is_qualified(sdi_resource_hdl_t res_hdl, bool *qualified);

/* Read vendor-specific information from media adapter */
t_std_error dn_pas_media_vendor_product_info_get(sdi_resource_hdl_t res_hdl, uint8_t *buf);
