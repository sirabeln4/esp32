# Multi-board ESP32 ESP-IDF Workspace

This repository is a shared ESP-IDF workspace for applications that run on
multiple ESP32-family chips and development boards. It initially supports:

| Board profile | ESP-IDF target | Expected hardware |
| --- | --- | --- |
| `espressif_esp32_devkitc_v4` | `esp32` | Espressif ESP32-DevKitC V4 with 4 MB flash |
| `espressif_esp32s3_devkitc1_n8` | `esp32s3` | ESP32-S3-DevKitC-1 N8 without PSRAM assumptions |
| `espressif_esp32c6_devkitc1_n8` | `esp32c6` | ESP32-C6-DevKitC-1 N8 |
| `seeed_xiao_esp32c6` | `esp32c6` | Seeed Studio XIAO ESP32-C6 with 4 MB flash |

The repository separates portable application code, chip configuration, and
physical board wiring. A build always selects an exact board profile; that
profile selects the correct ESP-IDF chip target and board defaults.

## Start here

Read [GETTING_STARTED.md](GETTING_STARTED.md), then build the starter project:

```powershell
.\scripts\build.ps1 -Project hello_world -Board espressif_esp32c6_devkitc1_n8
```

Flash it by adding the COM port:

```powershell
.\scripts\build.ps1 -Project hello_world -Board espressif_esp32c6_devkitc1_n8 -Port COM5 -Flash -Monitor
```

Run these commands from an activated ESP-IDF PowerShell terminal.

## Documentation

- [GETTING_STARTED.md](GETTING_STARTED.md): installation, builds, flashing, and adding a board
- [PROJECT_LAYOUT.md](PROJECT_LAYOUT.md): architecture and file ownership
- [docs/HOME_SENSOR_PROJECT_PLAN.md](docs/HOME_SENSOR_PROJECT_PLAN.md): the existing sensor-system plan carried forward as a reference

## GitHub

The repository is initialized locally on the `main` branch. After creating an
empty GitHub repository, connect and publish it with:

```powershell
git add .
git commit -m "Initial multi-board ESP32 workspace"
git remote add origin https://github.com/YOUR-USER/YOUR-REPOSITORY.git
git push -u origin main
```

Review `git status` before committing. Generated builds, generated `sdkconfig`
files, editor settings, and common secret files are excluded by `.gitignore`.

