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

/*
 * filename: pas_chassis_handler.c
 */

#include "private/pas_main.h"
#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_utils.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_event.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"

#include "dell-base-pas.h"
#include "sdi_entity.h"
#include "sdi_entity_info.h"
#include "cps_class_map.h"

#include <stdlib.h>

#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))


/* Send an entity notification */

static bool dn_pas_chassis_notify(pas_chassis_t *rec)
{
    cps_api_object_t obj;

    obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {
        return (false);
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                    BASE_PAS_CHASSIS_OBJ,
                                    cps_api_qualifier_OBSERVED
                                    );

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_CHASSIS_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_CHASSIS_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );

    return (dn_pas_cps_notify(obj));
}

/* Initialize cache records for chassis */

bool dn_cache_init_chassis(void)
{
    sdi_entity_hdl_t   entity_hdl;
    sdi_resource_hdl_t entity_info_hdl;
    pas_chassis_t      *rec;

    /** \todo Generalize; chassis EEPROM is not EEPROM on (SYSTEM_BOARD, 1) */

    entity_hdl = sdi_entity_lookup(SDI_ENTITY_SYSTEM_BOARD, 1);
    if (entity_hdl == 0)  return (false);

    entity_info_hdl = dn_pas_entity_info_hdl(entity_hdl);
    if (entity_info_hdl == 0)  return (false);

    rec = (pas_chassis_t *) CALLOC_T(pas_chassis_t, 1);
    if (rec == 0)  return (false);

    rec->sdi_entity_info_hdl = entity_info_hdl;

    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    if (!dn_pas_res_insertc(dn_pas_res_key_chassis(), rec)) {
        free(rec);

        return (false);
    }

    return (true);
}

/* Update chassis cache record  */

static inline void chassis_resp_set(
    bool                   *result,
    char                   *resp_buf,
    uint_t                 resp_buf_size,
    char                   *sdi_buf,
    char                   *chassis_cfg_buf,
    pas_oper_fault_state_t *oper_fault_state,
    const char             *mesg
                                    )
{
    char *s = 0;

    if (sdi_buf[0] != 0) {
        s = sdi_buf;
    } else if (chassis_cfg_buf[0] != 0) {
        s = chassis_cfg_buf;
    }

    if (s == 0) {
        PAS_ERR("Chassis EEPROM %s not programmed", mesg);

        dn_pas_oper_fault_state_update(oper_fault_state,
                                       PLATFORM_FAULT_TYPE_ECFG
                                       );
        *result = false;
    } else {
        strncpy(resp_buf, s, resp_buf_size);
        resp_buf[resp_buf_size - 1] = 0;
    }
}

