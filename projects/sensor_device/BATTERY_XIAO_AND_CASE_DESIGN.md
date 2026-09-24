# Battery, XIAO ESP32-C6, and Case Design

Status: design notes. The current working firmware runs on the ESP32-C6-DevKitC-1.
The XIAO ESP32-C6 battery-powered version and case are the next hardware revision.

## Planned parts

| Part | Planned choice | Notes |
| --- | --- | --- |
| Controller | Seeed Studio XIAO ESP32-C6 | 21 x 17.8 mm; retains Wi-Fi, BLE, Zigbee, and Thread support. |
| Battery | JLJLUP 1S LiPo, 3.7 V, 1100 mAh | Flat pouch, stated integrated protection board, about 42 x 25 x 10 mm, JST 1.25 two-pin plug. |
| Device connector | Letool JST 1.25 mm two-pin pigtail | Use one mating pigtail as the removable battery harness. |
| External charger | Six-channel 1S LiPo charger with Micro JST 1.25 ports | Use only its 3.7 V / 4.20 V Micro JST 1.25 ports; not its JST-PH 2.0 ports. |
| Display | Existing 0.96-inch SSD1306 I2C OLED | About 27 x 28 mm board; display faces the front. |
| Temperature/humidity | Existing SHT41 breakout | Must be isolated from heat and exposed to room air. |
| Ambient light | Existing VEML7700 breakout | Must face room light, away from OLED light leakage. |

The XIAO can also charge a connected, qualified 3.7 V lithium battery through
its USB-C port. The removable-battery plan uses the external charger so a
charged spare can be swapped in immediately.

Sources: [XIAO ESP32-C6 battery guidance](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/),
[XIAO ESP32-C6 schematic](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32C6/XIAO_ESP32_C6_v1.0_SCH_260114.pdf).

## Battery and charging safety

- This is a **one-cell (1S), 3.7 V nominal, 4.20 V full-charge LiPo** system.
- Do not use an 18650 charger, a two-cell/three-cell balance charger, or a
  4.35 V LiHV mode.
- Do not solder directly to a pouch cell or pull its connector by the wires.
- Charge on a non-flammable surface or in a LiPo safety bag, attended, outside
  the finished wall case.
- Inspect each pouch before use. Do not use a swollen, punctured, hot, or
  damaged pack.
- The 6-channel charger has independent ports. Do not parallel batteries with
  a Y cable or adapter.
- Start external charging at **0.2 A**. Only use the charger's **0.6 A** mode
  after the battery manufacturer confirms that a 0.6 A charge rate is allowed.

## Removable-battery wiring

The XIAO has battery pads on its underside; it does not have a battery socket.
Solder one short JST 1.25 mating pigtail to those pads, then secure the pigtail
with strain relief inside the case.

```text
  Removable 1S LiPo                 XIAO ESP32-C6 (underside)
  ┌─────────────────┐              ┌────────────────────────┐
  │ + red ──┐        │              │ BAT+ pad near D5        │
  │ - black ─┼─ JST ─┼─ pigtail ────┤ BAT- pad near D8        │
  └─────────┘        │              └────────────────────────┘
                     │
               1.25 mm, 2-pin
```

Before connecting the battery for the first time, use a multimeter to verify:

```text
battery red -> pigtail red -> XIAO BAT+
battery black -> pigtail black -> XIAO BAT-
```

Connector names and wire colors in online listings are not a substitute for a
polarity check.

### XIAO battery-voltage measurement

The XIAO ESP32-C6 does not provide a usable battery percentage from the battery
pads alone. For the firmware's `BAT` display, add the documented **200 kΩ, 1:2
voltage divider** from the battery-voltage test point to the XIAO `A0` pin,
with the lower resistor connected to GND. This keeps the ADC input within its
safe range. The firmware converts approximately 3.30-4.20 V into 0-100%; it
is an estimate, not a fuel gauge, and will vary with load and battery age.

See Seeed's [XIAO ESP32-C6 battery-voltage guidance](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/).
Do not connect the raw LiPo voltage directly to an ESP32 GPIO or ADC input.

