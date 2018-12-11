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
 * filename: pas_monitor.c
 */

#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_entity.h"
#include "private/pas_media.h"
#include "private/pas_comm_dev.h"
#include "private/pas_config.h"
#include "private/pas_utils.h"

#include "std_utils.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"
#include "private/pas_media.h"

#include  <unistd.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/time.h>
#include  <sched.h>


struct timer {
    uint_t cnt;    /* Total number of items to poll */
    uint_t period; /* Timer period, i.e. time between poll of each item, in ms */
    void   (*callback)(struct timer *);
    void   *callback_cookie;

    uint_t   cur;               /* Current item number being polled */
    uint64_t deadline;          /* Deadline for timer expiration */
};

static uint_t* pluggable_ports;

void populate_pollable_port_list(void)
{
    uint_t res = 0;
    uint_t index = dn_pas_config_media_get()->port_count;

    if (index == 0) {
        PAS_ERR("Media port count is 0. Nothing to poll");
        return;
    }
    if(index < PAS_MEDIA_START_PORT) {
        PAS_ERR("Invalid port count: %d from config. Aborting.", index);
        assert(!(index < PAS_MEDIA_START_PORT));
    }
    pluggable_ports = (uint_t*)malloc(
        (dn_pas_config_media_get()->pluggable_media_count) * sizeof(uint_t));
    if(pluggable_ports == NULL) {
        PAS_ERR("Unable to allocate memory for computing pluggable port list. Aborting.");
        assert(pluggable_ports != NULL);
    }

    for (; index > 0; index--){
        if (dn_pas_is_port_pluggable(index)) {
            pluggable_ports[res++] = index;
        }
    }
    if ( (res) != dn_pas_config_media_get()->pluggable_media_count){
        PAS_ERR("Disparity in pluggable media counts. Check config file");
    }
}

uint_t get_pollable_port(uint_t index)
{
     // Used with timer->cur, which goes from 1 to n
     return pluggable_ports[index-1];
}

/* \todo This needs to be driven from config file -- fixed config for now */

/* Poll a PSU */

static void dn_poll_psu(struct timer *tmr)
{
    pas_entity_t *rec;

    if (++tmr->cur > tmr->cnt)  tmr->cur = 1;

    rec = dn_pas_entity_rec_get(PLATFORM_ENTITY_TYPE_PSU, tmr->cur);
    if (rec == 0)  return;

    dn_pas_lock();

    if(!dn_pald_diag_mode_get()) {

        dn_entity_poll(rec, false);
    }

    dn_pas_unlock();
}

/* Poll a fan tray */

static void dn_poll_fan_tray(struct timer *tmr)
{
    pas_entity_t *rec;

    if (++tmr->cur > tmr->cnt)  tmr->cur = 1;

    rec = dn_pas_entity_rec_get(PLATFORM_ENTITY_TYPE_FAN_TRAY, tmr->cur);
    if (rec == 0)  return;

    dn_pas_lock();

    if(!dn_pald_diag_mode_get()) {

        dn_entity_poll(rec, false);
    }

    dn_pas_unlock();
}

/* Poll a card */

static void dn_poll_card(struct timer *tmr)
{
    pas_entity_t *rec;
    bool pas_led_set = false;

    if (++tmr->cur > tmr->cnt)  tmr->cur = 1;

    rec = dn_pas_entity_rec_get(PLATFORM_ENTITY_TYPE_CARD, tmr->cur);
    if (rec == 0)  return;

    dn_pas_lock();

    if(!dn_pald_diag_mode_get()) {

        dn_entity_poll(rec, false);

        dn_entity_lpc_bus_poll(rec,&pas_led_set);

    }

    dn_pas_unlock();

    if (pas_led_set) {
        // entity_type=PLATFORM_ENTITY_TYPE_CARD, slot=1,led_name= "Alarm Major",led_state =on
        if ( STD_IS_ERR(dn_pas_generic_led_set(PLATFORM_ENTITY_TYPE_CARD,1,"Alarm Major",1))) {
            PAS_ERR("Major Alarm not able to set");
        }
    }
}

/*
 * Poll a communication device (comm-dev)
 */

static void dn_poll_comm_dev (struct timer *tmr)
{
    if (++tmr->cur > tmr->cnt)  tmr->cur = 1;

    dn_pas_lock();

    if(!dn_pald_diag_mode_get()) {
        dn_comm_dev_poll();
    }

    dn_pas_unlock();
}

