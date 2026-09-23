# SHT41 / SHT41-D / GY-SHT41-D Pinout

Four pins; I2C

The GY-SHT41-D breakout uses standard I2C communication. Power this module at
3.3 V when it shares the ESP32-C6 I2C bus with other sensors and displays.

| Pin | Type | Description | Notes |
| --- | --- | --- | --- |
| `VDD` | Power | Power-supply input (2.4 V to 5.5 V) | Wide voltage range for flexible applications |
| `GND` | Power | Ground connection | Connect to ESP32 ground |
| `SDA` | Communication | I2C data line | Bidirectional data communication |
| `SCL` | Communication | I2C clock line | Clock signal from the master device |

- Standard I2C interface for easy integration.
- Default I2C address: `0x44`.
- Specified module accuracy: +/-0.2 degrees C temperature and +/-1.8% relative humidity.
- The module supports a 2.4 V to 5.5 V input range; use 3.3 V in this project.
