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

/***************************************************************************
 *
 * Implemenation of PAS shared lib, per API defined in dn_pas.h
 */

#include "dn_platform_utils.h"
#include "std_type_defs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#define PATH_UNIT_INFO       "/etc/unit_info"
#define DEFAULT_UNIT_ID     (1)
#define BUFF_SIZE           (64)


bool dn_platform_unit_id_get(uint_t *unit)
{
    uint_t unit_id        = DEFAULT_UNIT_ID;
    FILE *fp              = NULL;
    char  buff[BUFF_SIZE] = "";
    int   fd              = -1;

    if ((fp = fopen(PATH_UNIT_INFO, "r")) != NULL) {

        if (((fd = fileno(fp)) != -1) && flock(fd, LOCK_SH) == 0) {

            if (fgets(buff, BUFF_SIZE, fp) != NULL) {

                unit_id = strtol(buff, NULL, 10);
            }
            flock(fd, LOCK_UN);
        }

        fclose(fp);
    }

    *unit = unit_id;

    return true;
}

bool dn_platform_unit_id_set(uint_t unit)
{
    FILE *fp          = NULL;
    bool  ret         = false;
    int fd            = -1;

    if ((fp = fopen(PATH_UNIT_INFO, "w")) != NULL) {


        if (((fd = fileno(fp)) != -1) && flock(fd, LOCK_EX) == 0) {

            if (fprintf(fp, "%u", unit) > 0) {

                ret = true;
            }
            flock(fd, LOCK_UN);
        }

        fclose(fp);
    }

    return ret;
}


