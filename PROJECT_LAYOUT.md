# Project Layout

## Design goals

This workspace keeps four concerns separate:

1. Applications define what a device does.
2. Shared components implement reusable drivers and services.
3. Chip-target defaults describe differences between ESP32, ESP32-S3, and
   ESP32-C6.
4. Board profiles describe the physical development board.

An exact board profile is always selected at configure time. There is no
ambiguous generic `esp32` board profile: `esp32` is an ESP-IDF chip target and
specifically means the original ESP32.

## Directory tree

```text
esp32/
|-- .gitignore
|-- README.md
|-- GETTING_STARTED.md
|-- PROJECT_LAYOUT.md
|-- cmake/
|   `-- project_setup.cmake
|-- config/
|   |-- sdkconfig.common.defaults
|   |-- sdkconfig.esp32.defaults
|   |-- sdkconfig.esp32s3.defaults
|   |-- sdkconfig.esp32c6.defaults
|   `-- boards/
|       |-- espressif_esp32_devkitc_v4.defaults
|       |-- espressif_esp32s3_devkitc1_n8.defaults
|       |-- espressif_esp32c6_devkitc1_n8.defaults
|       `-- seeed_xiao_esp32c6.defaults
|-- docs/
|   `-- HOME_SENSOR_PROJECT_PLAN.md
|-- projects/
|   `-- hello_world/
|       |-- CMakeLists.txt
|       |-- sdkconfig.defaults
|       `-- main/
|           |-- CMakeLists.txt
|           `-- main.c
|-- scripts/
|   `-- build.ps1
`-- shared_components/
    |-- board_support/
    |   |-- CMakeLists.txt
    |   `-- include/
    |       |-- board.h
    |       `-- boards/
    |           |-- espressif_esp32_devkitc_v4.h
    |           |-- espressif_esp32s3_devkitc1_n8.h
    |           |-- espressif_esp32c6_devkitc1_n8.h
    |           `-- seeed_xiao_esp32c6.h
    |-- networking/
    |   `-- README.md
    `-- sensor_drivers/
        `-- README.md
```

## Configuration order

`cmake/project_setup.cmake` assembles the defaults in this order:

1. Common defaults
2. Selected target defaults
3. Selected board defaults
4. Application defaults

Later files can override earlier files. A generated `sdkconfig` still has the
highest practical priority for that build, which is why each board profile has
its own build directory and generated configuration.

## Portability boundary

Application code includes `board.h` and uses names such as
`BOARD_I2C_SDA_GPIO`. It must not include a manufacturer-specific header.
Manufacturer headers are selected internally by `board_support`.

Board headers should contain only physical facts and explicitly chosen default
connections. Protocol drivers must not be implemented in board headers.

Chip-specific functionality should be isolated behind components or guarded by
ESP-IDF capability/target macros. For example, Thread code should not be part of
an original ESP32 build, while general Wi-Fi station code can be shared.

## Profile naming

Use lowercase identifiers with underscores:

```text
<manufacturer>_<board>_<important-module-variant>
```

Include the module variant when it affects flash or PSRAM, such as `n8`,
`n8r8`, or `n16r8`. Create separate profiles when two boards need different
ESP-IDF defaults, even if their header pinouts are identical.

