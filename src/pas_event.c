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

#include "private/pas_log.h"
#include "private/pas_event.h"

#include "cps_api_key.h"
#include "cps_api_operation.h"
#include "cps_api_events.h"

static cps_api_event_service_handle_t handle;

/* Initialize CPS event subsystem */

bool dn_pas_cps_ev_init(void)
{
    if (cps_api_event_service_init() != cps_api_ret_code_OK) {
        PAS_ERR("Failed to init CPS API event service");

        return (false);
    }

    if (cps_api_event_client_connect(&handle) != cps_api_ret_code_OK) {
        PAS_ERR("Failed to connect to CPS API event service");

        return (false);
    }

    return (true);
}

/* Issue a CPS event */

bool dn_pas_cps_notify(cps_api_object_t obj)
{
    bool result = false;

    /* Keys for events always have OBSERVED qualifier */

    cps_api_key_set(cps_api_object_key(obj),
                    CPS_OBJ_KEY_INST_POS,
                    cps_api_qualifier_OBSERVED
                    );

    if (handle == 0) return (result);

    result = (cps_api_event_publish(handle, obj) == cps_api_ret_code_OK);

    cps_api_object_delete(obj);

    return (result);
}

/*
 * Issue a CPS event with specified qualifier 
 */

bool dn_pas_cps_notify_qual(cps_api_object_t obj,cps_api_qualifier_t qual)
{
    bool result = false;

    /* Keys for events always have OBSERVED qualifier */
    cps_api_key_set(cps_api_object_key(obj),
                    CPS_OBJ_KEY_INST_POS, qual);

    if (handle == 0) return (result);

    result = (cps_api_event_publish(handle, obj) == cps_api_ret_code_OK);

    cps_api_object_delete(obj);

    return (result);
}

