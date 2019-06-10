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
 * filename: pas_job_queue.cpp
 *
 */

#include "private/pas_log.h"
#include "private/pas_job_queue.h"
#include "std_utils.h"
#include "string.h"
#include <queue>
#include <mutex>
#include <unistd.h>

#define JOB_Q_LOCK_GUARD(jq)  std::lock_guard<std::mutex> lck(*(std::mutex *)((jq)->access_lock))
#define PAS_MEDIA_JOB_Q_NAME "PAS_MEDIA_JOB_QUEUE"

static pas_job_q_t* pas_media_job_q =  NULL;

pas_job_q_t* pas_job_q_create_q(char* name){

    auto *q =  new std::queue<pas_job_q_job_t>();
    pas_job_q_t* jq = (pas_job_q_t*)malloc(sizeof(pas_job_q_t));    
    auto mtx = new std::mutex();

    jq->queue = (void*)q;
    jq->access_lock = (void*)mtx;
    safestrncpy(jq->name, name, sizeof(jq->name));
    return jq;
}

void pas_job_q_destroy_q(pas_job_q_t* jq){

    /* Lock q first */
    /* then delete q */
    JOB_Q_LOCK_GUARD(jq);
    delete ((std::queue<pas_job_q_job_t>*)(jq->queue));
    delete (std::mutex*)(jq->access_lock);
    free((void*)jq);
}

t_std_error pas_job_q_size_get(pas_job_q_t* jq, size_t* size){

    if ((size == NULL) || (jq == NULL)){
        PAS_ERR("Invalid argument when getting job q size, q name: ", (jq==NULL)? "" : jq->name);
        return EINVAL;
    }
    *size = (size_t)(((std::queue<pas_job_q_job_t> *)(jq->queue))->size());
    return STD_ERR_OK;
}

t_std_error pas_job_q_is_empty(pas_job_q_t* jq, bool* is_empty){

    if ((is_empty == NULL) || (jq == NULL)){
        PAS_ERR("Invalid argument when checking if empty q with name: ", (jq==NULL)? "" : jq->name);
        return EINVAL;
    }

    size_t size = 0;
    t_std_error rc = STD_ERR_OK;

    rc = pas_job_q_size_get(jq, &size);
    if (rc == STD_ERR_OK){
        *is_empty = (size == 0);
    }
    return rc;
}

t_std_error pas_job_q_push_job(pas_job_q_t* jq, pas_job_q_job_t *job){

    if ((jq == NULL) || (job == NULL)){
        PAS_ERR("Null argument when adding job to q ");
        return EINVAL;
    }

    PAS_TRACE("New job \"%s\" pushed to queue \"%s\"", job->name, jq->name);

    if (job->job_type != PAS_JOB_TYPE_BLOCKING){
        return EOPNOTSUPP;
    }
    ((std::queue<pas_job_q_job_t>*)(jq->queue))->push(*job);

    return STD_ERR_OK;
}

static pas_job_q_job_t* pas_job_q_peek_front(pas_job_q_t* jq){

    bool empty = false;

    if (pas_job_q_is_empty(jq, &empty) != STD_ERR_OK){
        return NULL;
    }
    if (empty){
        return NULL;
    }
    JOB_Q_LOCK_GUARD(jq);

    /* front() returns ref to job */
    return &(((std::queue<pas_job_q_job_t>*)(jq->queue))->front());
}

static void pas_job_q_pop(pas_job_q_t* jq){
    bool empty = false;

    if (pas_job_q_is_empty(jq, &empty) != STD_ERR_OK){
        return;
    }
    if (empty){
        return;
    }
    JOB_Q_LOCK_GUARD(jq);
    ((std::queue<pas_job_q_job_t>*)(jq->queue))->pop();
}
/* Not thread safe */
static void pas_job_q_run_jobs(pas_job_q_t* jq){
    bool empty = true;
    pas_job_q_job_t *job = NULL;
    t_std_error rc = STD_ERR_OK;

    if (jq == NULL){
        PAS_ERR("Null job queue");
        return;
    }

    PAS_TRACE("Starting job queue processing of queue: %s", jq->name);
    while(true){
        while(empty){
            usleep(1000*1000);
            if (pas_job_q_is_empty(jq, &empty) != STD_ERR_OK){
                empty = true;
                continue;;
            }
            if (!empty){
                break;;
            }
        }
        empty = true;
        job = pas_job_q_peek_front(jq);
        if (job == NULL){
            continue;
        }
        {
            /* only guard this scope */
            JOB_Q_LOCK_GUARD(jq);
            if (job->job_type != PAS_JOB_TYPE_BLOCKING){
                PAS_ERR("Rejecting job %s", job->name);
            } else if (job->job_func != NULL){
                PAS_TRACE("Running job %s", job->name);
                rc = job->job_func(job->args);
                PAS_TRACE("Finished job %s with rc %u", job->name, rc);
            }
            /* reclaim mem alloc by caller for args */
            free(job->args);
        }
        pas_job_q_pop(jq);
    }
}

pas_job_q_t* pas_media_job_q_get(void){
    return pas_media_job_q;
}

t_std_error pas_job_q_thread(void){

    /* Currently only single queue supported */
    pas_media_job_q = pas_job_q_create_q((char*)PAS_MEDIA_JOB_Q_NAME);

    pas_job_q_run_jobs(pas_media_job_q);

    /* Cleanup */
    pas_job_q_destroy_q(pas_media_job_q);

    /* Should never return */
    PAS_ERR("Failed to start job queue thread");
    return STD_ERR(PAS,FAIL,0);
}
