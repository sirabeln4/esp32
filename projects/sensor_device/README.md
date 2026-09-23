# Sensor Device

`sensor_device` is the next application after `hello_world`. It reads the
SHT41 temperature/humidity sensor and VEML7700 ambient-light sensor over I2C,
updates the SSD1306 OLED every two seconds, and serves the latest readings in a
local browser page.

## Hardware

| Device | I2C address | Connection |
| --- | --- | --- |
| SHT41 | `0x44` | SDA GPIO22, SCL GPIO23, 3.3 V, GND |
| SSD1306 OLED | `0x3C` or `0x3D` | SDA GPIO22, SCL GPIO23, 3.3 V, GND |
| VEML7700 | `0x10` | SDA GPIO22, SCL GPIO23, 3.3 V, GND |

## Configure Wi-Fi

The Wi-Fi credentials are per-project. Configure them before the first build:

```powershell
.\scripts\build.ps1 -Project sensor_device -Board espressif_esp32c6_devkitc1_n8 -Menuconfig
```

Open **Wi-Fi connection settings**, enter the SSID and password, then save and
exit. The generated configuration is ignored by Git.

## Build, flash, and monitor

The board profile selects the flash and partition layout automatically. The
DevKitC-1 N8 uses an 8 MB partition table; the XIAO ESP32-C6 uses a 4 MB
partition table. Both reserve one large factory application partition because
the MQTT-enabled firmware is larger than ESP-IDF's 1 MB default. USB flashing
works normally; over-the-air update support can be added later with a
different partition layout.

```powershell
.\scripts\build.ps1 -Project sensor_device -Board espressif_esp32c6_devkitc1_n8 -Port COM6 -Flash -Monitor
```

For the XIAO currently connected as **COM5**, change the board profile and use
that port:

```powershell
.\scripts\build.ps1 -Project sensor_device -Board seeed_xiao_esp32c6 -Port COM5 -Flash -Monitor
```

Each board has its own `build-<board>` directory and generated `sdkconfig`,
so Wi-Fi and MQTT settings remain separate. Run `-Menuconfig` once for each
board before its first flash. If switching after an older build exists, use
`-Fullclean` once for that board so its generated configuration is refreshed.

The monitor reports setup status and a local URL such as `http://192.168.86.29/`.
Temperature (Fahrenheit), humidity, and ambient light are displayed on the OLED
and browser page; the periodic readings are not printed to the serial monitor.
The browser page refreshes every five seconds. Exit the serial monitor with
`Ctrl+]`.

## Home Assistant MQTT

The device publishes temperature (Fahrenheit), humidity, and ambient light to
an MQTT broker and uses Home Assistant MQTT Discovery to create the three
sensor entities automatically. In Home Assistant, install/start the Mosquitto
broker app, configure the MQTT integration with discovery enabled, and create a
dedicated non-administrator user such as `esp32_sensor`.

Then run menuconfig and open **Home Assistant MQTT settings**. The defaults use
the local broker at `mqtt://192.168.86.50:1883` and username `esp32_sensor`.
Enter the MQTT password and the friendly **Home Assistant device name** there;
do not add the password to source code or commit the generated sdkconfig. Keep
the **MQTT device ID** unchanged after discovery because it controls MQTT
topics and Home Assistant unique IDs.

ESP-IDF 6 distributes MQTT separately from the core framework. This project
therefore includes Espressif's official ESP-MQTT v1.1.0 source under
`components/mqtt`, so a normal build does not depend on downloading it from the
component registry.
