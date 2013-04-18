CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(system_server C)

SET(SRCS
	ss_main.c
	ss_sysnoti.c
	ss_launch.c
	ss_queue.c
	ss_core.c
	ss_sig_handler.c
	ss_log.c
	ss_device_change_handler.c
	ss_predefine.c
	ss_noti.c
	ss_lowbat_handler.c
	ss_lowmem_handler.c
	ss_pmon_handler.c
	ss_mmc_handler.c
	ss_usb_handler.c
	ss_ta_handler.c
	ss_bs.c
	ss_procmgr.c
	ss_timemgr.c
	ss_cpu_handler.c
	ss_vibrator.c
	)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(MOVINAND_FORMAT movi_format.sh)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	ecore
	ecore-file
	ecore-x
	eina
	sysman
	vconf
	heynoti
	pmapi
	tapi
	dlog
	syspopup-caller
	notification
	devman
	device-node
	libsmack)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -g -fno-omit-frame-pointer -finstrument-functions")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")
IF( $ENV{ARCH} MATCHES "arm" ) 
	ADD_DEFINITIONS("-DTARGET")
ENDIF()
ADD_DEFINITIONS("-DDEBUG -DENABLE_DLOG_OUT")

SET(UDEV_RULES_PATH share/system-server/udev-rules)
SET(UDEV_RULES udev-rules/91-system-server.rules)

CONFIGURE_FILE(${UDEV_RULES}.in ${UDEV_RULES} @ONLY)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-ldl" "-lsmack")

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(FILES ${MOVINAND_FORMAT} DESTINATION bin)
INSTALL(FILES ${UDEV_RULES} DESTINATION ${UDEV_RULES_PATH})
INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/system_server.sh DESTINATION /etc/rc.d/init.d)
INSTALL(FILES system-server.conf DESTINATION /etc/dbus-1/system.d)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/mmc-smack-label DESTINATION bin)

ADD_SUBDIRECTORY(restarter)
ADD_SUBDIRECTORY(sys_event)
ADD_SUBDIRECTORY(sys_device_noti)
ADD_SUBDIRECTORY(sys_pci_noti)
