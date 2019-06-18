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
 * filename: pald.c
 */

#include "private/pald.h"
#include "private/pas_log.h"
#include "private/pas_config.h"
#include "private/pas_chassis.h"
#include "private/pas_entity.h"
#include "private/pas_psu.h"
#include "private/pas_fan_tray.h"
#include "private/pas_card.h"
#include "private/pas_fan.h"
#include "private/pas_event.h"
#include "private/pas_media.h"
#include "private/pas_utils.h"
#include "private/pas_data_store.h"
#include "private/pas_comm_dev.h"
#include "private/pas_job_queue.h"

#include "std_thread_tools.h"
#include "private/dn_pas.h"
#include "std_mutex_lock.h"
#include "dell-base-platform-common.h"

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>             /* signal() */
#include <systemd/sd-daemon.h>  /* sd_notify() */
#include <stdlib.h>             /* exit(), EXIT_SUCCESS */
#include <stdbool.h>            /* bool, true, false */

enum {
    TERM_SIG = SIGTERM
};

std_mutex_lock_create_static_init_fast(pas_lock);

typedef struct pald_thread_functions {
    const char  *name;
    t_std_error (*main_thread)(void);
    t_std_error (*main_thread_param)(void *arg);
} t_pald_thread_list;

static t_pald_thread_list thread_main_functions [] = {
    //pas main thread
    { "pas_main_thread", dn_pas_main_thread, NULL },

    //pas monitor main thread
    { "pas_monitor_thread", dn_pas_monitor_thread, NULL },

    //pas remote poller thread
    { "pas_remote_poller_thread", dn_pas_remote_poller_thread, NULL },

    //pas FUSE handler main thread
    { "pas_fuse_handler_thread", NULL, dn_pas_fuse_handler_thread },

    //pas job queue thread 
    {"pas_queue_job_thread", pas_job_q_thread, NULL}
};


static const size_t total_thread_num
    = sizeof(thread_main_functions)/sizeof(*thread_main_functions);

static cps_api_operation_handle_t cps_hdl;
static char *progname, *config_filename, *fuse_mount_dir;
static bool pas_status, diag_mode;
static char PAS_CONFIG_FILENAME_DFLT[] = "/etc/opx/pas/config.xml";
static char PAS_FUSE_MOUNT_DIR_DFLT[]  = "/mnt/fuse";
volatile static bool shutdwn = false;

/************************************************************************
 *
 * Name: sigterm_hdlr
 *
 *      This function is to handle the SIGTERM signal
 *
 * Input: Integer signo
 *
 * Return Values: None
 * -------------
 *
 ************************************************************************/
static void sigterm_hdlr(int signo)
{
    /* Avoid system calls at all cost */
    shutdwn = true;
}

/************************************************************************
 *
 * Name: dn_pald_status
 *
 *      This function is to notify that PAS is up and running
 *
 * Input: bool status
 *
 * Return Values: None
 * -------------
 *
 ************************************************************************/
static bool dn_pald_status(bool status)
{
    cps_api_object_t obj;
    uint_t           slot = 0;

    obj = cps_api_object_create();
    if (obj == CPS_API_OBJECT_NULL) {
        return false;
    }

    dn_pas_myslot_get(&slot);

    dn_pas_obj_key_pas_status_set(obj,
                              cps_api_qualifier_OBSERVED,
                              true,
                              slot
                             );

    cps_api_object_attr_add_u8(obj,
                               BASE_PAS_READY_STATUS,
                               status
                               );

    return (dn_pas_cps_notify(obj));
}

/************************************************************************
 *
 * Name: dn_pald_cleanup_previous_running_thread
 *
 *      This function is to cleanup running thread(s)
 *
 * Input: None
 *
 * Return Values: None
 * -------------
 *
 ************************************************************************/
void dn_pald_cleanup_previous_running_thread(
         size_t thread_idx,
         std_thread_create_param_t *pas_thread_entry
                                            )
{
    size_t running_thread_idx  = 0;

    /* kill all previous running threads if have */
    for (running_thread_idx=0;
        running_thread_idx < thread_idx;
        running_thread_idx++
        ) {
       pthread_kill(*(pthread_t *)pas_thread_entry[running_thread_idx].thread_id,
                    TERM_SIG
                    );
       std_thread_destroy_struct(&pas_thread_entry[running_thread_idx]);
    }
}

