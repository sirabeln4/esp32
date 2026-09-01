# Shared setup included by each application's top-level CMakeLists.txt before
# ESP-IDF's project.cmake.

if(NOT DEFINED BOARD OR BOARD STREQUAL "")
    message(FATAL_ERROR
        "BOARD is required. Use scripts/build.ps1 or configure with "
        "-DBOARD=<profile>.")
endif()

set(_workspace_root "${CMAKE_CURRENT_LIST_DIR}/..")

if(BOARD STREQUAL "espressif_esp32_devkitc_v4")
    set(_board_target "esp32")
elseif(BOARD STREQUAL "espressif_esp32s3_devkitc1_n8")
    set(_board_target "esp32s3")
elseif(BOARD STREQUAL "espressif_esp32c6_devkitc1_n8")
    set(_board_target "esp32c6")
elseif(BOARD STREQUAL "seeed_xiao_esp32c6")
    set(_board_target "esp32c6")
else()
    message(FATAL_ERROR "Unknown BOARD profile: ${BOARD}")
endif()

if(DEFINED IDF_TARGET AND NOT IDF_TARGET STREQUAL _board_target)
    message(FATAL_ERROR
        "BOARD=${BOARD} requires IDF_TARGET=${_board_target}, but "
        "IDF_TARGET=${IDF_TARGET} was requested.")
endif()

set(IDF_TARGET "${_board_target}" CACHE STRING "ESP-IDF target" FORCE)
set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
set(SDKCONFIG_DEFAULTS
    "${_workspace_root}/config/sdkconfig.common.defaults"
    "${_workspace_root}/config/sdkconfig.${_board_target}.defaults"
    "${_workspace_root}/config/boards/${BOARD}.defaults"
    "${CMAKE_SOURCE_DIR}/sdkconfig.defaults"
)
list(APPEND EXTRA_COMPONENT_DIRS "${_workspace_root}/shared_components")

message(STATUS "Board profile: ${BOARD}")
message(STATUS "ESP-IDF target: ${IDF_TARGET}")