bool dn_pas_chassis_poll(pas_chassis_t *rec)
{
    bool                   result = true;
    sdi_entity_info_t      sdi_entity_info[1], *chassis_cfg;
    pas_oper_fault_state_t oper_fault_state[1];
    bool                   notif = false;

    dn_pas_oper_fault_state_init(oper_fault_state);

    /* Get chassis EEPROM from SDI */

    if (STD_IS_ERR(sdi_entity_info_read(rec->sdi_entity_info_hdl,
                                        sdi_entity_info
                                        )
                   )
        ) {
        dn_pas_oper_fault_state_update(oper_fault_state,
                                       PLATFORM_FAULT_TYPE_ECOMM
                                       );

        memset(sdi_entity_info, 0, sizeof(*sdi_entity_info));
    }

    /* Get chassis config defaults */

    chassis_cfg = dn_pas_config_chassis_get();

    /* For each response field, use SDI or config file information */

    chassis_resp_set(&result,
                     rec->eeprom->vendor_name,
                     sizeof(rec->eeprom->vendor_name),
                     sdi_entity_info->vendor_name,
                     chassis_cfg->vendor_name,
                     oper_fault_state,
                     "vendor name"
                     );
    chassis_resp_set(&result,
                     rec->eeprom->product_name,
                     sizeof(rec->eeprom->product_name),
                     sdi_entity_info->prod_name,
                     chassis_cfg->prod_name,
                     oper_fault_state,
                     "product name"
                     );
    chassis_resp_set(&result,
                     rec->eeprom->hw_version,
                     sizeof(rec->eeprom->hw_version),
                     sdi_entity_info->hw_revision,
                     chassis_cfg->hw_revision,
                     oper_fault_state,
                     "hardware revision"
                     );
    chassis_resp_set(&result,
                     rec->eeprom->platform_name,
                     sizeof(rec->eeprom->platform_name),
                     sdi_entity_info->platform_name,
                     chassis_cfg->platform_name,
                     oper_fault_state,
                     "platform name"
                     );
    chassis_resp_set(&result,
                     rec->eeprom->ppid,
                     sizeof(rec->eeprom->ppid),
                     sdi_entity_info->ppid,
                     chassis_cfg->ppid,
                     oper_fault_state,
                     "PPID"
                     );
    chassis_resp_set(&result,
                     rec->eeprom->part_number,
                     sizeof(rec->eeprom->part_number),
                     sdi_entity_info->part_number,
                     chassis_cfg->part_number,
                     oper_fault_state,
                     "part number"
                     );
    chassis_resp_set(&result,
                     rec->eeprom->service_tag,
                     sizeof(rec->eeprom->service_tag),
                     sdi_entity_info->service_tag,
                     chassis_cfg->service_tag,
                     oper_fault_state,
                     "service tag"
                     );

    uint8_t empty_mac_addr[6] = { 0 }, *m = 0;

    if (memcmp(sdi_entity_info->base_mac,
               empty_mac_addr,
               sizeof(empty_mac_addr)
               )
        != 0
        ) {
        m = sdi_entity_info->base_mac;
    } else if (memcmp(chassis_cfg->base_mac,
                      empty_mac_addr,
                      sizeof(empty_mac_addr)
                      )
               != 0
               ) {
        m = chassis_cfg->base_mac;
    }
    if (m == 0) {
        PAS_ERR("Chassis EEPROM base mac address not programmed");

        dn_pas_oper_fault_state_update(oper_fault_state,
                                       PLATFORM_FAULT_TYPE_ECFG
                                       );
        result = false;
    } else {
        memcpy(rec->base_mac_addresses, m, sizeof(rec->base_mac_addresses));
    }

    if (sdi_entity_info->mac_size != 0) {
        rec->num_mac_addresses = sdi_entity_info->mac_size;
    } else if (chassis_cfg->mac_size != 0) {
        rec->num_mac_addresses = chassis_cfg->mac_size;
    }
    if (rec->num_mac_addresses == 0) {
        PAS_ERR("Chassis EEPROM number of mac addresses not programmed");

        dn_pas_oper_fault_state_update(oper_fault_state,
                                       PLATFORM_FAULT_TYPE_ECFG
                                       );

        result = false;
    }

    if (rec->oper_fault_state->oper_status != oper_fault_state->oper_status) {
        notif = true;
    }

    *rec->oper_fault_state = *oper_fault_state;

    if (notif)  dn_pas_chassis_notify(rec);

    return (result);
}

