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

#ifndef __PAS_CONFIG_H
#define __PAS_CONFIG_H

/** ************************************************************************
 *
 * Interface to configuration handling
 */

#include "std_type_defs.h"
#include "cps_api_operation.h"
#include "sdi_entity_info.h"
#include "dell-base-pas.h"

#define INTERFACE_MODE_STR_LEN   (17)
#define MEDIA_TYPE_STR_LEN       (128)
#define MAX_SUPPORTED_SPEEDS           (BASE_IF_SPEED_MAX)
#define PAS_MEDIA_MAX_PORT_DENSITY     (10) /* Arbitrary upper limit for port density */
#define PAS_MEDIA_PORT_DENSITY_DEFAULT (1)  /* Port density is normally 1, but can be higher. Example it is 2 for QSFP28-DD*/


#define PRE_STRINGIZE(enm) #enm
#define STRINGIZE_ENUM(enm) PRE_STRINGIZE(enm)

/* Configuration information for an entity */

struct pas_config_entity {
    uint_t entity_type;
    uint_t sdi_entity_type;
    char   *name;
    uint_t num_slots;           /* Number of slots */
    uint_t poll_interval;       /* Poll interval, in ms */
    struct {                    /* Fan speed control params */
        bool   speed_control_en; /* Enable speed control */
        uint_t margin;          /* Speed error margin, in % */
        uint_t incr;            /* Integrator increase amount */
        uint_t decr;            /* Integrator decrease amount */
        uint_t limit;           /* Integrator limit */
    } fan;
};

/* Configuration information for card entity */

struct pas_config_card {
    bool   valid;               /* true <=> Card type is valid */
    uint_t type;                /* Card type */
};

struct pas_config_led {
    bool deflt;                 /* Default LED state */
};

struct pas_config_temperature {
    uint_t num_thresholds;      /* Number of temperature thresholds */
};

/* Comm-Dev configuration */

struct pas_config_comm_dev {
    uint_t poll_interval;  /* Polling interval */
};

/*
 * Media config for each media type.
 */

typedef struct pas_media_type_config_s {
    char                  media_type_str[MEDIA_TYPE_STR_LEN]; /* Media type string */
    PLATFORM_MEDIA_TYPE_t media_type;  /* Media type value */
    bool                  media_lr;    /* Long range media, High power mode to be enabled */
    bool                  supported;   /* Is this media supported in this platform*/
    bool                  media_disable;     /* media status, need to enable or not */
    bool                  cdr_control;       /* CDR needs to be controlled or not */
    uint_t                unsup_speed_count; /* Unsupported speed count */
    BASE_IF_SPEED_t       unsup_speed[MAX_SUPPORTED_SPEEDS];/* Unsupported speed list */
} pas_media_type_config;
/* Configuration information for ports */

typedef struct
{
    PLATFORM_PORT_TYPE_t      port_type;
    PLATFORM_MEDIA_CATEGORY_t category;
    PLATFORM_MEDIA_TYPE_t     media_type;
    BASE_IF_SPEED_t           speed;
    bool                      present;
    uint_t                    port_density;
}pas_port_info_t;

/* Configuration information for media resources */

struct pas_config_media {
    uint_t poll_interval;       /* Poll interval, in ms */
    uint_t rtd_interval;        /* Real time data poll interval =
                                   rtd_interval * poll_interval */
    bool   lockdown;            /* unsupported media above 10g capability
                                   disable if its true otherwise enable*/
    bool   led_control;         /* Handle LED set if led_control is true. */
    bool   identification_led_control; /* Port identification LED control flag, true incase PAS owns led control */
    uint_t pluggable_media_count;   /* Plugable media count. */
    bool   lr_restriction;      /* LR media restriction enable */
    uint_t media_count;         /* Media type config count. */
    pas_media_type_config  *media_type_config; /* Media type config based on platform. */
    pas_port_info_t  **port_info_tbl   ; /* An array of pointers to info of port*/
    uint_t port_count;                      /* Number of ports. */
};

/* Default configuration information for media type */

typedef struct pas_config_media_phy_defaults_s {
    char                  media_type_str[MEDIA_TYPE_STR_LEN]; /* Media type string */
    PLATFORM_MEDIA_TYPE_t media_type;    /* Media type value */
    char                  interface_mode[INTERFACE_MODE_STR_LEN]; /* Interface mode SGMII/SFI/GMII */
    bool                  intrl_phy_autoneg; /* Autonegotiation */
    uint_t                intrl_phy_supported_speed_count; /* Supported speed count */
    BASE_IF_SPEED_t       intrl_phy_supported_speed[MAX_SUPPORTED_SPEEDS]; /* Supported speed list */
} pas_media_phy_defaults;

/* Default config list */

typedef struct pas_config_media_phy_s {
    uint_t                   count; /* Entry count */
    pas_media_phy_defaults   *media_phy_defaults; /* List of default config for media types */
} pas_config_media_phy;

/* Get the chassis configuration */
sdi_entity_info_t *dn_pas_config_chassis_get(void);

/* Get the entity configuration for the given entity type */
struct pas_config_entity *dn_pas_config_entity_get_type(uint_t entity_type);

/* Get the entity configuration for the given index */
struct pas_config_entity *dn_pas_config_entity_get_idx(uint_t idx);

/* Get the card configuration */
struct pas_config_card *dn_pas_config_card_get(void);

/* Get the temperature configuration */
struct pas_config_temperature *dn_pas_config_temperature_get(void);

/* Get comm dev configuration */
struct pas_config_comm_dev *dn_pas_config_comm_dev_get (void);

/* Get the media configuration */
struct pas_config_media *dn_pas_config_media_get(void);

/* Get media phy configuration defaults */
pas_config_media_phy *dn_pas_config_media_phy_get(void);

/* Get configuration of an LED */
struct pas_config_led *dn_pas_config_led_get(
    uint_t entity_type,
    char   *name
                                             );

/* Get an opaque iterator to the first LED in the same group as the given LED */
void *dn_pas_config_led_group_iter_first(uint_t entity_type, char *name);

/* Return name of the LED corresponding to the given opaque iterator */
char *dn_pas_config_led_group_iter_name(void *iter);

/* Return the opaque iterator for the next-higher-priority LED */
void *dn_pas_config_led_group_iter_next(void *iter);

/* PAS CPS handler registration */
bool dn_pas_cps_handler_reg (const char *config_filename,
                cps_api_operation_handle_t _cps_hdl);
/* Read the configuration file */
bool dn_pas_config_init(const char *config_filename, cps_api_operation_handle_t _cps_hdl);
/* Used to keep track of basic port type configurations derived from config file */
typedef struct port_info_node
{
    pas_port_info_t node;
    struct port_info_node* next;
}port_info_node_t;

/* Type to convert port type string in config file to usable port type */
typedef struct
{
    const char*             port_type_str;
    PLATFORM_PORT_TYPE_t    port_type;
}port_type_str_to_enum_t;

/* Type to convert media type string in config file to usable media type */
typedef struct
{
    const char*             port_media_type_str;
    PLATFORM_MEDIA_TYPE_t   media_type;
}port_media_type_str_to_enum_t;

#endif /* !defined(__PAS_CONFIG_H) */
