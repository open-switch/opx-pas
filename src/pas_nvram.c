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

#include "private/pas_nvram.h"
#include "private/pas_entity.h"
#include "private/pas_res_structs.h"
#include "private/pas_data_store.h"
#include "private/pas_utils.h"
#include "private/pas_data_store.h"
#include "private/pas_config.h"
#include "private/dn_pas.h"
#include "private/pas_log.h"

#include "std_type_defs.h"
#include "std_error_codes.h"
#include "sdi_entity.h"
#include "sdi_nvram.h"
#include "cps_api_service.h"
#include "dell-base-platform-common.h"
#include "dell-base-pas.h"

#include <stdlib.h>
#include <zlib.h>

#define ARRAY_SIZE(a)        (sizeof(a) / sizeof((a)[0]))
#define CALLOC_T(_type, _n)  ((_type *) calloc((_n), sizeof(_type)))

static pas_nvram_t nvram;

#define TLV_MAGIC       "TLV\0"
#define TLV_MAGIC_LEN   4
#define TLV_MAGIC_OFFS  0

#define TLV_VERSION         0x01
#define TLV_VERSION_OFFS    4
#define TLV_VERSION_LEN     1

#define TLV_COUNT_OFFS      5
#define TLV_COUNT_LEN       2

#define TLV_DATA_OFFS       7

#define TLV_CSUM_LEN        4

/* Create an NVRAM cache record */

static void _dn_nvram_free_entries(void)
{
    std_dll *entry;
    /* While not empty */
    while ((entry = std_dll_getfirst(&nvram.nvram_data)) != (std_dll *)0) {
        /* Remove entry from the list */
        std_dll_remove(&nvram.nvram_data, entry);
        /* Desroty (now dangling) entry */
        free(entry);
    }
}

/* Initialize the NVRAM cache, for calling from the CPS handlers */
void dn_nvram_init(void)
{
    if (nvram.initialized) {
        return;
    }

    /* NVRAM is always on the system board */
    sdi_entity_hdl_t *parent = sdi_entity_lookup(SDI_ENTITY_SYSTEM_BOARD, 1);

    sdi_resource_hdl_t *nvram = sdi_entity_resource_lookup(parent,
                                    SDI_RESOURCE_NVRAM, "NVRAM");
    if(nvram == NULL){
        PAS_ERR("NVRAM resource not found");
        return;
    }
    dn_cache_init_nvram(nvram, NULL);
}

void dn_cache_init_nvram(
    sdi_resource_hdl_t sdi_resource_hdl,
    void               *data
                       )
{
    pas_entity_t  *parent  = (pas_entity_t *) data;
    uint_t size;

    if (nvram.initialized) {
        return;
    }

    nvram.parent           = parent;
    nvram.sdi_resource_hdl = sdi_resource_hdl;

    std_dll_init(&nvram.nvram_data);
    nvram.initialized = true;

    if (sdi_nvram_size(sdi_resource_hdl, &size) == STD_ERR_OK) {
        nvram.nvram_size = size;
        nvram.tlv_size = size - TLV_DATA_OFFS - TLV_CSUM_LEN;
    } else {
        /* If unable to read the size, then default to a read-only NVRAM */
        nvram.nvram_size = 0;
        nvram.tlv_size = 0;
    }

    dn_nvram_parse();
}

/* Delete the NVRAM cache record, and free all TLV pairs */

void dn_cache_del_nvram(pas_entity_t *parent)
{
    nvram.parent = NULL;
    nvram.sdi_resource_hdl = NULL;

    _dn_nvram_free_entries();
}

/* Return the NVRAM cache record */
pas_nvram_t *dn_pas_nvram_rec_get(void) {
    return &nvram;
}