/************************************************************************
 *
 * Name: dn_pas_lock
 *
 *      This function is to lock a mutex
 *
 * Input: None
 *
 * Return Values: None
 * -------------
 *
 ************************************************************************/
void dn_pas_lock(void)
{
    std_mutex_lock(&pas_lock);
}

/************************************************************************
 *
 * Name: dn_pas_unlock
 *
 *      This function is to unlock a mutex
 *
 * Input: None
 *
 * Return Values: None
 * -------------
 *
 ************************************************************************/
void dn_pas_unlock(void)
{
    std_mutex_unlock(&pas_lock);
}

/*
 * Name: dn_pas_timedlock
 *
 *      This function is to lock a mutex with timeout of 30 seconds, if calling
 *      thread is able to acquire the mutex with in 30 seconds it returns STD_ERR_OK
 *      otherwise error.
 * Input: None
 * Return Values: On success STD_ERR_OK, otherwise ERROR.
 */

t_std_error dn_pas_timedlock(void)
{
    struct timespec timeout;
    t_std_error     ret = STD_ERR_OK;

    memset(&timeout, 0, sizeof(timeout));
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 30; // 30 seconds timeout

    if (pthread_mutex_timedlock(&pas_lock, &timeout) != 0) {
        ret = STD_ERR(PAS, FAIL, 0);
    }

    return ret;
}

/************************************************************************
 *
 * Name: dn_pas_config_file_handle
 *
 *      This function is to handle processing pas configureation file
 *
 * Input: None
 *
 * Return Values: error code or STD_ERR_OK
 * -------------
 *
 ************************************************************************/

static t_std_error dn_pas_config_file_handle(void)
{
    return (dn_pas_config_init(config_filename, cps_hdl)
            && dn_cache_init_chassis()
            && dn_cache_init_entity()
            && dn_cache_init_psu()
            && dn_cache_init_fan_tray()
            && dn_cache_init_card()
            && dn_pas_phy_media_init()
            && dn_pas_comm_dev_init()
            && dn_pas_cps_handler_reg(config_filename, cps_hdl)
            ? STD_ERR_OK : STD_ERR(PAS, FAIL, 0)
            );
}

/******************************************************************************
 *
 * Name: dn_pald_thread_init
 *
 *      This function is to start pas_monitor, pas and pas fuse handler threads
 *
 * Input: command line arguement(s)
 *
 * Return Values: error code or STD_ERR_OK
 * -------------
 *
 *****************************************************************************/
static t_std_error dn_pald_thread_init(void)
{
    size_t      thread_idx          = 0;
    std_thread_create_param_t pas_thread_entry[total_thread_num];

    for (; thread_idx < total_thread_num; thread_idx++) {
      if (thread_main_functions[thread_idx].main_thread != NULL) {

          std_thread_init_struct(&pas_thread_entry[thread_idx]);
          pas_thread_entry[thread_idx].name
              = thread_main_functions[thread_idx].name;
          pas_thread_entry[thread_idx].thread_function =
              (std_thread_function_t)
                  thread_main_functions[thread_idx].main_thread;
          pas_thread_entry[thread_idx].param = 0;

          if (std_thread_create(&pas_thread_entry[thread_idx])!=STD_ERR_OK) {
              PAS_ERR("Failed to create thread %s",
                      pas_thread_entry[thread_idx].name
                      );

              dn_pald_cleanup_previous_running_thread(thread_idx,
                                                      pas_thread_entry
                                                      );
              return STD_ERR(PAS,FAIL,0);
          }
      }

      if (thread_main_functions[thread_idx].main_thread_param != NULL) {

          std_thread_init_struct(&pas_thread_entry[thread_idx]);
          pas_thread_entry[thread_idx].name
              = thread_main_functions[thread_idx].name;
          pas_thread_entry[thread_idx].thread_function =
              (std_thread_function_t)
                  thread_main_functions[thread_idx].main_thread_param;

          pas_thread_entry[thread_idx].param = 0;

          if (std_thread_create(&pas_thread_entry[thread_idx])!=STD_ERR_OK) {
              PAS_ERR("Failed to create thread %s",
                      pas_thread_entry[thread_idx].name
                      );

              dn_pald_cleanup_previous_running_thread(thread_idx,
                                                      pas_thread_entry
                                                      );
              return STD_ERR(PAS,FAIL,0);
          }
      }
    }
    return STD_ERR_OK;
}

