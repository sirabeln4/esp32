# ESP32-C6-DevKitC-1 Reference

This project targets the Espressif **ESP32-C6-DevKitC-1** with an
ESP32-C6-WROOM-1 N8 module (8 MB flash). Confirm the board's PCB silkscreen
and module marking before relying on this pinout; other ESP32-C6 boards use
different physical header layouts.

Source: [Espressif ESP32-C6-DevKitC-1 v1.2 user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/user_guide.html).

## Project I2C wiring

| Function | Board header | GPIO |
| --- | --- | --- |
| I2C SDA | J3 pin 6 (label `22`) | GPIO22 |
| I2C SCL | J3 pin 5 (label `23`) | GPIO23 |
| Sensor/display power | J1 pin 1 (label `3V3`) | 3.3 V |
| Ground | J3 pin 1, 12, or 15 (label `G`) | Ground |

## Header labels

Use the labels printed on the board rather than header position alone. The
following compact tables map header position to its silkscreen label.

### J1

| Pin | Label | Pin | Label |
| ---: | --- | ---: | --- |
| 1 | `3V3` | 9 | `8` |
| 2 | `RST` | 10 | `10` |
| 3 | `4` | 11 | `11` |
| 4 | `5` | 12 | `2` |
| 5 | `6` | 13 | `3` |
| 6 | `7` | 14 | `5V` |
| 7 | `0` | 15 | `G` |
| 8 | `1` | 16 | `NC` |

### J3

| Pin | Label | Pin | Label |
| ---: | --- | ---: | --- |
| 1 | `G` | 9 | `19` |
| 2 | `TX` / GPIO16 | 10 | `18` |
| 3 | `RX` / GPIO17 | 11 | `9` |
| 4 | `15` | 12 | `G` |
| 5 | `23` | 13 | `13` / USB D+ |
| 6 | `22` | 14 | `12` / USB D- |
| 7 | `21` | 15 | `G` |
| 8 | `20` | 16 | `NC` |

## USB and power

- **USB Type-C to UART:** power, flashing, and serial console through the
  on-board USB-to-UART bridge. This is the port used for the project on `COM6`.
- **ESP32-C6 USB Type-C:** native USB, flashing, serial communication, and
  JTAG debugging.
- The board can be powered through either USB-C port, or through the `5V` and
  `GND` headers, or through `3V3` and `GND` headers. Use only one intended
  power path at a time.
- The built-in RGB LED uses GPIO8.

## Pin cautions

- GPIO4, GPIO5, GPIO8, GPIO9, and GPIO15 are strapping pins. Avoid attaching
  circuits that force their voltage during reset unless their boot behavior is
  understood.
- GPIO12 and GPIO13 are the native USB D- and D+ pins. Do not use them for
  breadboard peripherals while relying on native USB.
- GPIO16 and GPIO17 are the UART TX/RX pins used by the UART console.

## Related official files

The user guide links to revision-specific schematics, PCB layout, dimensions,
and the ESP32-C6-WROOM-1 datasheet. Check the board revision before using a
schematic for hardware modifications.
