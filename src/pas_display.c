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

#include "private/pas_display.h"
#include "private/pas_entity.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_utils.h"
#include "private/pas_data_store.h"
#include "private/dn_pas.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "cps_api_service.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>

#define ARRAY_SIZE(a)        (sizeof(a) / sizeof((a)[0]))
#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))


/* Create a display cache record */

void dn_cache_init_disp(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       )
{
    pas_entity_t  *parent = (pas_entity_t *) data;
    const char    *disp_name;
    pas_display_t *rec;

    ++parent->num_displays;

    rec = CALLOC_T(pas_display_t, 1);
    if (rec == 0)  return;

    disp_name = sdi_resource_alias_get(sdi_resource_hdl);

    rec->parent           = parent;
    rec->disp_idx         = parent->num_displays;
    STRLCPY(rec->name, disp_name);
    rec->sdi_resource_hdl = sdi_resource_hdl;

    dn_pas_oper_fault_state_init(rec->oper_fault_state);

    char res_key_name[PAS_RES_KEY_SIZE];

    if (!dn_pas_res_insertc(dn_pas_res_key_disp_name(res_key_name,
                                                     sizeof(res_key_name),
                                                     parent->entity_type,
                                                     parent->slot,
                                                     rec->name
                                                     ),
                            rec
                            )
        ) {
        free(rec);

        return;
    }

    char res_key_idx[PAS_RES_KEY_SIZE];

    if (!dn_pas_res_insertc(dn_pas_res_key_disp_idx(res_key_idx,
                                                    sizeof(res_key_idx),
                                                    parent->entity_type,
                                                    parent->slot,
                                                    rec->disp_idx
                                                    ),
                            rec
                            )
        ) {
        dn_pas_res_removec(res_key_name);
        free(rec);
    }
}

pas_display_t *dn_pas_disp_rec_get_name(
    uint_t entity_type,
    uint_t slot,
    char   *disp_name
)
{
    char res_key[PAS_RES_KEY_SIZE];

    return ((pas_display_t *)
            dn_pas_res_getc(dn_pas_res_key_disp_name(res_key, sizeof(res_key),
                                                     entity_type,
                                                     slot,
                                                     disp_name
                                                     )
                            )
            );
}

pas_display_t *dn_pas_disp_rec_get_idx(
    uint_t entity_type,
    uint_t slot,
    uint_t disp_idx
                                       )
{
    char res_key[PAS_RES_KEY_SIZE];

    return ((pas_display_t *)
            dn_pas_res_getc(dn_pas_res_key_disp_idx(res_key, sizeof(res_key),
                                                    entity_type,
                                                    slot,
                                                    disp_idx
                                                    )
                            )
            );
}

/* Delete a temperature sensor cache record */

void dn_cache_del_disp(pas_entity_t *parent)
{
    uint_t        disp_idx;
    pas_display_t *rec;

    for (disp_idx = 1; disp_idx <= parent->num_displays; ++disp_idx) {
        char res_key[PAS_RES_KEY_SIZE];

        rec = dn_pas_res_removec(dn_pas_res_key_disp_idx(res_key,
                                                         sizeof(res_key),
                                                         parent->entity_type,
                                                         parent->slot,
                                                         disp_idx
                                                         )
                                );

        dn_pas_res_removec(dn_pas_res_key_disp_name(res_key,
                                                    sizeof(res_key),
                                                    parent->entity_type,
                                                    parent->slot,
                                                    rec->name
                                                    )
                           );
        
        free(rec);
    }
}

