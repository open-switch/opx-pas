LIBRARIES=dn_platform_utils

dn_platform_utils_SRCS=platform-utils/dn_platform_utils_imp.c

dn_platform_utils_CFLAGS=-D_FILE_OFFSET_BITS=64

PROGRAMS=dn_pas_svc

dn_pas_svc_CFLAGS=-D_FILE_OFFSET_BITS=64
CXXFLAGS += -std=c++11

dn_pas_svc_SRCS=pald.c pas_monitor/pas_monitor.c pas/pas_main.c fuse/pas_fuse_main.c \
          pas/pas_chassis_handler.c pas/pas_entity_handler.c pas/pas_psu_handler.c pas/pas_fan_tray_handler.c \
          pas/pas_card_handler.c pas/pas_fan_handler.c pas/pas_led_handler.c pas/pas_display_handler.c \
          pas/pas_temp_threshold_handler.c pas/pas_pld_handler.c pas/pas_port_module_handler.c pas/pas_status_handler.c \
          pas/pas_media_handler.c pas/pas_media_channel_handler.c pas/pas_phy_handler.c \
          pas/pas_temperature_handler.c pas/pas_media_config_handler.c \
          pas_data_store.cpp pas_lib.c \
          pas_event.c pas_entity.c pas_psu.c pas_fan_tray.c pas_card.c pas_fan.c pas_media.c pas_media_utils.c \
          pas_utils.c pas_config.c pas_temp_sensor.c pas_led.c pas_display.c

dn_pas_svc_SRCS+=fuse/pas_fuse_fan.c fuse/pas_fuse_common.c fuse/pas_fuse_led.c fuse/pas_fuse_thermal_sensor.c \
           fuse/pas_fuse_display_led.c fuse/pas_fuse_entity_info.c fuse/pas_fuse_media.c fuse/pas_fuse_parser.c \
           fuse/pas_fuse_diag_mode.c fuse/pas_fuse_nvram.c

dn_pas_svc_SRCS+=remote_poller/pas_remote_poller.c

dn_pas_svc_LDFLAGS=-lfuse -ldn_platform_utils -ldn_common -lsdi_sys_v2 -lcps-api-common -lcps-class-map-util -lrt -levent_log -lpthread

# UNIT_TEST	= pasd_unittest_s6000

# pasd_unittest_s6000_SRCS	= unit_test/s6000/pasd_unittest_s6000.cpp
# ifeq (${PLATFORM},VM)
# pasd_unittest_s6000_CFLAGS	+= -DVM
# endif
# pasd_unittest_s6000_LDFLAGS	= -ldn_pas -lcps-api-common -lcps-class-map-util -ldn_common
# ifeq (${PLATFORM},VM)
# pasd_unittest_s6000_LDFLAGS	+= -lsdi_db
# endif

include ${MAKE_INC}/workspace.mak

