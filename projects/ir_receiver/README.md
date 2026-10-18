# XIAO ESP32-C6 infrared receiver

ESP-IDF project for a VS1838B demodulating 38 kHz receiver. Decodes standard NEC,
extended NEC (16-bit addresses), short held-button repeats, and repeated full NEC
frames. Prints address, command, and a 32-bit LSB-first raw value to the USB console.
Other protocols and invalid frames print raw mark/space durations for investigation;
they are **not** automatically decoded. No Arduino libraries are needed.

## Wiring

Disconnect power before wiring. Connect by the receiver's actual pin labels/datasheet:
bare receivers and breakout modules can have different pin orders. Do not infer the
order from a photograph of another module.

| VS1838B connection | XIAO connection |
| --- | --- |
| VCC / supply | 3V3 |
| GND | GND |
| OUT / signal | D0 (GPIO0) |

Use a receiver/module rated to operate at 3.3 V. Keep its output within the ESP32's
3.3 V logic range; do not connect a 5 V output directly. If your module requires a
5 V supply, use appropriate level shifting on OUT. A 100 nF capacitor between VCC
and GND close to the receiver helps with supply noise. Keep wires short.

The VS1838B removes the carrier and produces active-low pulses; the firmware captures
those pulses rather than sampling the 38 kHz carrier. The receive pin is set by
`IR_RX_GPIO` in `main/main.c`. ESP-IDF GPIO numbers are chip numbers, not XIAO D labels.

## Build, flash, and monitor

From the workspace root in an activated ESP-IDF PowerShell terminal:

```powershell
.\scripts\build.ps1 -Project ir_receiver -Board seeed_xiao_esp32c6
.\scripts\build.ps1 -Project ir_receiver -Board seeed_xiao_esp32c6 -Port COM5 -Flash -Monitor
```

Replace COM5 with the XIAO's actual USB serial port. The project's defaults select
native USB Serial/JTAG console output. Exit the ESP-IDF monitor with Ctrl+].
If flashing cannot connect, hold BOOT while connecting USB, then release BOOT and
retry. Press RESET after flashing if needed.

Point an NEC remote toward the receiver and press a button. For example:

```text
NEC address=0x0000 command=0x45 raw=0xBA45FF00
NEC repeat address=0x0000 command=0x45
```

Codes depend on the remote. NEC command inverse bytes are validated. Standard NEC
address inverse bytes are collapsed to an 8-bit address; otherwise the address is
interpreted as extended NEC. Raw values use the first received bit as bit zero, so
older tools using MSB-first notation may print a different hex value.

## Capture limits and troubleshooting

Capture uses 1 microsecond ticks, a 12 ms idle gap to end each frame, and a 64-symbol
buffer (up to 128 pulses). Long protocols, such as air-conditioner remotes, can be
truncated; protocols containing gaps of 12 ms or longer can be split. Raw output is
diagnostic rather than a lossless recorder under continuous traffic. Repeats need
a valid prior frame within 150 ms. Unknown frames clear the repeat association.

- No output at all: verify the USB port and reset the board with the monitor open.
- Ready message but no button output: verify VCC/GND/OUT, receiver orientation,
  remote batteries, and the chosen pin. Test at roughly 0.5 to 2 meters.
- Unknown frames: your remote may use another protocol. Save the raw timing output
  and identify the remote model before adding a decoder.
- Frequent random frames: check supply decoupling, wiring, and strong light sources.

## Decoder verification

`tests/test_decoder.py` loads the actual C decoder as a host shared library and
checks all 256 commands for three standard addresses, extended addressing, repeat
frames, timing tolerance, leading/trailing idle, and malformed/truncated input.
Build `main/nec_decoder.c` as a host shared library exporting `nec_decode`, then run:

```powershell
python projects/ir_receiver/tests/test_decoder.py PATH_TO_HOST_LIBRARY
```

These checks do not validate physical receiver wiring or RMT capture on the board.

## Sources

- [Seeed XIAO ESP32-C6 pin map and boot instructions](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [ESP-IDF RMT receiver documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/rmt.html)
- [Espressif NEC transceiver example](https://github.com/espressif/esp-idf/tree/v6.0.2/examples/peripherals/rmt/ir_nec_transceiver)
