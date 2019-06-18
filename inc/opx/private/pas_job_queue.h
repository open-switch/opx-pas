/*
 * Copyright (c) 2019 Dell Inc.
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
 * filename: pas_job_queue.h
 *
 */

#ifndef __PAS_JOB_QUEUE_H
#define __PAS_JOB_QUEUE_H


#include "std_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PAS_JOB_Q_JOB_NAME_LEN 100
#define PAS_JOB_Q_NAME_LEN     PAS_JOB_Q_JOB_NAME_LEN




typedef enum {
    PAS_JOB_TYPE_BLOCKING,

    /* Non blocking jobs not supported yet*/
} pas_job_q_job_type_t;

typedef struct  _pas_job_q {
    char name[PAS_JOB_Q_NAME_LEN];
    void* access_lock;  /* this is a c++ mutex */
    void* queue;  /*This is a c++ q */
}pas_job_q_t;


typedef struct _pas_job_q_job{
    char          name[PAS_JOB_Q_JOB_NAME_LEN];
    t_std_error   (* job_func)(void* args);
    void*         args;
    pas_job_q_job_type_t job_type;
} pas_job_q_job_t;


pas_job_q_t* pas_job_q_create_q(char* name);

t_std_error pas_job_q_size_get(pas_job_q_t* jq, size_t* size);

t_std_error pas_job_q_is_empty(pas_job_q_t* jq, bool* is_empty);

/* Check if tag is blocking, if not, return not supp because we do not support non-blocking jobs yet*/
t_std_error pas_job_q_push_job(pas_job_q_t* jq, pas_job_q_job_t *job);

t_std_error pas_job_q_thread(void);

pas_job_q_t* pas_media_job_q_get(void);

#ifdef __cplusplus
}
#endif

#endif
