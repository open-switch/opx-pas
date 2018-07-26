/*
 * Copyright (c) 2018 Dell EMC..
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

#include "private/pas_config.h"
#include "private/pas_comm_dev.h"
#include "private/pas_ext_ctrl.h"
#include "private/pas_host_system.h"
#include "private/pas_temp_sensor.h"
#include "private/pas_entity.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_utils.h"
#include "private/pas_data_store.h"
#include "private/pas_log.h"
#include "private/pald.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "sdi_ext_ctrl.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>

static pas_extctrl_group_t db_extctrl; /* External Control database */

/* Create a extctrl cache record */

#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))

void dn_cache_init_extctrl (sdi_resource_hdl_t sdi_resource_hdl, void *data)
{
    pas_entity_t *parent = (pas_entity_t *) data;
    pas_extctrl_t *rec;
    pas_config_extctrl *cfg_ctrl = dn_pas_config_extctrl_get();
    uint_t type_idx = 0;
    const char *ctrl_name = sdi_resource_alias_get(sdi_resource_hdl);
    bool res_found = false;
    int i;

    if (sdi_resource_hdl == NULL) {
        return;
    }

    for (i = 0; i < cfg_ctrl->slist_cnt; i++) {
        if (strcmp(ctrl_name, cfg_ctrl->slist_config[i].extctrl) == 0) {
            res_found = true;
            type_idx = i;

            break;
        }
    }

    if (res_found == false) {
        return;
    }

    rec = &db_extctrl.extctrls[type_idx];

    rec->idx  = type_idx;
    rec->slot = parent->slot;
    rec->entity_type = parent->entity_type;
    rec->sdi_extctrl_hdl = sdi_resource_hdl;

    db_extctrl.num_ctrls++;

    STRLCPY(rec->name, cfg_ctrl->slist_config[type_idx].extctrl);

    char res_key_name[PAS_RES_KEY_SIZE];

    if (!dn_pas_res_insertc(dn_pas_res_key_extctrl_name(res_key_name,
                                                        sizeof(res_key_name),
                                                        rec->entity_type,
                                                        rec->slot,
                                                        rec->name
                                                        ), rec
                            )
        ) {
        return;
    }

    char res_key_idx[PAS_RES_KEY_SIZE];

    if (!dn_pas_res_insertc(dn_pas_res_key_extctrl_idx(res_key_idx,
                                                       sizeof(res_key_idx),
                                                       rec->entity_type,
                                                       rec->idx), rec
                            )
        ) {
        dn_pas_res_removec(res_key_name);
        return;
    }

    char res_sensor_key_name[PAS_RES_KEY_SIZE];
    
    pas_extctrl_slist_config *slist = &cfg_ctrl->slist_config[type_idx];
    for (i = 0; i < slist->count; i++) {
        if (!dn_pas_res_insertc(dn_pas_res_key_extctrl_sensor_name(res_sensor_key_name,
                                                                   sizeof(res_sensor_key_name),
                                                                   slist->sensor[i].name), rec
                                )
            ) {
            dn_pas_res_removec(res_key_name);
            dn_pas_res_removec(res_key_idx);
            return;
        }
    }  

    return;
}

pas_extctrl_t *dn_pas_extctrl_rec_get_name(pas_entity_t *parent, char *extctrl_name)
{
  char res_key[PAS_RES_KEY_SIZE];

  return ((pas_extctrl_t *)
          dn_pas_res_getc(dn_pas_res_key_extctrl_name(res_key, sizeof(res_key),
                                                      parent->entity_type,
                                                      parent->slot,
                                                      extctrl_name
                                                      )
                          )
          );
}

pas_extctrl_t *dn_pas_extctrl_rec_get_idx(uint_t extctrl_idx)
{
  if (extctrl_idx >= db_extctrl.num_ctrls) return NULL;

  return (&db_extctrl.extctrls[extctrl_idx]);
}

pas_extctrl_t *dn_pas_extctrl_rec_get_by_sensor (char *sensor_name)
{
    char res_key[PAS_RES_KEY_SIZE];

    return ((pas_extctrl_t *)
            dn_pas_res_getc(dn_pas_res_key_extctrl_sensor_name(res_key, sizeof(res_key),
                                                               sensor_name)));
}

