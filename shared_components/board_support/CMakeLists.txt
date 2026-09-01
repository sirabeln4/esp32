idf_component_register(INCLUDE_DIRS "include")

if(BOARD STREQUAL "espressif_esp32_devkitc_v4")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BOARD_ESPRESSIF_ESP32_DEVKITC_V4=1)
elseif(BOARD STREQUAL "espressif_esp32s3_devkitc1_n8")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BOARD_ESPRESSIF_ESP32S3_DEVKITC1_N8=1)
elseif(BOARD STREQUAL "espressif_esp32c6_devkitc1_n8")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BOARD_ESPRESSIF_ESP32C6_DEVKITC1_N8=1)
elseif(BOARD STREQUAL "seeed_xiao_esp32c6")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BOARD_SEEED_XIAO_ESP32C6=1)
else()
    message(FATAL_ERROR "board_support does not implement BOARD=${BOARD}")
endif()