static void dn_poll_media(struct timer *tmr)
{
    phy_media_tbl_t      *mtbl = NULL;

    if (++tmr->cur > tmr->cnt)  tmr->cur = 1;

    dn_pas_lock();

    if(!dn_pald_diag_mode_get()) {

        dn_pas_phy_media_poll(get_pollable_port(tmr->cur), true);

        mtbl = dn_phy_media_entry_get((tmr->cur));

        if ((mtbl != NULL) && (mtbl->res_data != NULL)
                && !mtbl->res_data->valid) {

            mtbl->res_data->valid = true;
        }
    }

    dn_pas_unlock();
}


enum { MAX_TIMERS = 4 };

static struct timer timers[MAX_TIMERS];

static uint_t num_timers;

#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))

static struct timer *timerq[ARRAY_SIZE(timers)];

static uint64_t timer_now_ms(void)
{
    struct timespec ts[1];
    clock_gettime(CLOCK_MONOTONIC_RAW, ts);
    return ((uint64_t) ts->tv_sec * 1000 + ts->tv_nsec / 1000000);
}

/* Initialize the timer queue */

struct {
    uint_t entity_type;
    void   (*poll_func)(struct timer *tmr);
} tmr_init_tbl[] = {
    { PLATFORM_ENTITY_TYPE_PSU,      dn_poll_psu },
    { PLATFORM_ENTITY_TYPE_FAN_TRAY, dn_poll_fan_tray },
    { PLATFORM_ENTITY_TYPE_CARD,     dn_poll_card },
};

static t_std_error timerq_init(void)
{
    uint_t n, i;

    num_timers = 0;

    /* Set entity polling */

    for (i = 0; i < ARRAY_SIZE(tmr_init_tbl); ++i) {
        struct pas_config_entity *e;

        e = dn_pas_config_entity_get_type(tmr_init_tbl[i].entity_type);
        if (e == 0 || e->num_slots == 0)  continue;

        timers[num_timers].cnt      = e->num_slots;
        timers[num_timers].period   = e->poll_interval / e->num_slots;
        timers[num_timers].callback = tmr_init_tbl[i].poll_func;

        ++num_timers;
    }

    /* Add media polling, if applicable
       Poll only  pluggable media, so use pluggable count for timer period */
    n = dn_pas_config_media_get()->pluggable_media_count;
    populate_pollable_port_list();
    if (( dn_pas_config_media_get()->port_count > 0) &&  (n > 0)) {
        timers[num_timers].cnt = n;
        timers[num_timers].period =
            dn_pas_config_media_get()->poll_interval / n;
        timers[num_timers].callback = dn_poll_media;

        ++num_timers;
    }

    /*
     * Add communication device polling, if applicable
     */

    n = sdi_entity_resource_count_get(
            sdi_entity_lookup(SDI_ENTITY_SYSTEM_BOARD, 1),
            SDI_RESOURCE_COMM_DEV);

    if (n > 0) {
        timers[num_timers].cnt = n;
        timers[num_timers].period =
            dn_pas_config_comm_dev_get()->poll_interval;
        timers[num_timers].callback = dn_poll_comm_dev;
        num_timers = num_timers +1 ;
    }

    /* Set initial deadlines */

    for (i = 0; i < num_timers; ++i) {
        (timerq[i] = &timers[i])->deadline = timer_now_ms() + 97;
    }

    if (num_timers == 0) return STD_ERR(PAS, FAIL, 0);
    return STD_ERR_OK;
}

/* Move the timer at the head of the queue to its proper place */

static void timerq_shuffle(void)
{
    uint_t       i, j;
    struct timer *temp;

    for (i = 0; ; i = j) {
        j = i + 1;
        if (j >= num_timers)  break;

        if (timerq[i]->deadline < timerq[j]->deadline)  break;

        temp = timerq[i];
        timerq[i] = timerq[j];
        timerq[j] = temp;
    }
}

/* Sleep for the interval for the next timer, call its
   callback, ad infinitum
 */

t_std_error dn_pas_monitor_thread(void)
{
    struct timer *cur;
    uint64_t     now;

    if(STD_ERR_OK != timerq_init()) return STD_ERR(PAS, FAIL, 0);

    for (;;) {
        cur = timerq[0];

        now = timer_now_ms();

        if (cur->deadline > now)  usleep(1000 * (cur->deadline - now));

        (*cur->callback)(cur);

        cur->deadline += cur->period;

        timerq_shuffle();
        sched_yield();
    }

    return (STD_ERR_OK);        /* Should never return */
}
