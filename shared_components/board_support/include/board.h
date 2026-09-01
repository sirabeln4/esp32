#pragma once

#if defined(BOARD_ESPRESSIF_ESP32_DEVKITC_V4)
#include "boards/espressif_esp32_devkitc_v4.h"
#elif defined(BOARD_ESPRESSIF_ESP32S3_DEVKITC1_N8)
#include "boards/espressif_esp32s3_devkitc1_n8.h"
#elif defined(BOARD_ESPRESSIF_ESP32C6_DEVKITC1_N8)
#include "boards/espressif_esp32c6_devkitc1_n8.h"
#elif defined(BOARD_SEEED_XIAO_ESP32C6)
#include "boards/seeed_xiao_esp32c6.h"
#else
#error "No supported board profile was selected"
#endif

