# ESP32 ESP-IDF Learning Path

Learn this workspace by completing one small exercise at a time. Use
[COMMAND_REFERENCE.md](COMMAND_REFERENCE.md) for quick reminders. This guide
explains the process and gives you checkpoints to confirm your understanding.

Plan on eight sessions of about 20–45 minutes; the first build may take longer.
No hardware is needed until the flashing lesson. The commands below run in
PowerShell from the workspace root.

## Your progress

- [ ] 1. Find your way around the workspace
- [ ] 2. Open the correct ESP-IDF environment
- [ ] 3. Create a small learning application
- [ ] 4. Build and inspect the results
- [ ] 5. Flash and watch the application run
- [ ] 6. Change, rebuild, and verify
- [ ] 7. Understand configuration and board selection
- [ ] 8. Apply the process to sensor_device

## 1. Find your way around the workspace

**Goal:** Distinguish source code, configuration, and generated files.

```text
esp32/                           Workspace root; run build.ps1 here
├── .vscode/                     Local editor and ESP-IDF selection settings
├── cmake/project_setup.cmake    Maps board profiles to chips and defaults
├── config/                      Shared, chip, and physical-board defaults
│   └── boards/                  Defaults for each supported board
├── docs/                        Wiring, hardware references, project plans
├── projects/                    Independently buildable applications
│   ├── hello_world/              Existing I2C/Wi-Fi/web test application
│   ├── ir_receiver/              IR receiver application
│   └── sensor_device/            Sensor, OLED, browser, and MQTT application
├── scripts/build.ps1            Workspace build/flash/monitor helper
└── shared_components/           Reusable components
    └── board_support/           Board names, capabilities, and pin mappings
```

The installed framework lives separately at `C:\esp\v6.1\esp-idf`.
Write your application in this workspace; you normally do not edit the
installed framework.

Inside an application:

| Item | What it contains | Edit it? |
| --- | --- | --- |
| `CMakeLists.txt` | Application name and build setup | Yes |
| `main/main.c` | Application code, including `app_main()` | Yes |
| `main/CMakeLists.txt` | Sources and component dependencies | Yes |
| `main/Kconfig.projbuild` | Optional custom menuconfig options | When needed |
| `sdkconfig.defaults` | Reproducible application defaults | Yes; no passwords |
| `components/` | Optional application-specific components | When needed |
| `build-<board>/` | Compiled firmware, build cache, generated configuration | Do not hand-edit compiled files |

**Exercise:** Open `sensor_device`'s two CMake files and find `app_main()` in
`main/main.c`. Open `scripts/build.ps1` and locate how it constructs the build
directory name. Notice that underscores in the board name become hyphens.

**Checkpoint:** Explain why editing a file under `build-…` is different from
editing `main/main.c`.

## 2. Open the correct ESP-IDF environment

**Goal:** Know which tools your terminal will use.

1. Open this workspace folder in VS Code.
2. Press **Ctrl+Shift+P**, then run **ESP-IDF: Select Current ESP-IDF Version**.
3. Select the installation at `C:\esp\v6.1\esp-idf`.
4. Open a new terminal with **ESP-IDF: Open ESP-IDF Terminal**.

```powershell
cd C:\Users\noels\workspace_vscode\esp32
idf.py --version
$env:IDF_PATH
Get-Location
```

Expected: ESP-IDF 6.1, the 6.1 framework path, and this workspace directory.
Your `.vscode/settings.json` currently selects 6.1, but an older terminal can
still contain a previous version's environment. Close it and open a new one.

**Checkpoint:** Explain the difference between opening PowerShell and opening
a PowerShell session with ESP-IDF activated.

## 3. Create a small learning application

**Goal:** Create a new project without changing your sensor application.

The existing `hello_world` now includes I2C, Wi-Fi, and a web server. For the
first exercise, create a smaller application named `learning_basics`.
This section describes files for you to create; they are not created by this guide.

In VS Code Explorer, create this structure under `projects/`:

```text
learning_basics/
├── CMakeLists.txt
├── sdkconfig.defaults
└── main/
    ├── CMakeLists.txt
    └── main.c
```

