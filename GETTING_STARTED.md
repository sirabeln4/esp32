# Getting Started

## Requirements

- Git
- VS Code, if desired
- ESP-IDF 6.x and its matching tools
- A data-capable USB cable
- One of the boards listed in [README.md](README.md)

Open an ESP-IDF terminal and verify the environment:

```powershell
idf.py --version
```

## Build the starter application

From the repository root:

```powershell
.\scripts\build.ps1 -Project hello_world -Board seeed_xiao_esp32c6
```

Each board gets an independent directory under the selected application:

```text
projects/hello_world/build-seeed-xiao-esp32c6/
```

Generated configuration is kept inside that build directory, so building one
board does not overwrite another board's `sdkconfig`.

Available board names are:

```text
espressif_esp32_devkitc_v4
espressif_esp32s3_devkitc1_n8
espressif_esp32c6_devkitc1_n8
seeed_xiao_esp32c6
```

## Flash and monitor

Find the board's COM port in Windows Device Manager or PowerShell:

```powershell
Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description
```

Then build, flash, and monitor:

```powershell
.\scripts\build.ps1 -Project hello_world -Board seeed_xiao_esp32c6 -Port COM5 -Flash -Monitor
```

Exit ESP-IDF Monitor with `Ctrl+]`.

For boards with two USB connectors, confirm whether you are using the native
USB Serial/JTAG port or the USB-to-UART bridge. The target defaults enable a
secondary USB Serial/JTAG console on ESP32-S3 and ESP32-C6; the original ESP32
uses UART.

## Configure an application

Open `menuconfig` for one specific build profile:

```powershell
.\scripts\build.ps1 -Project hello_world -Board espressif_esp32c6_devkitc1_n8 -Menuconfig
```

Local choices are written under that board's ignored build directory. Put
reproducible, non-secret defaults in the appropriate tracked defaults file:

- `config/sdkconfig.common.defaults` for every build
- `config/sdkconfig.<target>.defaults` for one chip target
- `config/boards/<board>.defaults` for one exact board
- `projects/<application>/sdkconfig.defaults` for one application

Do not put Wi-Fi passwords or other credentials in tracked defaults files.

## Add an application

Copy `projects/hello_world` to a new directory, change the project name in its
top-level `CMakeLists.txt`, and replace the source under `main`. Portable drivers
and services belong in `shared_components`; application orchestration belongs
in the application's `main` component.

## Add a board

Before adding a board, identify its exact manufacturer, model, module marking,
flash size, PSRAM size, board revision, pinout, LED type, and USB connection.
Boards with the same ESP32 chip can still differ in all of those details.

Add all of the following:

1. A board entry in `cmake/project_setup.cmake` mapping the profile to an
   ESP-IDF target and defaults file.
2. `config/boards/<board>.defaults` for flash, PSRAM, and board-specific ESP-IDF
   settings.
3. `shared_components/board_support/include/boards/<board>.h` for wiring and
   physical-board capabilities.
4. An include branch and name macro in `board_support/include/board.h`.
5. A compile-definition branch in `board_support/CMakeLists.txt`.
6. The new profile in the build script's validation list and documentation.

Never assume that a third-party board matches an Espressif or Seeed pinout
merely because it uses the same chip.

## When code needs conditional behavior

Use `board.h` for pin mappings and board features. Use ESP-IDF target or SoC
capability macros only for genuine chip differences:

```c
#if CONFIG_IDF_TARGET_ESP32C6
    /* ESP32-C6-only behavior */
#endif
```

Most FreeRTOS, Wi-Fi, HTTP, MQTT, NVS, and sensor-processing code should remain
independent of both the board and CPU architecture.
