ACLOCAL_AMFLAGS=-I m4

SUBDIRS = inc

lib_LTLIBRARIES = libopx_pas.la
libopx_pas_la_SOURCES=   src/platform-utils/dn_platform_utils_imp.c
libopx_pas_la_CPPFLAGS= -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/inc/opx -I$(top_srcdir)/inc/opx/private -I$(includedir)/opx
libopx_pas_la_CXXFLAGS=-std=c++11
libopx_pas_la_LDFLAGS= -shared -version-info 1:1:0
libopx_pas_la_LIBADD= -lopx_common -lopx_cps_api_common -lopx_cps_class_map -lopx_logging

#copy opx-pas systemd service file to target
systemdconfdir=/lib/systemd/system
systemdconf_DATA= config/init/opx-pas.service


#The CLI used to change levels at runtime
bin_PROGRAMS = opx_pas_service

nobase_include_HEADERS = inc/opx/private/dn_pas.h inc/opx/dn_platform_utils.h

opx_pas_service_SOURCES = src/pas_lib.c src/pald.c src/pas_monitor/pas_monitor.c src/pas/pas_main.c src/fuse/pas_fuse_main.c \
                       src/pas/pas_chassis_handler.c src/pas/pas_entity_handler.c src/pas/pas_psu_handler.c src/pas/pas_fan_tray_handler.c \
                       src/pas/pas_card_handler.c src/pas/pas_fan_handler.c src/pas/pas_led_handler.c src/pas/pas_display_handler.c \
                       src/pas/pas_temp_threshold_handler.c src/pas/pas_pld_handler.c src/pas/pas_port_module_handler.c src/pas/pas_status_handler.c \
                       src/pas/pas_media_handler.c src/pas/pas_media_channel_handler.c src/pas/pas_phy_handler.c \
                       src/pas/pas_temperature_handler.c src/pas/pas_media_config_handler.c src/pas_data_store.cpp \
                       src/pas_event.c src/pas_entity.c src/pas_psu.c src/pas_fan_tray.c src/pas_card.c src/pas_fan.c src/pas_media.c src/pas_media_utils.c \
                       src/pas_utils.c src/pas_config.c src/pas_temp_sensor.c src/pas_led.c src/pas_display.c

opx_pas_service_SOURCES+= src/fuse/pas_fuse_fan.c src/fuse/pas_fuse_common.c src/fuse/pas_fuse_led.c src/fuse/pas_fuse_thermal_sensor.c \
                        src/fuse/pas_fuse_display_led.c src/fuse/pas_fuse_entity_info.c src/fuse/pas_fuse_media.c \
			src/fuse/pas_fuse_parser.c src/fuse/pas_fuse_diag_mode.c src/remote_poller/pas_remote_poller.c \
			src/fuse/pas_fuse_nvram.c

opx_pas_service_CPPFLAGS= -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/inc/opx -I$(top_srcdir)/inc/opx/private -I$(includedir)/opx
opx_pas_service_CXXFLAGS= -std=c++11
opx_pas_service_LDFLAGS= -shared
opx_pas_service_LDADD= libopx_pas.la -lfuse -lopx_common -lopx_sdi_sys -lopx_cps_api_common -lopx_cps_class_map -lrt -lopx_logging -lpthread


# Tool scripts

bin_SCRIPTS =	scripts/opx-show-env
