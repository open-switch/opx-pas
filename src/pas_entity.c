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

#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_entity.h"
#include "private/pas_psu.h"
#include "private/pas_fan_tray.h"
#include "private/pas_fan.h"
#include "private/pas_temp_sensor.h"
#include "private/pas_led.h"
#include "private/pas_power_monitor.h"
#include "private/pas_display.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_event.h"
#include "private/pas_utils.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_service.h"

#include <stdlib.h>


/* Send an entity notification */

static bool dn_entity_notify(pas_entity_t *rec)
{
    cps_api_object_t obj;

    obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {
        return (false);
    }

    dn_pas_obj_key_entity_set(obj,
                              cps_api_qualifier_OBSERVED,
                              true,
                              rec->entity_type,
                              true,
                              rec->slot
                              );

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_ENTITY_PRESENT,
                               rec->present
                               );
    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_ENTITY_OPER_STATUS,
                               rec->oper_fault_state->oper_status
                               );
    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_ENTITY_FAULT_TYPE,
                               rec->oper_fault_state->fault_type
                               );

    if (rec->eeprom_valid) {
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_VENDOR_NAME,
                                    rec->eeprom->vendor_name
                                    );
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_PRODUCT_NAME,
                                    rec->eeprom->product_name
                                    );
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_HW_VERSION,
                                    rec->eeprom->hw_version
                                    );
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_PLATFORM_NAME,
                                    rec->eeprom->platform_name
                                    );
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_PPID,
                                    rec->eeprom->ppid
                                    );
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_PART_NUMBER,
                                    rec->eeprom->part_number
                                    );
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_SERVICE_TAG,
                                    rec->eeprom->service_tag
                                    );
        char buf[15];
        dn_pas_service_tag_to_express_service_code(buf, sizeof(buf), rec->eeprom->service_tag);
        cps_api_object_attr_add_str(obj,
                                    BASE_PAS_ENTITY_SERVICE_CODE,
                                    buf
                                    );
    }

    return (dn_pas_cps_notify(obj));
}

/* Initialize the cache entries for the given entity type */

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))

static const struct {
    uint_t entity_type, sdi_entity_type;
} entity_type_tbl[] = {
    { entity_type:     PLATFORM_ENTITY_TYPE_PSU,
      sdi_entity_type: SDI_ENTITY_PSU_TRAY
    },
    { entity_type:     PLATFORM_ENTITY_TYPE_FAN_TRAY,
      sdi_entity_type: SDI_ENTITY_FAN_TRAY
    },
    { entity_type:     PLATFORM_ENTITY_TYPE_CARD,
      sdi_entity_type: SDI_ENTITY_SYSTEM_BOARD
    }
};

bool dn_cache_init_entity(void)
{
    uint_t             i, slot, n;
    sdi_entity_hdl_t   sdi_entity_hdl;
    pas_entity_t       *rec;

    for (i = 0; i < ARRAY_SIZE(entity_type_tbl); ++i) {
        for (slot = 1,
                 n = sdi_entity_count_get(entity_type_tbl[i].sdi_entity_type);
             n;
             --n, ++slot
             ) {
            sdi_entity_hdl = sdi_entity_lookup(
                                 entity_type_tbl[i].sdi_entity_type,
                                 slot
                                               );
            if (sdi_entity_hdl == 0) {
                return (false);
            }

            rec = CALLOC_T(pas_entity_t, 1);
            if (rec == 0) {
                return (false);
            }

            rec->entity_type         = entity_type_tbl[i].entity_type;
            rec->slot                = slot;
            rec->sdi_entity_hdl      = sdi_entity_hdl;

            rec->admin_status = BASE_CMN_ADMIN_STATUS_TYPE_UP;

            dn_pas_oper_fault_state_init(rec->oper_fault_state);

            rec->power_on = true; /** \todo Do not assume entity power is on */

            char res_key[PAS_RES_KEY_SIZE];

            if (!dn_pas_res_insertc(dn_pas_res_key_entity(res_key,
                                                          sizeof(res_key),
                                                          rec->entity_type,
                                                          rec->slot
                                                          ),
                                    rec
                                    )
                ) {
                free(rec);

                return (false);
            }
        }
    }

    return (true);
}