### Charging paths

```text
Normal installed charging
USB-C 5 V source -> XIAO USB-C -> XIAO charge controller -> installed LiPo

Fast swap charging
USB 5 V source -> 1S external charger (Micro JST 1.25 port) -> spare LiPo
```

For the external charger, select standard **1S / 3.7 V / 4.20 V LiPo** mode.
Never use its JST-PH 2.0 connector unless a future battery specifically uses
that connector.

## XIAO wiring for this project

The XIAO ESP32-C6 exposes the same GPIO numbers used by the current firmware:
`D4` is GPIO22 and `D5` is GPIO23. The I2C code can therefore keep SDA GPIO22
and SCL GPIO23 after the board profile is added.

```text
                         XIAO ESP32-C6
                    ┌─────────────────────┐
  3V3  ─────────────┤ 3V3                 │
  GND  ─────────────┤ GND                 │
  SDA, GPIO22 ──────┤ D4 / GPIO22         │
  SCL, GPIO23 ──────┤ D5 / GPIO23         │
                    └─────────────────────┘

 Shared I2C bus (all devices use the same four connections)

        3V3 ─────┬──── SHT41 VDD
                 ├──── VEML7700 VIN
                 └──── OLED VCC

        GND ─────┬──── SHT41 GND
                 ├──── VEML7700 GND
                 └──── OLED GND

 GPIO22 / SDA ───┬──── SHT41 SDA       address 0x44
                 ├──── VEML7700 SDA    address 0x10
                 └──── OLED SDA        address 0x3C

 GPIO23 / SCL ───┬──── SHT41 SCL
                 ├──── VEML7700 SCL
                 └──── OLED SCL
```

Use 3.3 V for all three I2C modules. Do not connect the battery directly to a
sensor or display power pin.

## Case requirements

1. The OLED must be visible from the front through a rectangular opening.
2. The VEML7700 needs its own front-facing opening and an opaque divider that
   prevents it from seeing the OLED directly.
3. The SHT41 must occupy a ventilated, thermally isolated air chamber.
4. The battery, XIAO, and charging components belong in a separate rear
   electronics compartment.
5. Provide a USB-C opening for programming and optional installed-battery
   charging.
6. Provide a rear battery door for swapping a pouch battery without disturbing
   the sensor wiring.
7. Use wall-mount keyhole slots or two recessed screw holes on the rear.

Avoid direct sun, HVAC registers, exterior-wall cold spots, kitchens, and
bathrooms. Temperature/humidity readings need airflow representative of the
room, not warm stagnant enclosure air.

## Layout A: compact flat wall sensor (starting point)

Suggested outside size: **90 x 65 x 25 mm**.

```text
Front (90 x 65 mm)                   Cross section
┌─────────────────────────┐          ┌──────────────────────┐
│  ┌───────────────────┐  │          │ OLED + VEML face     │
│  │   OLED window     │  │          ├──────────────────────┤
│  └───────────────────┘  │          │ XIAO + flat battery  │
│                      ○  │          │ (rear electronics)   │
│  Vents:  ║ ║ ║ ║ ║      │          ├── opaque baffle ────┤
│                         │          │ SHT41 air chamber    │
└─────────────────────────┘          │ vents front + bottom │
                                     └──────────────────────┘
      ○ = VEML7700 aperture
```

Pros: smallest useful enclosure; the 42 x 25 x 10 mm battery fits flat.

Risk: the battery/electronics are close to the SHT41, so the baffle and vent
path must be well designed.

## Layout B: accuracy-first wall sensor (selected)

Suggested outside size: **100 x 75 x 30 mm**.

```text
Front                                  Internal rear view
┌────────────────────────────┐         ┌────────────────────────────┐
│      OLED display window   │         │ XIAO        flat LiPo       │
│                        ○   │         │ USB-C opening               │
│                            │         ├──────── opaque divider ────┤
│  SHT41 air grille          │         │ SHT41 board in air chamber  │
│  ║ ║ ║ ║ ║ ║ ║ ║           │         │ front and bottom vents      │
└────────────────────────────┘         └────────────────────────────┘
```

