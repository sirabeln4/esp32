# Hello World Wi-Fi Project

`hello_world` is a hardware bring-up application for the Espressif
ESP32-C6-DevKitC-1 N8. It verifies the board, the shared I2C bus, Wi-Fi, and a
minimal local web server.

## What it does

1. Prints the selected board name and default I2C pins.
2. Scans I2C on SDA GPIO22 and SCL GPIO23. It recognizes:
   - VEML7700 ambient-light sensor at `0x10`
   - SSD1306 OLED at `0x3C` or `0x3D`
   - SHT41 temperature/humidity sensor at `0x44`
3. Scans nearby Wi-Fi access points.
4. Connects to the Wi-Fi network configured through `menuconfig`.
5. Starts a web server that responds with a Hello World page.

## Configure Wi-Fi

Open an ESP-IDF PowerShell terminal, or activate ESP-IDF in a normal
PowerShell terminal:

```powershell
. "C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1"
```

From the repository root, configure the board-specific Wi-Fi settings:

```powershell
.\scripts\build.ps1 -Project hello_world -Board espressif_esp32c6_devkitc1_n8 -Menuconfig
```

Open **Wi-Fi connection test**, enter the SSID and password, then save and
exit. The generated configuration is stored in the ignored build directory.

## Build, flash, and monitor

With the board connected on `COM6`:

```powershell
.\scripts\build.ps1 -Project hello_world -Board espressif_esp32c6_devkitc1_n8 -Port COM6 -Flash -Monitor
```

After a successful connection, the monitor prints a URL such as
`http://192.168.86.29/`. Open that URL from a device on the same Wi-Fi network.
Exit the monitor with `Ctrl+]`.