static void _dn_cache_init_entity_res(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                                      )
{
    void (*func)(sdi_resource_hdl_t, void *);

    switch (sdi_resource_type_get(sdi_resource_hdl)) {
    case SDI_RESOURCE_FAN:
        func = dn_cache_init_fan;
        break;

    case SDI_RESOURCE_TEMPERATURE:
        func = dn_cache_init_temp_sensor;
        break;

    case SDI_RESOURCE_LED:
        func = dn_cache_init_led;
        break;

    case SDI_RESOURCE_DIGIT_DISPLAY_LED:
        func = dn_cache_init_disp;
        break;

    case SDI_RESOURCE_POWER_MONITOR:
        func = dn_cache_init_power_monitor;
        break;

    default:
        return;
    }

    (*func)(sdi_resource_hdl, data);
}

static void dn_cache_init_entity_res(pas_entity_t *rec)
{
    switch (rec->entity_type) {
        case PLATFORM_ENTITY_TYPE_CARD:

                rec->num_leds
                    = rec->num_displays
                    = rec->num_temp_sensors
                    = rec->num_plds
                    = rec->num_power_monitors
                    = 0;
                break;

        case PLATFORM_ENTITY_TYPE_PSU:
        case PLATFORM_ENTITY_TYPE_FAN_TRAY:

                rec->num_fans = 0;
                rec->num_leds = 0;
                break;

        default:
                return;
    }

    sdi_entity_for_each_resource(rec->sdi_entity_hdl,
                                 _dn_cache_init_entity_res,
                                 rec
                                 );

    /* Initialize NPU temperature sensor in the cache DB */
    if(rec->entity_type == PLATFORM_ENTITY_TYPE_CARD){

        dn_cache_init_remote_temp_sensor();
    }
}

static void dn_cache_del_entity_res(pas_entity_t *rec)
{
    switch (rec->entity_type) {
        case PLATFORM_ENTITY_TYPE_CARD:

                dn_cache_del_temp_sensor(rec);
                dn_cache_del_led(rec);
                dn_cache_del_disp(rec);
                dn_cache_del_power_monitor(rec);

                rec->num_leds
                    = rec->num_displays
                    = rec->num_temp_sensors
                    = rec->num_plds
                    = rec->num_power_monitors
                    = 0;
                break;

        case PLATFORM_ENTITY_TYPE_PSU:
        case PLATFORM_ENTITY_TYPE_FAN_TRAY:

                dn_cache_del_fan(rec);
                dn_cache_del_led(rec);

                rec->num_fans = rec->num_leds = 0;
                break;

        default: ;

    }
}

pas_entity_t *dn_pas_entity_rec_get(uint_t entity_type, uint_t slot)
{
    char res_key[PAS_RES_KEY_SIZE];

    return ((pas_entity_t *) dn_pas_res_getc(dn_pas_res_key_entity(res_key,
                                                                   sizeof(res_key),
                                                                   entity_type,
                                                                   slot
                                                                   )
                                             )
            );
}

static void dn_entity_fans_poll(
    pas_entity_t *parent,
    bool         update_allf,
    bool         *notif
                                )
{
    uint_t        fan_idx;
    pas_fan_t     *rec;

    for (fan_idx = 1; fan_idx <= parent->num_fans; ++fan_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        rec = (pas_fan_t *) dn_pas_res_getc(dn_pas_res_key_fan(res_key,
                                                               sizeof(res_key),
                                                               parent->entity_type,
                                                               parent->slot,
                                                               fan_idx
                                                               )
                                            );
        if (rec == 0)  break;

        dn_fan_poll(rec, update_allf, notif);
    }
}