/****************************************************************
 *
 * Name: dn_pald_init
 *
 *      This function is to start pald initialization routines
 *
 * Input: command line arguement(s)
 *
 * Return Values: error code or STD_ERR_OK
 * -------------
 *
 ***************************************************************/

bool dn_pald_status_get(void)
{
    return pas_status;
}

bool dn_pald_diag_mode_get(void)
{
    return diag_mode;
}

void dn_pald_diag_mode_set(bool state)
{
    diag_mode = state;
}

char *dn_pald_progname_get(void)
{
    return (progname);
}

char *dn_pald_fuse_mount_dir_get(void)
{
    return (fuse_mount_dir);
}


static t_std_error dn_pald_init(int argc, char *argv[])
{
    t_std_error ret = STD_ERR_OK;

    /* Parse and store command line args */

    progname        = argv[0];
    config_filename = PAS_CONFIG_FILENAME_DFLT;
    fuse_mount_dir  = PAS_FUSE_MOUNT_DIR_DFLT;

    for (++argv, --argc; argc > 0; ) {
        if (strcmp(*argv, "-f") == 0) {
            if (argc < 2)  break;

            config_filename = argv[1];

            argc -= 2;  argv += 2;

            continue;
        }
        if (strcmp(*argv, "-m") == 0) {
            if (argc < 2)  break;

            fuse_mount_dir = argv[1];

            argc -= 2;  argv += 2;

            continue;
        }

        break;
    }
    if (argc > 0) {
        fprintf(stderr,
                "usage: %s [ -f <config-filename> ] [ -m <fuse-mount-dir> ]\n",
                progname
                );

        exit(1);
    }

    uint_t       sdi_init_try_count  = 0;
    const uint_t sdi_init_max_try    = 5;
    const uint_t sdi_init_pause_time = 500000;

    while (sdi_sys_init() != STD_ERR_OK && sdi_init_try_count < sdi_init_max_try)
    {
        usleep(sdi_init_pause_time);
        ++sdi_init_try_count;
    }

    if(sdi_init_try_count >= sdi_init_max_try) {
        PAS_ERR("Failed to initialize SDI");

        return STD_ERR(PAS, FAIL, 0);
    }

    if (cps_api_operation_subsystem_init(&cps_hdl, 1)!= cps_api_ret_code_OK) {
        PAS_ERR("Failed to initalize CPS API");

        return STD_ERR(PAS, FAIL, 0);
    }

    do {
       /* Initialize event subsystem */

       if (! dn_pas_cps_ev_init()) {
          ret = STD_ERR(PAS,FAIL,0);
          break;
       }


       // read pas config file and put read data into internal data structure
       ret = dn_pas_config_file_handle();
       if (STD_IS_ERR(ret)) {
           PAS_ERR("Failed to parse config file");

           break;
       }

       std_mutex_lock_init_recursive(&pas_lock);

       /* init threads */
       ret = dn_pald_thread_init();
       if (STD_IS_ERR(ret)) {
           PAS_ERR("Failed to initialize threads");

            std_mutex_destroy(&pas_lock);
            break;
       }
    } while (0);

    if (STD_IS_ERR(ret)) {
       return ret;
    }

    return STD_ERR_OK;
}

t_std_error dn_pald_reinit()
{
    /** TODO */

    /** Delete all the data from the internal datastore */

    /** Sdi initialization */

    /** Re-create a datastore for PAS */

    return STD_ERR_OK;
}

/*******************************************************************************
 *
 * Name: main
 *
 *      pald main routine
 *
 * Input: command line arguement(s)
 *
 * Return Values: error code or STD_ERR_OK
 * -------------
 *
 *******************************************************************************/
t_std_error main(int argc, char *argv[])
{
    PAS_NOTICE("Starting");
    dn_pas_debug_log("PAS_DAEMON", "PAS starting");

    // signal must install before service init
    (void)signal(SIGTERM, sigterm_hdlr);

    if (dn_pald_init(argc, argv) != STD_ERR_OK) {
        return STD_ERR(PAS,FAIL,0);
    }

    sd_notify(0,"READY=1");
    dn_pas_status_init(cps_hdl);
    pas_status = true;
    dn_pald_status(pas_status);

    while(!shutdwn)
    {
        pause();
    }

    /* Let systemd know we got the shutdwn request
     * and that we're in the process of shutting down */
    sd_notify(0, "STOPPING=1");

    PAS_NOTICE("Exiting");
    exit(EXIT_SUCCESS);
}
