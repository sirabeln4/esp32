# ESP32-C6 I2C Sensor and Display Wiring

All modules share one I2C bus and are powered from the ESP32-C6 DevKitC-1's
3.3 V rail. Do not use the 5 V pin for this shared bus.

```text
ESP32-C6 DevKitC-1             SHT41        VEML7700       Elegoo OLED
------------------             -----        --------       -----------
3V3  ------------------------- VDD/VCC ---- VCC/VIN ------ VCC
GND  ------------------------- GND -------- GND ---------- GND

GPIO22 (J3 pin 6, SDA) ------- SDA -------- SDA ---------- SDA
GPIO23 (J3 pin 5, SCL) ------- SCL -------- SCL ---------- SCL
```

Expected I2C addresses:

| Device | Address |
| --- | --- |
| SHT41 temperature and humidity sensor | `0x44` |
| VEML7700 ambient-light sensor | `0x10` |
| Elegoo 0.96-inch I2C OLED | usually `0x3C`; sometimes `0x3D` |

## SHT41 / SHT41-D / GY-SHT41-D pinout

Four pins; I2C

The SHT41 breakout uses standard I2C communication with four pins. The
voltage range below refers to the GY-SHT41-D module; power it at 3.3 V in this
project so that the shared I2C bus uses safe ESP32-C6 logic levels.

| Pin | Type | Description | Notes |
| --- | --- | --- | --- |
| `VDD` | Power | Power-supply input (2.4 V to 5.5 V) | Wide voltage range for flexible applications |
| `GND` | Power | Ground connection | Connect to ESP32 ground |
| `SDA` | Communication | I2C data line | Bidirectional data communication |
| `SCL` | Communication | I2C clock line | Clock signal from the master device |

- Standard I2C interface for easy integration.
- Default I2C address: `0x44`.
- Specified module accuracy: +/-0.2 degrees C temperature and +/-1.8% relative humidity.
- The module's 2.4 V to 5.5 V input range supports 3.3 V and 5 V supplies; use
  3.3 V here because the VEML7700 and ESP32-C6 I2C logic require it.
- The OLED must be an I2C model with pins labeled `GND`, `VCC`, `SCL`, and
  `SDA`. Do not connect optional `INT` pins on the VEML7700.