static void dn_entity_temp_sensors_poll(
    pas_entity_t *parent,
    bool         update_allf,
    bool         *notif
                                )
{
    uint_t                   sensor_idx;
    pas_temperature_sensor_t *rec;

    for (sensor_idx = 1; sensor_idx <= parent->num_temp_sensors; ++sensor_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        rec = (pas_temperature_sensor_t *)
            dn_pas_res_getc(dn_pas_res_key_temp_sensor_idx(res_key,
                                                           sizeof(res_key),
                                                           parent->entity_type,
                                                           parent->slot,
                                                           sensor_idx
                                                           )
                            );
        if (rec == 0)  break;

        dn_temp_sensor_poll(rec, update_allf, notif);
    }
}

static void dn_entity_power_monitors_poll(pas_entity_t *parent)
{
    uint_t                   pm_idx;
    pas_power_monitor_t     *res_rec;

    for (pm_idx = 1; pm_idx <= parent->num_power_monitors; ++pm_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        res_rec = (pas_power_monitor_t *) dn_pas_res_getc(dn_pas_res_key_pm(res_key,
                                                         sizeof(res_key),
                                                         parent->entity_type,
                                                         parent->slot,
                                                         pm_idx));
        if (res_rec == 0)  break;

        dn_power_monitor_poll(res_rec);
    }
}

static void dn_entity_res_poll(
    pas_entity_t *rec,
    bool         update_allf,
    bool         *notif
                               )
{
    dn_entity_fans_poll(rec, update_allf, notif);
    dn_entity_temp_sensors_poll(rec, update_allf, notif);
    dn_entity_power_monitors_poll(rec);
}

static void dn_entity_fans_rec_clear(pas_entity_t *parent)
{
    uint_t        fan_idx;
    pas_fan_t     *rec;

    for (fan_idx = 1; fan_idx <= parent->num_fans; ++fan_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        rec = (pas_fan_t *) dn_pas_res_getc(dn_pas_res_key_fan(res_key,
                                                               sizeof(res_key),
                                                               parent->entity_type,
                                                               parent->slot,
                                                               fan_idx
                                                               )
                                            );
        if (rec == 0)  break;

        rec->targ_speed = rec->max_speed;

    }
}

/*
 * clear resource records of the entity
 */
static void dn_entity_res_clear(pas_entity_t *rec)
{

    dn_entity_fans_rec_clear(rec);
}

/* Set the operational status of an entity */

bool dn_pas_entity_fault_state_set(pas_entity_t *rec, uint_t fault_state)
{
    return (dn_pas_oper_fault_state_update(rec->oper_fault_state, fault_state));
}


/* Poll the given entity */

