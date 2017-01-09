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
 * filename: pas_monitor.h
 */ 
     
#ifndef ___PAS_MONITOR_H__
#define ___PAS_PONITOR_H__

#include "std_type_defs.h"
#include "std_event_service.h"

#include "private/pas_timer.h"
#include "private/pald.h"

enum {
    MAX_EVT_NAME_SIZE = 40      /* Size of event name buffer */
};

typedef int (*FUNCPTR) ();

typedef enum {
    MIN_EVT_ID          = 0,
    PSU                 = MIN_EVT_ID,
    FAN_TRAY,
    TEMPERATURE_SENSOR,
    MEDIA,
    MAX_EVT_ID
} EVT_ID;

typedef struct
{
    EVT_ID evt_id;          /* Event Id */
    bool   turn_on;         /* For dynamically turn on/off particular event, come from configure file */
    uint_t polling_rate;    /* in milli second */
    uint_t evt_poll_cnt;    /* To keep track of no of times the event polled */
    uint_t evt_occured_cnt; /* To keep track of no of times the event occured */
}evtList_t;

typedef struct
{
    EVT_ID     evt_id;                      /* Event Id */
    char       evt_name[MAX_EVT_NAME_SIZE]; /* To print event List for Debugging */
    timer_id_t tmr_id;                      /* timer id */ 
    FUNCPTR    routine;                     /* Function really access hw   */
}polling_list_t;

typedef struct {
    std_event_client_handle   handle;
    std_event_msg_buff_t      buff;
} cps_api_to_std_event_map_t;


/* define PAS Monitor error codes */
enum e_pas_monitor_error_codes {
    PAS_MONITOR_ERR_TIMER_CREAT       = 1,
    PAS_MONITOR_ERR_PUBLISH           = 2,
    PAS_MONITOR_ERR_SET_TIMER         = 3,
    PAS_MONITOR_ERR_CPS_EVT_SERV_INIT = 4,
    PAS_MONITOR_ERR_CPS_EVT_CLNT_CONN = 5,
    PAS_MONITOR_ERR_OBJECT_CREATE     = 6,
};

/* poling routines */
t_std_error dn_poll_psu(void *);
t_std_error dn_poll_fan_tray(void *);
t_std_error dn_poll_temperature_sensor(void *);

#endif
