# Adafruit VEML7700 Lux Sensor Pinout

The Adafruit VEML7700 is a 16-bit I2C ambient-light sensor breakout. Its
default 7-bit I2C address is `0x10`.

## Module details

- 16-bit dynamic range.
- Stated range: 0 lux to 20,000 lux.
- Resolution: 0.0036 lux per count.
- Built-in level shifting for 3.3 V or 5 V power and logic.
- Dimensions: 16.6 mm x 16.5 mm x 4.0 mm.

## Power pins

| Pin | Function | ESP32-C6 connection |
| --- | --- | --- |
| `VIN` | Board power input. An on-board regulator accepts 3 V to 5 V and supplies the sensor. | Connect to `3V3` |
| `3Vo` | Regulated 3.3 V output from the module, up to 100 mA. | Leave unconnected |
| `GND` | Common power and logic ground. | Connect to `GND` |

Use `VIN`, not `3Vo`, to power the breakout. Although the board supports 5 V,
use the ESP32-C6's 3.3 V rail for this shared I2C project.

## I2C logic pins

| Pin | Function | ESP32-C6 connection |
| --- | --- | --- |
| `SCL` | I2C clock line. | `GPIO23` (J3 pin 5) |
| `SDA` | I2C data line. | `GPIO22` (J3 pin 6) |

## STEMMA QT

The two STEMMA QT connectors duplicate the I2C and power connections. They can
connect to another STEMMA QT-compatible board or accessory. They are optional
when using the breakout's pin headers.

## Power LED and LED jumper

- **Power LED:** green LED labeled `on`, located near the STEMMA QT connector.
- **LED jumper:** on the rear of the board. Cutting its trace disables the
  power LED.