bool dn_entity_poll(pas_entity_t *rec, bool update_allf)
{
    enum { ENTITY_INIT_MAX_TRIES = 3 };
    enum { FAULT_LIMIT = 3 };

    bool              present, notif = false, power_status, fault_status;
    sdi_entity_info_t entity_info[1];
    pas_entity_t     *parent = NULL;

    pas_oper_fault_state_t prev_oper_fault_state[1];
    *prev_oper_fault_state = *rec->oper_fault_state;
    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    /* Check entity presence */

    if (STD_IS_ERR(sdi_entity_presence_get(rec->sdi_entity_hdl, &present))) {
        return (false);
    }

    if (!rec->valid || present != rec->present) {
        /* First poll, or presence changed => Generate a notification */

        notif = true;

        /* generate syslog for present status */
        PAS_NOTICE("%s %d %s\n",
                   rec->entity_type == PLATFORM_ENTITY_TYPE_PSU ? "PSU" :
                   rec->entity_type == PLATFORM_ENTITY_TYPE_FAN_TRAY ? "Fan Tray" :
                   rec->entity_type == PLATFORM_ENTITY_TYPE_CARD ? "Card":"Unknown Type",
                   rec->slot, present ? "is present" : "is removed"
                   );
    }
    if ((!rec->valid || !rec->eeprom_valid || !rec->present) && present) {
        /* Transitioned to present */

        update_allf = true;     /* Update whole cache record */

        /* Record insertion */

        ++rec->insertion_cnt;
        time(&rec->insertion_timestamp);

        rec->power_status  = true;
        rec->init_ok       = false;
        rec->init_fail_cnt = 0;
        rec->fault_cnt     = 0;
    }
    if (rec->valid && rec->present && !present) {
        /* Transitioned to absent */

        rec->sdi_entity_info_hdl = 0;
        rec->eeprom_valid = false;

        /* Delete cache records for resources */

        dn_cache_del_entity_res(rec);
    }

    rec->present = present;
    rec->valid   = true;

    if (rec->present) {
        /* check PSU power status handling
           two cases:
           1) When system boots up, PSU is inserted but without power cable
           2) PSU is inserted and power on and than remove the power cable

           Send the notification and syslog if PSU power status is changed
           from power failure to normal
        */

        if (rec->entity_type == PLATFORM_ENTITY_TYPE_PSU) {
            if (STD_IS_ERR(sdi_entity_psu_output_power_status_get(
                               rec->sdi_entity_hdl,
                               &power_status
                                                                  )
                           )
                ) {
                dn_pas_entity_fault_state_set(rec,
                                              PLATFORM_FAULT_TYPE_ECOMM
                                              );
                power_status = false;
            } else if (!power_status) {
                dn_pas_entity_fault_state_set(rec,
                                              PLATFORM_FAULT_TYPE_EPOWER
                                              );
            }

            /* Power was off, now on => Trigger complete update */
            if (power_status && !rec->power_status)  update_allf = true;

            if (!(rec->power_status = power_status)) {
                rec->init_ok       = false;
                rec->init_fail_cnt = 0;
                rec->eeprom_valid = false;
                dn_entity_res_clear(rec);
            }
        }

        /* initialization */
        if (rec->power_status
            && !rec->init_ok
            && rec->init_fail_cnt < ENTITY_INIT_MAX_TRIES
            ) {
            if (STD_IS_ERR(sdi_entity_init(rec->sdi_entity_hdl))) {
                if (++rec->init_fail_cnt >= ENTITY_INIT_MAX_TRIES) {
                    dn_pas_entity_fault_state_set(rec,
                                                  PLATFORM_FAULT_TYPE_ECOMM
                                                  );
                }
            } else {
                rec->init_ok = true;

                rec->sdi_entity_info_hdl = dn_pas_entity_info_hdl(rec->sdi_entity_hdl);
                if (rec->sdi_entity_info_hdl == 0) {
                    return (false);
                }

                /* Initialize cache records for resources */

                dn_cache_init_entity_res(rec);
            }
        }

        if (rec->init_ok) {
            if (update_allf) {
                /* Update constant fields */

                STRLCPY(rec->name,
                        sdi_entity_name_get(rec->sdi_entity_hdl)
                        );

                if (STD_IS_ERR(sdi_entity_info_read(rec->sdi_entity_info_hdl,
                                                    entity_info
                                                    )
                               )
                    ) {

                    dn_pas_entity_fault_state_set(rec,
                                                  PLATFORM_FAULT_TYPE_ECOMM
                                                  );
                    rec->eeprom_valid = false;
                } else {
                    STRLCPY(rec->eeprom->vendor_name,   entity_info->vendor_name);
                    STRLCPY(rec->eeprom->product_name,  entity_info->prod_name);
                    STRLCPY(rec->eeprom->part_number,   entity_info->part_number);
                    STRLCPY(rec->eeprom->hw_version,    entity_info->hw_revision);
                    STRLCPY(rec->eeprom->platform_name, entity_info->platform_name);
                    STRLCPY(rec->eeprom->ppid,          entity_info->ppid);
                    STRLCPY(rec->eeprom->service_tag,   entity_info->service_tag);

                    if (rec->eeprom_valid != true) {
                        notif = true;
                        rec->eeprom_valid = true;
                    }
                }

                if (rec->oper_fault_state->oper_status
                    != prev_oper_fault_state->oper_status
                    ) {
                    /* Operational status changed => Generate notification */

                    notif = true;

                    *prev_oper_fault_state = *rec->oper_fault_state;
                }
            }

            if (notif)  dn_entity_notify(rec);

            notif = false;

            switch (rec->entity_type) {
            case PLATFORM_ENTITY_TYPE_PSU:
                {
                    char      res_key[PAS_RES_KEY_SIZE];
                    pas_psu_t *psu_rec;

                    psu_rec = (pas_psu_t *)
                        dn_pas_res_getc(dn_pas_res_key_psu(res_key,
                                                           sizeof(res_key),
                                                           rec->slot
                                                           )
                                        );
                    if (psu_rec != 0) {
                        dn_psu_poll(psu_rec, update_allf);
                        parent = psu_rec->parent;
                    }
                }
                break;

            case PLATFORM_ENTITY_TYPE_FAN_TRAY:
                {
                    char           res_key[PAS_RES_KEY_SIZE];
                    pas_fan_tray_t *fan_tray_rec;

                    fan_tray_rec = (pas_fan_tray_t *)
                        dn_pas_res_getc(dn_pas_res_key_fan_tray(res_key,
                                                                sizeof(res_key),
                                                                rec->slot
                                                                )
                                        );
                    if (fan_tray_rec != 0) {
                        dn_fan_tray_poll(fan_tray_rec, update_allf);
                        parent = fan_tray_rec->parent;
                    }
                }
                break;

            default:
                ;
            }

            if (parent != NULL) {
                if (STD_IS_ERR(sdi_entity_fault_status_get(parent->sdi_entity_hdl,
                                                           &fault_status
                                                           )
                               )
                    ) {
                    dn_pas_entity_fault_state_set(parent,
                                                  PLATFORM_FAULT_TYPE_ECOMM
                                                  );
                } else if (fault_status) {
                    if (parent->fault_cnt < FAULT_LIMIT)  ++parent->fault_cnt;
                    if (parent->fault_cnt == FAULT_LIMIT) {
                            /* only report a log error once on fault */
                            PAS_WARN("Fault count (%d) exceeded for %s %d\n",
                                    parent->fault_cnt,
                                    rec->entity_type == PLATFORM_ENTITY_TYPE_PSU ? "PSU" :
                                    rec->entity_type == PLATFORM_ENTITY_TYPE_FAN_TRAY ? "Fan Tray" : "Other",
                                    rec->slot);

                            /* increment count to avoid futher logs (until cleared) */
                        ++parent->fault_cnt;
                    }
                    if ((parent->fault_cnt >= FAULT_LIMIT) && (rec->entity_type == PLATFORM_ENTITY_TYPE_FAN_TRAY)) {
                        dn_pas_entity_fault_state_set(parent,
                                PLATFORM_FAULT_TYPE_EHW );
                    } else {
                        *rec->oper_fault_state = *prev_oper_fault_state;
                    }
                } else {
                    parent->fault_cnt = 0;
                }
            }

            dn_entity_res_poll(rec, update_allf, &notif);
        }
    }

    if (rec->oper_fault_state->oper_status
        != prev_oper_fault_state->oper_status
        ) {
        /* Operational status changed => Generate notification */

        notif = true;
    }

    if (notif)  dn_entity_notify(rec);

    return (true);
}

/* Function to poll lpc bus on system board entity */

bool dn_entity_lpc_bus_poll(pas_entity_t *rec, bool *lpc_test_status)
{

    if (STD_IS_ERR(sdi_entity_lpc_bus_check(rec->sdi_entity_hdl,lpc_test_status))){
        return (false);
    }

    return true;
}

