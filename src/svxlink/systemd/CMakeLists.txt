#############################################################################
# SvxLink ROIP Repeater system Systemd setup
#############################################################################

set(SYSTEMD_CONFIGURATIONS_FILES_DIR "/lib/systemd/system" CACHE PATH
  "Path to systemd units")
set(SYSTEMD_DEFAULTS_FILES_DIR "${SYSCONF_INSTALL_DIR}/default" CACHE PATH
  "Path to systemd unit configuration files")

configure_file(svxlink_gpio_setup.service.in svxlink_gpio_setup.service @ONLY)
configure_file(svxlink.service.in svxlink.service @ONLY)
configure_file(remotetrx.service.in remotetrx.service @ONLY)
configure_file(svxreflector.service.in svxreflector.service @ONLY)
configure_file(svxlink.default.in svxlink @ONLY)
configure_file(remotetrx.default.in remotetrx @ONLY)
configure_file(svxreflector.default.in svxreflector @ONLY)

if(WITH_DMR_GW)
        configure_file(svxlink-dmr.service.in svxlink-dmr.service @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/svxlink-dmr.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        configure_file(svxlink-dmr.default.in svxlink-dmr @ONLY)
        install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink-dmr
        ${SYSTEMD_DEFAULTS_FILES_DIR})
endif(WITH_DMR_GW)

if(WITH_C4FM_GW)
        configure_file(svxlink-c4fm.service.in svxlink-c4fm.service @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/svxlink-c4fm.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        configure_file(svxlink-c4fm.default.in svxlink-c4fm @ONLY)
        install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink-c4fm
        ${SYSTEMD_DEFAULTS_FILES_DIR})
endif(WITH_C4FM_GW)

if(WITH_DSTAR_GW)
        configure_file(svxlink-dstar.service.in svxlink-dstar.service @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/svxlink-dstar.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        configure_file(svxlink-dstar.default.in svxlink-dstar @ONLY)
        install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink-dstar
        ${SYSTEMD_DEFAULTS_FILES_DIR})
endif(WITH_DSTAR_GW)

if(WITH_ASL_GW)
        configure_file(svxlink-asl.service.in svxlink-asl.service @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/svxlink-asl.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        configure_file(svxlink-dstar.default.in svxlink-dstar @ONLY)
        install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink-dstar
        ${SYSTEMD_DEFAULTS_FILES_DIR})
endif(WITH_ASL_GW)

if(WITH_EL_GW)
        configure_file(svxlink-el.service.in svxlink-el.service @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/svxlink-el.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
        configure_file(svxlink-el.default.in svxlink-el @ONLY)
        install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink-el
        ${SYSTEMD_DEFAULTS_FILES_DIR})
endif(WITH_EL_GW)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/svxlink_gpio_setup.service
              ${CMAKE_CURRENT_BINARY_DIR}/svxlink.service
              ${CMAKE_CURRENT_BINARY_DIR}/remotetrx.service
              ${CMAKE_CURRENT_BINARY_DIR}/svxreflector.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
message(STATUS "Systemd unit files will be installed to "
        "${SYSTEMD_CONFIGURATIONS_FILES_DIR} after make install")

install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxlink
        ${SYSTEMD_DEFAULTS_FILES_DIR} )
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/remotetrx
        ${SYSTEMD_DEFAULTS_FILES_DIR} )
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/svxreflector
        ${SYSTEMD_DEFAULTS_FILES_DIR} )
message(STATUS "Systemd unit configuration files will be "
               "installed to ${SYSTEMD_DEFAULTS_FILES_DIR} "
               "after make install")