Put this in `projects/learning_basics/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/project_setup.cmake")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(learning_basics)
```

Put this in `projects/learning_basics/main/CMakeLists.txt`:

```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES board_support freertos log)
```

Put this in `projects/learning_basics/sdkconfig.defaults`:

```text
# Application-specific defaults can be added here later.
```

Put this in `projects/learning_basics/main/main.c`:

```c
#include "board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    unsigned int count = 0;
    ESP_LOGI("learning", "Starting on %s", BOARD_NAME);

    while (1) {
        ESP_LOGI("learning", "Learning build 1: count=%u", count++);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

ESP-IDF calls `app_main()` after framework startup. This loop prints a counter
and waits two seconds between messages. The delay lets other tasks run.
No sensors, display, or Wi-Fi settings are needed for this exercise.

**Checkpoint:** Identify which file names the application, which file lists
its sources, and which file contains the behavior.

## 4. Build and inspect the results

**Goal:** Compile firmware without needing a connected board.

Examples use the XIAO ESP32-C6. If using the Espressif C6 DevKit, replace
`seeed_xiao_esp32c6` with `espressif_esp32c6_devkitc1_n8` throughout.

```powershell
.\scripts\build.ps1 -Project learning_basics -Board seeed_xiao_esp32c6
```

The helper selects the application, board, chip target, defaults, and output
directory, then invokes `idf.py`. CMake configures the build; the compiler and
linker produce firmware. You do not need to invoke those tools individually.

Look under `projects/learning_basics/build-seeed-xiao-esp32c6/` for:

| Output | Purpose |
| --- | --- |
| `learning_basics.bin` | Application firmware bytes |
| `learning_basics.elf` | Linked application with symbols used for debugging and log decoding |
| `bootloader/bootloader.bin` | Firmware that loads the application at boot |
| `partition_table/partition-table.bin` | Describes regions of flash memory |
| `sdkconfig` | Effective configuration for this application and board |
| `CMakeCache.txt` | Cached build configuration |

A successful build does not change the firmware already on a board.
Run the same build again and observe how little work is needed when nothing
has changed.

**Checkpoint:** Find the `.bin` and `.elf` files and explain why the device
still runs its previous firmware after a build-only command.

## 5. Flash and watch the application run

**Goal:** Put your firmware on the board and observe it.

Flashing this lesson replaces the application currently on the selected board.
Use a spare board if you want the sensor application to keep running.

1. Connect the matching board with a USB data cable.
2. Find its port in Windows Device Manager under **Ports (COM & LPT)**.
3. Close another serial monitor if it is using that port.
4. Substitute the actual port for `COM5` below.

```powershell
.\scripts\build.ps1 -Project learning_basics -Board seeed_xiao_esp32c6 -Port COM5 -Flash -Monitor
```

This builds, flashes, and starts the serial monitor. Expect a startup message
with the board name and a counter approximately every two seconds.
Exit the monitor with **Ctrl+]**.

To reconnect to the monitor through the workspace helper:

```powershell
.\scripts\build.ps1 -Project learning_basics -Board seeed_xiao_esp32c6 -Port COM5 -Monitor
```

The helper always builds before monitoring; this command does not flash.
Press the board's reset button and watch the counter restart. Close the
monitor and notice the board keeps running. Power-cycle it and reconnect to
observe startup again; you do not need to flash after every power cycle.

**Checkpoint:** Explain build, flash, reset, and monitor as four separate actions.

## 6. Change, rebuild, and verify

**Goal:** Prove that your edited source reached the hardware.

1. Change `Learning build 1` to `Learning build 2` in `main.c` and save.
2. Run the build-only command from lesson 4.
3. Open the monitor. The installed application should still print build 1.
4. Exit the monitor, then run the flash-and-monitor command from lesson 5.
5. Confirm that the board now prints build 2.
6. Change the delay from `2000` to `1000`, build and flash again, and observe
   the faster counter.

Normal development is **edit → save → build → flash → observe**.
Incremental builds are normal; full cleaning is not required for ordinary edits.

**Checkpoint:** Demonstrate a change in both the message and timing.

## 7. Understand configuration and board selection

**Goal:** Know what changes when you select another board or configure an app.

An application describes behavior; a board profile describes the hardware.
Both the XIAO C6 and Espressif C6 DevKit target `esp32c6`, but their flash sizes
and wiring can differ. Do not choose a profile solely because the chip matches.

Open configuration for your learning application:

```powershell
.\scripts\build.ps1 -Project learning_basics -Board seeed_xiao_esp32c6 -Menuconfig
```

Explore the menus and exit without changes. `-Menuconfig` opens configuration
and exits afterward; it does not build or flash the application.

Defaults are assembled in this order:

1. `config/sdkconfig.common.defaults`
2. `config/sdkconfig.<chip>.defaults`
3. `config/boards/<board>.defaults`
4. `projects/<application>/sdkconfig.defaults`

Later defaults can override earlier defaults. Existing values in the generated
build directory's `sdkconfig` take precedence over defaults. Custom options
defined in an application's `main/Kconfig.projbuild` appear in menuconfig.

Keep passwords in local configuration, not tracked defaults. In this workspace,
generated `sdkconfig` is ignored by Git but still contains sensitive settings.
Do not share it without reviewing its contents.

Build directories belong to a particular application, board, and tool setup.
When switching ESP-IDF versions, use a fresh build directory or preserve your
settings before cleaning. This workspace stores `sdkconfig` inside the build
directory, so `-Fullclean` can remove those settings. See the version-switching
section of [COMMAND_REFERENCE.md](COMMAND_REFERENCE.md).

**Checkpoint:** Explain why Wi-Fi settings entered for one board do not
automatically configure another board's build.

## 8. Apply the process to sensor_device

**Goal:** Use the same workflow on your real application.

Read [the sensor application guide](projects/sensor_device/README.md) and
[the wiring guide](docs/I2C_SENSOR_DISPLAY_WIRING.md) for your actual board.
Use its matching profile and port. These examples use the XIAO:

```powershell
.\scripts\build.ps1 -Project sensor_device -Board seeed_xiao_esp32c6 -Menuconfig
.\scripts\build.ps1 -Project sensor_device -Board seeed_xiao_esp32c6
.\scripts\build.ps1 -Project sensor_device -Board seeed_xiao_esp32c6 -Port COM5 -Flash -Monitor
```

If this build directory was created under 6.0.2, follow the fresh-build migration
instructions in the command reference before building with 6.1.

Confirm startup status, OLED readings, and the browser URL in the log. Test MQTT
in Home Assistant if configured. The application does not print every sensor
reading to the serial monitor, so use its display and browser to observe data.

**Checkpoint:** Describe the complete journey from editing `main.c` to seeing
the resulting behavior on the sensor device.

## When something goes wrong

| Symptom | First thing to check |
| --- | --- |
| `idf.py` is not recognized | Open an activated ESP-IDF terminal |
| Wrong ESP-IDF version | Select the version and open a fresh terminal |
| Script cannot be found | Check `Get-Location`; run from the workspace root |
| Compiler error | Read the first relevant error and its source line |
| Configuration points to the old framework | Use a fresh build directory for the new version |
| Cannot open COM port | Confirm the port and close other serial programs |
| Cannot connect to the board | Check data cable, selected port, board profile, and board boot-mode instructions |
| Old application still runs | Save the file, check project/profile, and flash the rebuilt firmware |
| No expected messages | Check monitor port, reset the board, and confirm what this app logs |

Record the command, first useful error, and what fixed it in the command
reference. Avoid deleting build directories as your first troubleshooting step.

## After these lessons

Create a second application with a different message without copying generated
build directories. Then try adding one source file and listing it in
`main/CMakeLists.txt`. Move code into a shared component when multiple
applications actually need it. Add a new board profile only when your physical
board is not already supported; see [GETTING_STARTED.md](GETTING_STARTED.md).

## Official references

- [ESP-IDF 6.1 Windows workflow](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c6/get-started/windows-start-project.html)
- [ESP-IDF 6.1 idf.py command reference](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c6/api-guides/tools/idf-py.html)

The workspace helper adds board-profile selection and separate build directories
to the standard ESP-IDF workflow. Use the helper for these lessons instead of
running a generic `idf.py set-target` command at the workspace root.
