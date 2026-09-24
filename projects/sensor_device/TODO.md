# Sensor Device - Handover and TODO

Last updated: 2026-09-21

This file records the state of the ESP32-C6 sensor project and the next work
needed to use its readings in Home Assistant.

## Current hardware and firmware state

- Board: Espressif ESP32-C6-DevKitC-1 N8 on `COM6`.
- I2C wiring: SDA is GPIO22 and SCL is GPIO23.
- The I2C scan has confirmed all three devices:
  - SHT41 temperature/humidity sensor at `0x44`
  - VEML7700 ambient-light sensor at `0x10`
  - SSD1306 OLED display at `0x3C`
- `main/main.c` reads the SHT41 and VEML7700 every two seconds.
- Temperature is converted to Fahrenheit. The OLED and the ESP32 browser page
  show temperature, humidity, and lux. Periodic readings are intentionally not
  printed to the serial monitor.
- Wi-Fi credentials are configured through `menuconfig` and the project has a
  working local HTTP status page.
- The OLED continues showing sensor readings while Wi-Fi is unavailable. It
  shows `OFFLINE` and retries the connection every three minutes.
- The XIAO profile can show an approximate LiPo percentage after its required
  200 kΩ 1:2 A0 voltage divider is fitted. The DevKit profile leaves battery
  monitoring disabled.
- The board profile selects `partitions_8mb.csv` for the DevKitC-1 N8 and
  `partitions_4mb.csv` for the XIAO. Each provides one large factory
  application partition required by the MQTT-enabled firmware. The current
  layout is for USB flashing; it does not reserve a second OTA slot.

## Home Assistant/network notes

- Home Assistant is running on a wired Raspberry Pi 5.
- Its current local IPv4 address is `192.168.86.50` and the web UI is reached
  at `http://192.168.86.50/` (HTTP port 80).
- `homeassistant.local` currently fails in Chrome on the Windows laptop due to
  local-name/mDNS resolution. This is not an ESP32 or MQTT issue; use the IPv4
  address. Reserve that address in the router before relying on it.
- The Home Assistant web port (80) is separate from the MQTT broker port
  (normally 1883).

## Next milestone: MQTT publishing and Home Assistant discovery

### Home Assistant prerequisite

- [x] Mosquitto broker is installed and running.
- [x] The built-in **MQTT** integration is configured with discovery enabled.
- [x] Dedicated MQTT user created: `esp32_sensor`. Its password deliberately
      is not recorded in this file, source code, or Git.
- [x] Broker address: `192.168.86.50`, port: `1883`.

### Firmware work

- [x] Add the ESP-MQTT client to the project and use it in `main/main.c`.
      ESP-IDF 6 no longer bundles MQTT, so the official ESP-MQTT v1.1.0 source
      is kept in `components/mqtt`; do not delete that folder.
- [x] Add `menuconfig` options for broker URI, username, password, device ID,
      and publish interval. Generated `sdkconfig` values remain untracked
      because they contain credentials.
- [x] Start MQTT only after Wi-Fi has an IP address and reconnect automatically
      after Wi-Fi or broker loss.
- [x] Publish retained availability `online`/`offline` state, using MQTT
      last-will for `offline`.
- [x] Publish retained Home Assistant MQTT Discovery configuration for three
      sensor entities:
  - Temperature: device class `temperature`, state class `measurement`, unit
    `deg F` (Home Assistant's discovery payload should use the degree-F symbol)
  - Humidity: device class `humidity`, state class `measurement`, unit `%`
  - Ambient light: device class `illuminance`, state class `measurement`, unit
    `lx`
- [x] Publish one retained JSON state message at a modest interval (default:
      30 seconds) using a topic such as:

  ```json
  {"temperature_f":72.4,"humidity":43.1,"illuminance":35.8}
  ```

  All three Home Assistant sensors can use this one state topic with JSON value
  templates. Discovery topics and the availability topic should be retained;
  periodic sensor state need not be retained, although retaining it gives Home
  Assistant an immediate value after restart.
- [x] Keep the OLED and local browser page. MQTT is an additional output, not a
      replacement for either one.

### Validation

- [ ] Enter the MQTT password through menuconfig. Leave the broker URI and
      username defaults in place unless the Home Assistant address changes:

  ```powershell
  . "C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1"
  .\scripts\build.ps1 -Project sensor_device -Board espressif_esp32c6_devkitc1_n8 -Menuconfig
  ```

  Open **Home Assistant MQTT settings**, enter the password for `esp32_sensor`,
  save, and exit.

- [ ] Build, flash, and monitor:

  ```powershell
  . "C:\Espressif\tools\Microsoft.v6.0.2.PowerShell_profile.ps1"
  .\scripts\build.ps1 -Project sensor_device -Board espressif_esp32c6_devkitc1_n8 -Port COM6 -Flash -Monitor
  ```

- [ ] Verify the serial monitor reports Wi-Fi and MQTT connected (without
      logging credentials).
- [ ] In Home Assistant, verify that one new sensor device appears
      automatically with temperature, humidity, illuminance, and availability
      entities.
- [ ] Compare the three values with the OLED/browser page.

## Lighting automation after MQTT works

- [ ] Use the new **illuminance** entity as the automation trigger/condition.
- [ ] Initial safe rule: when lux is below `50 lx` for `5 minutes`, turn on the
      kitchen light entity.
- [ ] Use hysteresis to avoid rapid switching: only turn lights off above a
      higher threshold, such as `150 lx`, and preferably add a delay.
- [ ] Consider time of day, sun elevation, and occupancy before enabling an
      automatic turn-off action.
- [ ] Test the automation with a manual light toggle and a covered/uncovered
      VEML7700 before relying on it.

## Project boundaries

- Leave `projects/hello_world` unchanged; it is the earlier Wi-Fi/web/I2C test
  project.
- Make MQTT changes only in `projects/sensor_device`.
- Wiring and component reference notes are in the repository `docs/` folder.
