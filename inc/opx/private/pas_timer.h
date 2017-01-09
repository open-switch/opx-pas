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
 * filename: pas_timer.h
 */ 
     
#ifndef __PAS_TIMER__H__
#define __PAS_TIMER__H__

#include "std_type_defs.h"
#include "std_error_codes.h"
#include <time.h>
#include <signal.h>


typedef enum {

/* define PAS related timers */ 
   PAS_MIN_TIMER_ID     = 0,
   PAS_PSU_TIMER        = PAS_MIN_TIMER_ID,
   PAS_FAN_TRAY_TIMER,
   PAS_TEMP_SENSOR_TIMER,
   PAS_MEDIA_TIMER,

    /* define other module timers */

    PAS_MAX_TIMER_ID

} timer_id_t;

typedef enum {
    ONETIME,
    PERIODIC
} timer_type_t;

typedef struct {
    timer_t 	 timer_id;
    timer_type_t type;	
    bool         timer_on;
    t_std_error  (*timer_func)( void *app_data );
    void         *timer_app_data;
} timer_entry_t;

void 	    dn_timer_init(size_t tmr_idx);
t_std_error dn_timer_create(size_t       tmr_idx,
                            timer_type_t type,
                            timer_t      *timer_id, 
                            t_std_error  (*timer_func)(void *app_data),
                            void         *app_data
                            );

int 	dn_timer_set(size_t tmr_idx, uint64_t mili_sec);
int 	dn_timer_cancel(size_t tmr_idx);
int 	dn_timer_delete(size_t tmr_idx);
void 	dn_timer_handle(int sig, siginfo_t *si, void *uc);
void    dn_timer_dump_timer_table(void);
timer_t *get_timer_id(size_t timer_idx);


/* define timer error codes */
typedef enum {
    TIMER_ERR_OK                      = 0,
    TIMER_ERR_INVALID_TIMER_TABLE_IDX = 300,
    TIMER_ERR_SETUP_SIGNAL_HANDLER    = 301,
    TIMER_ERR_TIMER_SET_TIME          = 302,
    TIMER_ERR_TIMER_DELETE            = 303,
} e_timer_error_codes_t;

#endif
