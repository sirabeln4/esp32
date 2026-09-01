# ESP32 Home Sensor and Dashboard Project

This plan was carried forward from the original XIAO ESP32-C6 repository. Its
system design and sensor guidance remain useful, but wiring must follow the
selected board profile in the new multi-board workspace. Features such as
Thread or Zigbee require a capable target such as ESP32-C6.

## Goal

Build room sensor nodes that measure ambient light, temperature, and humidity, send the readings to a local server, display them on a dashboard, and automate smart lights based on measured light and the local dawn/dusk state.

## Recommended architecture

```text
ESP32-C6 + room sensors
          |
          |  2.4 GHz Wi-Fi / MQTT
          v
Mosquitto broker + Home Assistant
          |
          +-- Dashboard and measurement history
          +-- Lux plus dawn/dusk automation
          +-- Smart-light integration
          +-- Alexa as an optional voice interface
```

Use Wi-Fi for the first version. Fixed room nodes can be powered over USB, communicate directly with the MQTT broker, and are easier to debug than Bluetooth sensor nodes. Bluetooth LE is more useful later for battery-powered devices, but it requires a nearby gateway. The ESP32-C6 also supports the 802.15.4 radio used by Thread and Zigbee, leaving room to experiment with those protocols later.

## Hardware already available

An ESP32-C6 is already available, so there is no need to buy another board before beginning. In the multi-board workspace, select an exact board profile with `scripts/build.ps1`; the profile selects `esp32c6` automatically. Do not edit a generated `sdkconfig` to change chips:

```powershell
.\scripts\build.ps1 -Project home_sensor -Board espressif_esp32c6_devkitc1_n8
```

Then rebuild and flash the existing hello-world application before connecting sensors.

## Initial shopping list

Buy breakout boards or modules, not bare surface-mount sensor chips.

| Quantity | Part | Purpose |
| ---: | --- | --- |
| 1 | Sensirion SHT40 breakout | Temperature and relative humidity |
| 1 | Vishay VEML7700 breakout | Ambient light measured in lux |
| 1 | Solderless breadboard | Initial assembly |
| 1 set | Male-to-male jumper wires | ESP32-to-sensor connections |
| 1 | USB data cable and suitable 5 V supply | Programming and power |
| Optional | STEMMA QT/Qwiic-to-male cables | Solderless connection to Adafruit breakouts |

The SHT40 and VEML7700 both use I2C and can share the same SDA and SCL wires. Their normal I2C addresses do not conflict: SHT40 is `0x44`, and VEML7700 is `0x10`.

## Amazon examples

Amazon prices, sellers, and availability change frequently. Confirm the exact manufacturer and model number on the listing before ordering.

### Additional ESP32-C6 board

