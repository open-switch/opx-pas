/*
 * Copyright (c) 2018 Dell EMC.
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


#include "private/pas_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>


void dn_pas_debug_log (char *module, const char *format, ...)
{
    va_list argp;
    char  log[DEBUG_LOG_MAXLEN];
    char  time_str[TIME_STAMP_LEN] = "";
    static FILE *fp = NULL;
    time_t t;

    if (fp == NULL) {
        if ((fp = fopen(DEBUG_FILE_PATH, "a+")) == NULL) {
            return;
        }
    }

    time(&t);
    ctime_r(&t, time_str);
    time_str[strlen(time_str) - 1] = '\0';

    va_start(argp, format);

    snprintf(log, sizeof(log), "%s, %s, ", time_str, (module != NULL) ? module : "PAS_DEBUG");
    vsnprintf(log + strlen(log), sizeof(log) - strlen(log), format, argp);
    fprintf(fp, "%s\n", log);
    fflush(fp);

    va_end(argp);
}
