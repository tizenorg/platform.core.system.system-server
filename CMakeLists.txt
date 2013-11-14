CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(system_server C)

# deviced
SET(DEVICED_NAME deviced)
SET(VERSION 0.1.0)

SET(SRCS
	src/core/main.c
	src/core/sysnoti.c
	src/core/launch.c
	src/core/queue.c
	src/core/core.c
	src/core/sig-handler.c
	src/core/log.c
	src/core/device-change-handler.c
	src/core/predefine.c
	src/core/noti.c
	src/battery/lowbat-handler.c
	src/proc/lowmem-handler.c
	src/proc/pmon-handler.c
	src/mmc/mmc-handler.c
	src/usb/usb-handler.c
	src/ta/ta-handler.c
	src/bs/bs.c
	src/proc/procmgr.c
	src/time/time-handler.c
	src/cpu/cpu-handler.c
	src/core/common.c
	src/vibrator/vibrator.c
	src/core/edbus-handler.c)

# libdeviced
SET(DEVICED_SRCS
	src/shared/battery.c)

SET(DEVICED_HEADERS
	src/deviced/dd-battery.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

# libdeviced
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src/deviced)

SET(MOVINAND_FORMAT scripts/movi_format.sh)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	ecore
	ecore-file
	ecore-x
	edbus
	eina
	sysman
	vconf
	heynoti
	pmapi
	tapi
	dlog
	syspopup-caller
	notification
	device-node
	libsmack
	libsystemd-daemon)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

# libdeviced
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -fvisibility=hidden")
SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -g -fno-omit-frame-pointer -finstrument-functions")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DPREDEFINE_SO_DIR=\"${LIB_INSTALL_DIR}/ss_predefine/\"")
ADD_DEFINITIONS("-DLIBPATH=\"${LIB_INSTALL_DIR}/\"")
ADD_DEFINITIONS("-DFACTORYFS=\"$ENV{FACTORYFS}\"")
IF( $ENV{ARCH} MATCHES "arm" )
	ADD_DEFINITIONS("-DTARGET")
ENDIF()
ADD_DEFINITIONS("-DDEBUG -DENABLE_DLOG_OUT")

SET(UDEV_RULES_PATH share/system-server/udev-rules)
SET(UDEV_RULES udev-rules/91-system-server.rules)

CONFIGURE_FILE(${UDEV_RULES}.in ${UDEV_RULES} @ONLY)

# libdeviced
ADD_LIBRARY(${DEVICED_NAME} SHARED ${DEVICED_SRCS})
TARGET_LINK_LIBRARIES(${DEVICED_NAME} ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(${DEVICED_NAME} PROPERTIES VERSION ${VERSION})
INSTALL(TARGETS ${DEVICED_NAME} DESTINATION ${LIB_INSTALL_DIR} COMPONENT RuntimeLibraries)

FOREACH(hfile ${DEVICED_HEADERS})
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${hfile} DESTINATION include/${DEVICED_NAME})
ENDFOREACH(hfile)

CONFIGURE_FILE(${DEVICED_NAME}.pc.in ${DEVICED_NAME}.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DEVICED_NAME}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-ldl" "-ludev" "-lsmack")
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)

INSTALL(FILES ${MOVINAND_FORMAT} DESTINATION bin)
INSTALL(FILES ${UDEV_RULES} DESTINATION ${UDEV_RULES_PATH})
INSTALL(FILES system-server.conf DESTINATION /etc/dbus-1/system.d)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/packaging/system-server.rule DESTINATION /opt/etc/smack/accesses.d)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/mmc-smack-label DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/shutdown.sh DESTINATION ${LIB_INSTALL_DIR}/system-server)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/systemd/system-server.service DESTINATION /usr/lib/systemd/system)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/systemd/system-server.socket  DESTINATION /usr/lib/systemd/system)

ADD_SUBDIRECTORY(restarter)
ADD_SUBDIRECTORY(sys_event)
ADD_SUBDIRECTORY(sys_pci_noti)