- Recommended model: **Espressif ESP32-C6-DevKitC-1-N8**
- 8 MB flash, Wi-Fi 6 on 2.4 GHz, Bluetooth LE, and 802.15.4 for Thread/Zigbee.
- [Amazon listing linked by Espressif](https://www.amazon.com/dp/B0BRMSDR4R)
- [Espressif development-board catalog](https://www.espressif.com/en/products/devkits/esp32)
- [Official ESP32-C6-DevKitC-1 documentation](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/user_guide_v1.1.html)

Prefer a listing that explicitly says `ESP32-C6-DevKitC-1-N8` and `Espressif`. Similar-looking ESP32, ESP32-C3, and clone boards can have different pinouts.

### Temperature and humidity sensor

- Recommended breakout: **Adafruit Sensirion SHT40, product ID 4885**
- [Amazon exact-model search](https://www.amazon.com/s?k=Adafruit+4885+SHT40)
- [Adafruit product page for comparison](https://www.adafruit.com/product/4885)
- [Sensirion SHT40 specifications](https://sensirion.com/products/catalog/SHT40?show_inventory=SHT40-AD1B-R2)

The Amazon listing should show an assembled breakout board with pins or STEMMA QT/Qwiic connectors. Do not buy a reel, loose SHT40 chip, DHT11, or DHT22 as a substitute. The SHT40 is the better starting sensor for consistent room humidity measurements.

### Ambient light sensor

- Recommended breakout: **Adafruit VEML7700 Lux Sensor, product ID 4162**
- [Amazon exact-model search](https://www.amazon.com/s?k=Adafruit+4162+VEML7700)
- [Adafruit VEML7700 guide and product identification](https://learn.adafruit.com/adafruit-veml7700/overview)
- [Vishay VEML7700 specifications](https://www.vishay.com/en/product/84286/)

The listing should say VEML7700, I2C, and lux sensor. A simple photoresistor can detect relative brightness, but it will not give the stable, comparable lux measurements wanted for room automation.

If the exact Adafruit boards are unavailable on Amazon, compatible alternatives are acceptable when they clearly use genuine SHT40 and VEML7700 sensors, expose `VIN/3V3`, `GND`, `SDA`, and `SCL`, and support 3.3 V logic. Adafruit is recommended initially because its board documentation makes wiring and troubleshooting easier.

## Suggested first wiring

Choose two free ESP32-C6 GPIO pins for SDA and SCL and define them in the application. ESP-IDF allows I2C signals to be routed to suitable GPIOs, so the exact choices should follow the pinout of the particular C6 board already owned.

```text
ESP32-C6                     SHT41                 VEML7700
development board            temperature/RH        ambient light
                             address 0x44           address 0x10

3V3 --------------------+---- VIN / 3V
                        |
                        +-------------------------- VIN / 3V

GND --------------------+---- GND
                        |
                        +-------------------------- GND

chosen SDA GPIO --------+---- SDA
                        |
                        +-------------------------- SDA

chosen SCL GPIO --------+---- SCL
                        |
                        +-------------------------- SCL
```

Equivalent connection table:

| ESP32-C6 | SHT41 breakout | VEML7700 breakout |
| --- | --- | --- |
| `3V3` | `VIN` or `3V` | `VIN` or `3V` |
| `GND` | `GND` | `GND` |
| Chosen SDA GPIO | `SDA` | `SDA` |
| Chosen SCL GPIO | `SCL` | `SCL` |

No I2C multiplexer is required because the SHT41 normally uses address `0x44`, while the VEML7700 uses `0x10`. In firmware, create one I2C master bus and add both addresses as devices on that bus.

Use 3.3 V unless the exact breakout documentation explicitly states otherwise. If the VEML7700 breakout has a `3Vo` regulator-output pin, leave it disconnected and use `VIN`. Do not power a bare sensor directly from 5 V.

Most assembled breakout boards include SDA and SCL pull-up resistors. If neither board includes them, add one 4.7 kOhm resistor from SDA to 3.3 V and another from SCL to 3.3 V. Do not add extra pull-ups until the breakout-board documentation has been checked.

## Soldered deployment board

The **ElectroCookie Mini PCB Prototype Board, model `ECPB_M_Multi_6P`**, is suitable for a first deployed sensor node if the ESP32-C6 development board uses standard 2.54 mm header spacing and its two header rows align with the holes. Always dry-fit the exact ESP32-C6 board before soldering.

The ElectroCookie boards are approximately 50.8 x 38.1 mm, double-sided, gold plated, and use plated-through holes. They are mechanically stronger and easier to rework than inexpensive single-sided phenolic perfboard.

- [ElectroCookie mini prototype board description](https://electrocookie.net/product/electrocookie-mini-pcb-prototype-board-solderable-breadboard-for-diy-e/68/)

### Connected-hole layout

The board follows a miniature solderless-breadboard pattern. Each five-hole group is electrically connected by copper underneath, while the two sides of the center gap are separate:

```text
A B C D E       F G H I J
o-o-o-o-o       o-o-o-o-o
 connected       connected

Each numbered column is separate from the next column unless
the columns are joined with a soldered wire.
```

Use a multimeter in continuity mode to confirm the actual connected-hole pattern before installing components. Do not rely only on the board color or printed markings.

### Recommended physical arrangement

Use the mini board primarily as a carrier for the ESP32-C6 and locking sensor connectors. Keep the sensors on short cables instead of mounting them immediately beside the ESP32.

```text
                       short four-wire cable
+----------------------+                 +---------+
| Mini prototype board |-----------------| SHT41   |
|                      |                 +---------+
|  ESP32-C6 installed  |
|  in female headers   |-----------------| VEML7700|
|                      |                 +---------+
|  Sensor connectors   |
+----------------------+
   ^              ^
 antenna end    USB connector
 overhangs      remains accessible
```

Mount the ESP32-C6 in female header sockets instead of soldering it permanently. This allows a failed or upgraded controller to be replaced without rebuilding the node. Place the board so that the USB connector remains accessible and the PCB antenna extends beyond the prototype-board edge.

The mini board may be almost completely occupied by a full-size ESP32-C6 development board. That is acceptable: put only the ESP32 sockets and sensor-cable connectors on the carrier. If more wiring space is required, use a half-size solderable breadboard PCB instead.

### Board-level electrical connections

Both sensors connect in parallel to the same four signals:

```text
ESP32-C6 3V3 ----+---- sensor connector ---- SHT41 VIN
                 |
                 +---- sensor connector ---- VEML7700 VIN

ESP32-C6 GND ----+---- sensor connector ---- SHT41 GND
                 |
                 +---- sensor connector ---- VEML7700 GND

ESP32-C6 SDA ----+---- sensor connector ---- SHT41 SDA
                 |
                 +---- sensor connector ---- VEML7700 SDA

ESP32-C6 SCL ----+---- sensor connector ---- SHT41 SCL
                 |
                 +---- sensor connector ---- VEML7700 SCL
```

Suitable connector choices include:

- STEMMA QT/Qwiic JST-SH connectors when both sensor breakouts support them.
- Four-pin JST-XH locking connectors for individually wired sensor modules.
- Standard 2.54 mm headers for bench testing, although loose Dupont jumpers are not recommended in a deployed enclosure.

### Soldering sequence

1. Dry-fit the ESP32-C6 and confirm that both header rows align without bending.
2. Insert two 2.54 mm female-header strips into the prototype board.
3. Temporarily plug the ESP32-C6 into the headers to hold them straight.
4. Solder the headers from the underside and then remove the ESP32-C6.
5. Install one or two four-pin sensor connectors.
6. Use 24 to 26 AWG insulated solid-core wire on the underside for `3V3`, `GND`, `SDA`, and `SCL`.
7. Inspect every joint for solder bridges.
8. With the ESP32 and sensors unplugged, verify continuity for every intended connection.
9. Confirm there is no continuity between `3V3` and `GND`.
10. Install the ESP32 and sensors only after the unpowered checks pass.

### Deployment precautions

- Keep the SHT41 away from the ESP32 and voltage regulator so that board heat does not bias the temperature reading.
- Put the SHT41 near ventilation holes, but do not coat or cover its sensing opening.
- Put the VEML7700 near a clear enclosure opening and away from direct glare from the controlled lamp.
- Provide strain relief where sensor cables enter the enclosure.
- Use nylon standoffs and the PCB mounting holes rather than allowing the solder joints to support the assembly.
- Use a ventilated plastic enclosure rather than a metal enclosure.
- Keep wiring, copper, sensors, fasteners, and metal away from the ESP32-C6 antenna.

Espressif recommends placing a module antenna beyond the carrier-board edge when possible. If it cannot overhang, maintain approximately 15 mm of clearance around the antenna and test wireless range in the final enclosure.

- [Espressif ESP32-C6 antenna placement guidance](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c6/pcb-layout-design.html)

After two or three identical nodes have been built and their layout is stable, consider replacing the solderable prototype board with a custom two-layer PCB.

## Server and dashboard

Use Home Assistant OS with its Mosquitto MQTT broker. Home Assistant can run on an existing always-on x86-64 computer, a Raspberry Pi 4/5, or a Home Assistant Green. Keep the server on Ethernet when possible.

- [Home Assistant installation options](https://www.home-assistant.io/installation)
- [Home Assistant MQTT integration](https://www.home-assistant.io/integrations/mqtt)
- [ESP-IDF MQTT client documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/protocols/mqtt.html)

Do not expose the MQTT broker directly to the internet. Use authentication and a dedicated MQTT account for the ESP32 nodes.

## Light automation design

A reasonable starting rule is:

```text
IF lux is below 40 for 3 minutes
AND the selected dawn/dusk condition is true
AND the light is currently off
THEN turn the light on
```

Use hysteresis and delays to prevent flickering:

- Turn on below approximately 40 lux for 3 minutes.
- Treat the room as bright again above approximately 80 lux for 5 minutes.
- Add a cooldown after changing the light.
- Tune the thresholds from recorded measurements rather than treating these initial values as universal.

The lamp will increase the light sensor reading. Do not immediately turn the lamp off merely because its own light pushed the reading above the bright threshold. Base automatic shutoff on occupancy, time, sustained daylight, or a carefully positioned sensor.

Home Assistant calculates sunrise, sunset, dawn, dusk, and solar elevation from the configured home location:

- [Home Assistant Sun integration](https://www.home-assistant.io/integrations/sun/)

Integrate the smart bulb or certified smart switch directly into Home Assistant. Alexa can remain the voice interface, while Home Assistant performs the sensor-driven automation. This is simpler and more reliable than making the ESP32 send a simulated Alexa voice request.

- [Home Assistant Alexa integration](https://www.home-assistant.io/integrations/alexa)

## Development sequence

1. Set the ESP-IDF target to `esp32c6`, build, flash, and monitor hello world.
2. Configure the ESP-IDF I2C master and detect both sensor addresses.
3. Read and log SHT40 temperature and humidity.
4. Read and log VEML7700 lux.
5. Add Wi-Fi connection and automatic reconnection.
6. Publish readings to MQTT every 30 to 60 seconds.
7. Add MQTT availability, retained discovery messages, and unique device IDs.
8. Create the Home Assistant dashboard and light automation.
9. Add watchdog recovery, stored configuration, and over-the-air updates.
10. Build additional room nodes after the first node is stable.

## Sensor placement

Place the SHT40 away from the ESP32, voltage regulator, and direct sunlight. Heat from the development board can bias the room-temperature reading. A short cable and a ventilated enclosure help.

Place the VEML7700 where it sees representative room daylight without looking directly into the controlled lamp. Record lux for several days before deciding on the final automation thresholds.

## Later additions

- SCD40 or SCD41 for true CO2 measurement.
- Magnetic reed switches for doors and windows.
- PIR motion sensors for inexpensive motion detection.
- Millimeter-wave sensors for detecting stationary occupants.
- Water-leak probes near plumbing and water heaters.
- BME280 when barometric pressure is also wanted; do not confuse it with the BMP280, which does not measure humidity.

Avoid controlling household mains voltage with a homemade relay board while learning. Use certified smart bulbs, plugs, or switches and control them through Home Assistant.
