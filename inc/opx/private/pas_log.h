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

#ifndef __PAS_LOG_H
#define __PAS_LOG_H

#include "event_log.h"

#define PAS_TRACE(_msg, ...) \
    EV_LOGGING(PAS, DEBUG, "", _msg, ## __VA_ARGS__)

#define PAS_NOTICE(_msg, ...) \
    EV_LOGGING(PAS, NOTICE, __func__, _msg, ## __VA_ARGS__)

#define PAS_WARN(_msg, ...) \
    EV_LOGGING(PAS, WARNING, __func__, _msg, ## __VA_ARGS__)

#define PAS_ERR(_msg, ...) \
    EV_LOGGING(PAS, ERR, __func__, _msg, ## __VA_ARGS__)

#endif /* !defined(__PAS_LOG_H) */