void dn_cache_init_extctrl_db (pas_entity_t *cd_rec)
{
    pas_config_extctrl* cfg_ctrl = dn_pas_config_extctrl_get();

    memset(&db_extctrl, 0, sizeof(db_extctrl)); /* start from scratch */
    db_extctrl.extctrls = (pas_extctrl_t *) CALLOC_T(pas_extctrl_t, cfg_ctrl->slist_cnt);
    if (db_extctrl.extctrls == NULL)
        PAS_ERR("ext control Run out of memory");
}

bool dn_cache_del_extctrl(pas_entity_t *parent)
{
    uint_t extctrl_idx;
    pas_extctrl_t *rec;
    pas_config_extctrl* cfg_ctrl = dn_pas_config_extctrl_get();

    for (extctrl_idx = 0; extctrl_idx < db_extctrl.num_ctrls; extctrl_idx++) {
        char res_key[PAS_RES_KEY_SIZE];
        char res_sensor_key_name[PAS_RES_KEY_SIZE];
      
        rec = dn_pas_res_removec(dn_pas_res_key_extctrl_idx(res_key,
                                 sizeof(res_key),
                                 parent->entity_type,
                                 extctrl_idx));
        if (NULL == rec) return false;

        int i;
        pas_extctrl_slist_config *slist = &cfg_ctrl->slist_config[extctrl_idx];
        for (i = 0; i < slist->count; i++) {
            dn_pas_res_removec(dn_pas_res_key_extctrl_sensor_name(res_sensor_key_name,
                                                                  sizeof(res_sensor_key_name),
                                                                  slist->sensor[i].name));
        }   

        dn_pas_res_removec(dn_pas_res_key_extctrl_name(res_key,
                                                       sizeof(res_key),
                                                       rec->entity_type,
                                                       rec->slot,
                                                       rec->name ));      
    }

    free(db_extctrl.extctrls);
    memset(&db_extctrl, 0, sizeof(db_extctrl)); /* erase the data base */

    return true;
}

static bool dn_pas_extctrl_set_value (pas_extctrl_t *rec, char *ctrl_name, int temp)
{
    int tmp_sz = 1;
    bool rc = true;

    if (NULL == rec->sdi_extctrl_hdl) {
        return false;
    }

    dn_pas_lock();
    do {
        if (dn_pald_diag_mode_get()) {
            break;
        }

        if (STD_IS_ERR(sdi_ext_ctrl_set(rec->sdi_extctrl_hdl, &temp, tmp_sz))) {
            PAS_ERR("ext control not set to %d for %s", temp, ctrl_name);
            rc = false;
            break;
        }   
    } while (0);
    dn_pas_unlock();

    return rc;
}


bool dn_entity_extctrl_poll (pas_entity_t *card_rec, bool update_allf, bool *notif)
{
    int ctrl_idx;
    pas_extctrl_t *rec;
    pas_temperature_sensor_t *tmp_rec;
    char *ctrl_name;
    int temp, aggreg_temp, aggreg_cnt = 0;
    pas_config_extctrl *cfg_ctrl = dn_pas_config_extctrl_get();
    uint_t type;
  
    for (ctrl_idx = 0; ctrl_idx < db_extctrl.num_ctrls; ctrl_idx++) {
        rec = &db_extctrl.extctrls[ctrl_idx];
        aggreg_temp = 0;

        int i;
        pas_extctrl_slist_config *slist = &cfg_ctrl->slist_config[ctrl_idx];
        
        type = slist->type;
        aggreg_cnt = 0;
        for (i = 0; i < slist->count; i++) {
            tmp_rec = dn_pas_temperature_rec_get_name(rec->entity_type,
                                                      rec->slot,
                                                      slist->sensor[i].name);      
            if (tmp_rec) {
                temp = tmp_rec->cur;
                if (PAS_SLIST_TYPE_MAX == type) {
                    aggreg_temp = ((aggreg_temp > temp)? aggreg_temp: temp);
                } else { /* PAS_SLIST_TYPE_AVG */
                    aggreg_temp += temp;
                }
                aggreg_cnt++;
            } else {
                PAS_ERR("slist sensor %s not found", slist->sensor[i].name);
                return false;
            }
        }
      
        if (aggreg_cnt && (PAS_SLIST_TYPE_AVG == type)) {
            aggreg_temp /= aggreg_cnt;
        }

        ctrl_name = slist->extctrl;
        if (false == dn_pas_extctrl_set_value(rec, ctrl_name, aggreg_temp)) {
            return false;
        }
    }

    return true;
}