t_std_error dn_pas_chassis_get(cps_api_get_params_t * param, size_t key_idx)
{
    cps_api_key_t    *key = &param->keys[key_idx];
    pas_chassis_t    *rec;
    cps_api_object_t obj;

    rec = dn_pas_res_getc(dn_pas_res_key_chassis());
    if (rec == 0)  return (STD_ERR(PAS, NEXIST, 0));

    if (!rec->valid || cps_api_key_get_qual(key) == cps_api_qualifier_REALTIME) {
        if (dn_pas_timedlock() != STD_ERR_OK) {
            PAS_ERR("Not able to acquire the mutex (timeout)");
            return (STD_ERR(PAS, FAIL, 0));
        }

        if (!dn_pald_diag_mode_get())  rec->valid = dn_pas_chassis_poll(rec);

        dn_pas_unlock();
    }

    obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {

        dn_pas_unlock();
        return (STD_ERR(PAS, NOMEM, 0));
    }

    cps_api_object_set_key(obj, key);

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_CHASSIS_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_CHASSIS_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_VENDOR_NAME,
                                rec->eeprom->vendor_name
                                );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_PRODUCT_NAME,
                                rec->eeprom->product_name
                                );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_HW_VERSION,
                                rec->eeprom->hw_version
                                );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_PLATFORM_NAME,
                                rec->eeprom->platform_name
                                );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_PPID,
                                rec->eeprom->ppid
                                );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_PART_NUMBER,
                                rec->eeprom->part_number
                                );

    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_SERVICE_TAG,
                                rec->eeprom->service_tag
                                );

    char buf[15];
    dn_pas_service_tag_to_express_service_code(buf, sizeof(buf), rec->eeprom->service_tag);
    cps_api_object_attr_add_str(obj,
                                BASE_PAS_CHASSIS_SERVICE_CODE,
                                buf
                                );

    cps_api_object_attr_add(obj,
                            BASE_PAS_CHASSIS_BASE_MAC_ADDRESSES,
                            rec->base_mac_addresses,
                            sizeof(rec->base_mac_addresses)
                            );

    cps_api_object_attr_add_u32(obj,
                                BASE_PAS_CHASSIS_NUM_MAC_ADDRESSES,
                                rec->num_mac_addresses
                                );

   cps_api_object_attr_add_u8(obj,
                              BASE_PAS_CHASSIS_REBOOT,
                              rec->reboot_type
                              );

    dn_pas_unlock();

    if (!cps_api_object_list_append(param->list, obj)) {
        cps_api_object_delete(obj);
        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

#define PAS_REBOOT_CMD_LEN      (128)
#define PAS_REBOOT_REASON_LEN   (4096)
#define PAS_CMD_BUFF_SIZE       (PAS_REBOOT_REASON_LEN + PAS_REBOOT_CMD_LEN)

static t_std_error dn_pas_chassis_set1(
                    cps_api_transaction_params_t *param,
                    cps_api_qualifier_t           qual,
                    uint8_t                       reboot_type,
                    char                         *reboot_reason)
{
    cps_api_object_t old_obj;
    pas_chassis_t    *rec;
    char             cmd_buf[PAS_CMD_BUFF_SIZE] = "";

    rec = (pas_chassis_t *) dn_pas_res_getc(dn_pas_res_key_chassis());
    if (rec == 0) {
        /* Not found */

        return (STD_ERR(PAS, NEXIST, 0));
    }

    /* Add old values, for rollback */

    old_obj = cps_api_object_create();

    if (old_obj == CPS_API_OBJECT_NULL) {
        return (STD_ERR(PAS, NOMEM, 0));
    }

    dn_pas_obj_key_chassis_set(old_obj, qual, reboot_type);

    if (!cps_api_object_list_append(param->prev, old_obj)) {
        cps_api_object_delete(old_obj);

        return (STD_ERR(PAS, FAIL, 0));
    }

    old_obj = CPS_API_OBJECT_NULL; /* No longer owned */

    rec->reboot_type = reboot_type;

    snprintf(cmd_buf, sizeof(cmd_buf) - 1, "/usr/sbin/opx-reload %s",
             reboot_reason);

    if (system(cmd_buf) != 0) {
        PAS_ERR("Reboot failed");

        return (STD_ERR(PAS, FAIL, 0));
    }

    return (STD_ERR_OK);
}

t_std_error dn_pas_chassis_set(cps_api_transaction_params_t* param,
                               cps_api_object_t              obj)
{

    cps_api_qualifier_t      qual;
    uint8_t                  reboot_type;
    cps_api_object_attr_t    a;
    char                     reboot_reason[PAS_REBOOT_REASON_LEN];
    uint32_t                 len = 0;

    if (cps_api_object_type_operation(cps_api_object_key(obj)) != cps_api_oper_SET) {
        return cps_api_ret_code_ERR;
    }

    a = cps_api_object_attr_get(obj, BASE_PAS_CHASSIS_REBOOT);
    if (a == CPS_API_ATTR_NULL) {
        return (STD_ERR(PAS, FAIL, 0));
    }
    reboot_type = cps_api_object_attr_data_u8(a);

    if (reboot_type != PLATFORM_REBOOT_TYPE_COLD &&
        reboot_type != PLATFORM_REBOOT_TYPE_WARM ) {
        PAS_WARN("Invalid reboot type (%u)", reboot_type);

        return (STD_ERR(PAS, FAIL, 0));
    }
    memset(reboot_reason, 0, sizeof(reboot_reason));
    a = cps_api_object_attr_get(obj, BASE_PAS_CHASSIS_REBOOT_REASON);
    if (a != CPS_API_ATTR_NULL) {
        len = (cps_api_object_attr_len(a) > sizeof(reboot_reason) - 1)
            ?  sizeof(reboot_reason) - 1 : cps_api_object_attr_len(a);
        strncpy(reboot_reason, (char *) cps_api_object_attr_data_bin(a), len);
    }

    qual = cps_api_key_get_qual(cps_api_object_key(obj));

    dn_pas_chassis_set1(param, qual, reboot_type, reboot_reason);

    return STD_ERR_OK;
}