/* Parse the NVRAM TLV and save the tags into the DLL */
void dn_nvram_parse(void)
{
    uint8_t *data_p;
    t_std_error rc;
    uint16_t count;

    size_t remaining;

    unsigned long checksum;
    unsigned long stored_checksum;

    /* This assumes that the NVRAM is fairly small (< 2KB) */
    uint8_t buf[nvram.nvram_size];

    _dn_nvram_free_entries();

    do {
        /* Read NVRAM contents into memory */
        rc = sdi_nvram_read(nvram.sdi_resource_hdl, buf, 0, nvram.nvram_size);
        if (STD_IS_ERR(rc)) {
            PAS_ERR("Error reading contents from NVRAM");
            break;
        }

        if (memcmp(&buf[TLV_MAGIC_OFFS], TLV_MAGIC, TLV_MAGIC_LEN) != 0) {
            PAS_ERR("Magic does not match - %02x%02x%02x%02x", buf[0], buf[1], buf[2], buf[3]);
            break;
        }

        /* Check version */
        if (buf[TLV_VERSION_OFFS] != TLV_VERSION) {
            PAS_ERR("Version does not match - %02x", buf[0]);
            break;
        }

        /* Get count, which is stored in big-endian format */
        count = buf[TLV_COUNT_OFFS] << 8 | buf[TLV_COUNT_OFFS + 1];

        /* Fail if count exceeds the NVRAM capacity */
        if (count > (nvram.nvram_size - TLV_CSUM_LEN)) {
            PAS_ERR("Count(%d) exceeds max size (%d)", count, nvram.nvram_size);
            break;
        }

        /* Verify checksum */
        checksum = crc32(0, Z_NULL, 0); /* Initial value of checksum */
        checksum = crc32(checksum, buf, count);

        /* Read stored checksum, it is saved in big-endian format */
        data_p = &buf[count];
        stored_checksum = data_p[0] << 24 | data_p[1] << 16 |
                          data_p[2] << 8 | data_p[3];
        /* If bit-31 is set, the value will be sign extended to 64 bits, and
         * cause the below check to fail. Therefore, mask off the upper 32
         * bits and ensure that only the lower 32 bits are used in the
         * comparision
         */
        stored_checksum = stored_checksum & 0xFFFFFFFFUL;
        if (stored_checksum != checksum) {
            PAS_ERR("NVRAM checksum mismatch - expected %016llx, got %016llx",
                    stored_checksum, checksum);
            break;
        }

        /* Parse the TLV fields */
        remaining = count - TLV_CSUM_LEN - TLV_DATA_OFFS;
        for (data_p = &buf[TLV_DATA_OFFS]; data_p;
             data_p = std_tlv_next(data_p, &remaining)) {

            pas_nvram_entry_t *entry = calloc(1, sizeof(*entry));
            if (!entry) {
                /* Insufficient memory to save the data, return early */
                PAS_ERR("Insufficient memory to save NVRAM data");
                break;
            }

            entry->tag = std_tlv_tag(data_p);
            entry->length = std_tlv_len(data_p);

            /* Length limit the data size */
            if (entry->length > sizeof(entry->data)) {
               entry->length = sizeof(entry->data);
            }

            /* Copy the data to the entry */
            memcpy(entry->data, std_tlv_data(data_p), entry->length);

            /* Add the entry to the end of the list */
            std_dll_insertatback(&nvram.nvram_data, (std_dll *)entry);
        }
    } while(0);
}

/* Save the individual entries in TLV format */
void dn_nvram_write(void)
{
    unsigned long checksum;
    uint8_t tmpbuf[16];

    uint8_t nvram_data[nvram.nvram_size];
    uint8_t *data_p;

    size_t count;
    size_t remaining;

    pas_nvram_entry_t *entry;
    std_dll *dll_entry;

    /* Initialize the NVRAM with the Magic & Version fields */
    memcpy(&nvram_data[TLV_MAGIC_OFFS], TLV_MAGIC, TLV_MAGIC_LEN);
    nvram_data[TLV_VERSION_OFFS] = TLV_VERSION;

    /* Write the TLV fields */
    data_p = &nvram_data[TLV_DATA_OFFS];
    /* Remaining space for TLV data after accounting for header & checksum */
    remaining = nvram.nvram_size - TLV_DATA_OFFS - TLV_CSUM_LEN;

    for (dll_entry = std_dll_getfirst(&nvram.nvram_data); dll_entry;
         dll_entry = std_dll_getnext(&nvram.nvram_data, dll_entry)) {

        entry = (pas_nvram_entry_t *)dll_entry;
        data_p = std_tlv_add(data_p, &remaining,
                             entry->tag, entry->length, entry->data);

        if (!data_p) {
            /* TLV Add encountered an error, abort */
            return;
        }
    }

    /* Write the count (Big-Endian) */
    count = data_p - nvram_data;
    tmpbuf[0] = (count >> 8) & 0xFF;
    tmpbuf[1] = (count & 0xFF);
    memcpy(&nvram_data[TLV_COUNT_OFFS], tmpbuf, TLV_COUNT_LEN);

    /* Calculate the checksum */
    checksum = crc32(0, Z_NULL, 0);
    checksum = crc32(checksum, nvram_data, count);

    /* Write the checksum (Big-Endian) */
    tmpbuf[0] = (checksum >> 24) & 0xFF;
    tmpbuf[1] = (checksum >> 16) & 0xFF;
    tmpbuf[2] = (checksum >> 8) & 0xFF;
    tmpbuf[3] = (checksum & 0xFF);
    memcpy(&nvram_data[count], tmpbuf, TLV_CSUM_LEN);

    /* Write the data to NVRAM */
    sdi_nvram_write(nvram.sdi_resource_hdl, nvram_data,
                    TLV_MAGIC_OFFS, count + TLV_CSUM_LEN);
}
