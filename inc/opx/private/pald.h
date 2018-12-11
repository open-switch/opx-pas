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

/**
 * filename: pald.h
 *
 */

#ifndef __PALD__H
#define __PALD__H

#include "std_error_codes.h"
#include "cps_api_operation.h"

void dn_pas_lock(void);
void dn_pas_unlock(void);
t_std_error dn_pas_timedlock(void);

/* Return pald program name (argv[0]) */
char *dn_pald_progname_get(void);

/* Return path where FUSE fs to be mounted */
char *dn_pald_fuse_mount_dir_get(void);

/* PAS status setter and getter functions */
bool dn_pald_status_get(void);

/* Diagnostic mode setter and getter functions */
bool dn_pald_diag_mode_get(void);
void dn_pald_diag_mode_set(bool state);

t_std_error dn_pald_reinit(void);

t_std_error dn_pas_main_thread(void);
t_std_error dn_pas_monitor_thread(void);
t_std_error dn_pas_remote_poller_thread(void);
t_std_error dn_pas_fuse_handler_thread(void *arguement);

void dn_pas_status_init(cps_api_operation_handle_t cps_hdl);

/* define PALD error codes */
enum e_pald_error_codes {
    PALD_SDI_SYS_INIT_ERR   = 400,
    PALD_SET_FAN_SPEED_ERR  = 401
};

/* shared by multiple files */
enum {
    ENTITY_INSTANCE_ID_START_COUNT = 1
};

#endif