Pros: better temperature accuracy, easier wiring, more room for a battery
door, and greater separation between the light sensor and OLED.

Tradeoff: larger than a commercial thermostat, but still suitable for a wall.

## Decisions made

- **Case layout:** Layout B, the accuracy-first design.
- **VEML7700 direction:** front-facing, with a separate aperture and an opaque
  divider from the OLED.
- **Battery access:** rear sliding battery cover.
- **USB-C access:** only when the main case is opened; no exterior USB-C cutout
  is required.
- **Fabrication:** no personal 3D printer is available.

## No-printer enclosure options

### Option 1: modified ABS project box (recommended)

Buy a two-piece indoor ABS project enclosure approximately **125 x 80 x 35 mm**.
This provides enough usable interior space for Layout B, the 42 x 25 x 10 mm
battery, sensor baffles, and mounting hardware. For example, Hammond's RL6215
ABS enclosure is 125 x 80 x 35 mm externally with about 118.8 x 73.8 mm usable
board area. [Hammond RL series dimensions](https://www.hammfg.com/es/electronics/small-case/plastic/rl.pdf)

Modify the box with ordinary hobby tools:

1. Cut the OLED opening in the front lid with a small hand saw, rotary tool, or
   drill-plus-file method. Start undersize and file to fit.
2. Drill the VEML7700 front aperture; begin with a 5 mm hole and enlarge only
   if the sensor is unnecessarily recessed.
3. Drill a row of small SHT41 ventilation holes or cut narrow slots at the
   lower front and lower edge.
4. Make the internal baffle from 1 to 1.5 mm ABS sheet or acrylic sheet. Attach
   it with small screws and standoffs, or an adhesive rated for ABS.
5. Create the rear battery opening and make a slide cover from thin ABS sheet.
   Two narrow ABS or styrene strips form guide rails; include a small stop screw
   or latch so the cover cannot slide out on the wall.

This option is robust, inexpensive, easy to revise, and works well for an
indoor wall sensor. It is the planned first enclosure method.

### Option 2: laser-cut acrylic sandwich case

Make the case from flat front, spacer, divider, and rear panels cut by a local
makerspace, library, sign shop, or online laser-cutting service. This produces
a clean front face and makes the ventilated SHT41 compartment easy to define.
It needs a precise drawing and uses screws/standoffs to assemble.

### Option 3: use a local print service

A library, makerspace, friend, or online 3D-print service can print a custom
case from a future design. You do not need to own a printer. This is useful
after the electronics have been proven in the modified ABS project box.

### Option 4: modify a commercial thermostat enclosure

Possible, but not recommended for the first build. It often lacks a battery
door, enough depth, and the internal separation needed for accurate SHT41
measurements.

## Proposed ABS project-box build details

```text
Front lid
+------------------------------------------------+
|          OLED opening: approximately 25 x 15 mm |
|                                            o     | <- VEML hole
|                                                  |
|  o o o o o o o o                                  | <- SHT41 vents
+------------------------------------------------+

Inside the base
+------------------------------------------------+
| XIAO + OLED wiring + battery                     |
|                                                   |
|  -------- opaque/thermal baffle --------         |
|  SHT41 board; air path from front to bottom       |
+------------------------------------------------+

Rear
+------------------------------------------------+
| Wall-mount holes                                 |
|  [ recessed rectangular battery opening ]        |
|  [ sliding ABS cover retained by two rails ]     |
+------------------------------------------------+
```

Keep the SHT41 in the lower chamber, with no direct line of sight to the
battery or XIAO. Put the VEML7700 on the opposite upper front corner from the
OLED, with a small opaque wall around the sensor board to prevent display light
from reaching it.

## Build drawing: 150 x 100 x 44 mm clear-lid project box

The selected IP65 project box is larger than the earlier Layout B example, so
use the extra space for a genuinely separate SHT41 air chamber. Orient it
landscape: **150 mm wide by 100 mm high**, with the clear lid facing into the
room and the black base against the wall.

The clear lid lets the OLED remain protected behind the plastic. Do **not** put
the VEML7700 behind the lid: it needs its own opening so that the cover plastic
does not change its light measurement.

### Front: clear lid, room-facing

All positions below are approximate measurements from the outside edges. Mark
them with masking tape, check the actual boards against the marks, then cut or
drill. Start every opening undersize.

```text
Room-facing clear lid, 150 mm wide x 100 mm high

       0 mm                                               150 mm
        +---------------------------------------------------+
        |                                                   |
        |   OLED board behind clear lid                     |
        |   screen centered about 42 mm from left           |  18 mm
        |   (no lid opening is required)                    |
        |                                       VEML7700    |
        |                                        5-6 mm o   |  20 mm
        |                                    [opaque hood]  |
        |                                                   |
        |  SHT41 air chamber                                |
        |  o o o o o o o o o o o o    <- 2 mm holes         |  78 mm
        |  o o o o o o o o o o o o    <- second row          |  84 mm
        +---------------------------------------------------+
                         bottom edge: four 2 mm SHT41 vents
```

- **OLED:** Attach its board to the inside of the clear lid with four small
  nylon standoffs, or thin high-bond foam tape at the board corners. Keep its
  screen almost touching, but not stressing, the clear lid. This preserves the
  waterproof front appearance without a display cutout.
- **VEML7700:** Drill a 5 mm pilot hole in the clear lid near the upper-right
  corner, then enlarge only if the sensor's viewing window is recessed. Mount
  the board inside, centered on the hole. Glue a short black plastic tube or a
  four-sided black foam/ABS hood around the sensor; it blocks OLED light from
  leaking into the sensor.
- **SHT41:** Place it in the lower-left section behind the two vent rows. The
  vents must not be covered by tape, glue, or wall-mount hardware. A small
  insect mesh on the inside is optional, but do not seal the vents.

### Inside: base and thermal divider

View this with the clear lid removed and the black base lying open. The divider
is the most important accuracy feature: it stops warm air from the XIAO and
battery entering the SHT41 chamber.

```text
Inside black base, lid removed (front is at top of drawing)

  +---------------------------------------------------------+
  | OLED ribbon/wires             VEML7700 board + hood     |
  |                                                         |
  |  XIAO ESP32-C6                 1S LiPo, flat            |
  |  antenna end toward left       42 x 25 x 10 mm           |
  |  and away from battery                                  |
  |                                                         |
  |=========================================================| <- baffle
  |  SHT41 board in its own lower air chamber               |
  |  front vent holes  -->  [SHT41]  -->  bottom vent holes |
  +---------------------------------------------------------+
```

Make the baffle from 1-1.5 mm black ABS, styrene, or acrylic sheet. It should
run nearly from the front/lid side to the rear/base side and nearly from one
side wall to the other. Seal its edges with small beads of neutral-cure silicone
or foam tape; leave only the intended SHT41 vent path open. Use screws and
small standoffs where possible so the design can be revised later.

Keep the XIAO's PCB antenna end at least 10-15 mm away from the LiPo, OLED
board, and long bundles of wire. The plastic enclosure is Wi-Fi friendly, so
no external antenna should be needed for a normal indoor location.

### Battery slide door and wall mounting

Do not place an access door on the rear face if the box will be screwed flat to
the wall: the wall would block it. Use a **bottom-edge sliding door** instead.
It still opens into the rear/electronics compartment, but remains accessible
while the enclosure stays mounted.

```text
Bottom edge, viewed from below while mounted on wall

  +---------------------------------------------------------+
  | fixed base | [  50 x 30 mm sliding battery door  ] | fixed base |
  +---------------------------------------------------------+
                 ^ two thin rails inside the base
                 ^ small stop screw or snap tab at one end
```

Cut the opening only after the battery and connector have been positioned.
Allow enough room to unplug the JST connector without pulling on its wires.
Secure the XIAO battery pigtail to the base with a zip-tie anchor or a glued
strain-relief point, so the connector cannot pull on the XIAO battery pads.

The IP65 rating applies only before these openings are made. This is an indoor
air-quality/light sensor case, so its finished design is intentionally vented
and is **not waterproof**.
