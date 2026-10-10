# Aether Lab — Chrome Dino Game

A Chrome Dino-inspired endless runner built with an ESP32, a 128×64 OLED display, a push button, and a MAX98357 I²S audio amplifier. Jump over incoming cacti, earn points, and try to survive with three lives.

> **Project status:** Firmware source is available in `Chrome_Dino.ino`. Confirm the exact ESP32 board and wiring against your hardware before uploading; the GPIO assignments below are taken directly from the source code.

## Features

- **OLED gameplay:** Draws a pixel-style dinosaur, cacti, scrolling ground, score, and remaining lives.
- **Jump control:** A push button makes the dinosaur jump when it is on the ground.
- **Physics:** Gravity and jump velocity simulate a simple jump arc.
- **Obstacle generation:** Two cacti move across the screen and are recycled with varied sizes and spacing.
- **Collision detection:** Rectangle-based collision checks use slightly forgiving hitboxes.
- **Three lives:** A collision removes one life. The game ends when all lives are lost.
- **Progressive difficulty:** The game gets faster as the score increases, up to a configured maximum speed.
- **Sound effects:** I²S audio tones and short sequences play for jumping, scoring, collisions, starting, game over, and restarting.
- **Game states:** Includes a title menu, gameplay, and game-over screen.
- **Serial diagnostics:** Prints startup status and gameplay events at 115200 baud.

## Hardware

| Component | Purpose |
|---|---|
| ESP32 development board compatible with the configured GPIO pins | Runs the game |
| 128×64 I²C OLED using an SSD1306 controller | Displays the game |
| Momentary push button | Starts the game, jumps, and restarts |
| MAX98357 I²S amplifier module | Converts I²S audio to amplified output |
| Compatible speaker | Plays game sound effects |
| Jumper wires and suitable power/USB connection | Wiring and power |

**Board compatibility note:** This sketch uses GPIO 32 for the button, GPIO 21/22 for I²C, and GPIO 25/26/27 for I²S. These assignments fit some classic ESP32 development boards, but not every ESP32 variant. In particular, do not assume this pinout will work unchanged on an ESP32-C3 or XIAO board. Check your exact board's pinout and adjust the definitions if needed.

## Wiring

### OLED display (I²C)

| OLED pin | ESP32 pin defined in code |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | Use the voltage supported by your OLED module |
| GND | GND |

The sketch uses I²C address `0x3C` and rotates the display by 180 degrees in software.

### Push button

| Button connection | ESP32 pin |
|---|---|
| One terminal | GPIO 32 (`BUTTON_PIN`) |
| Other terminal | GND |

The code configures the button as `INPUT_PULLUP`, so it is active-low: pressing the button connects GPIO 32 to GND. The sketch includes a 120 ms debounce interval.

### MAX98357 I²S amplifier

| MAX98357 pin | ESP32 pin defined in code |
|---|---|
| BCLK / BCK | GPIO 27 |
| LRC / LRCLK / WS | GPIO 25 |
| DIN | GPIO 26 |
| GND | GND |
| VIN | Follow the module manufacturer's power requirements |
| Speaker outputs | Connect a compatible speaker to the amplifier's speaker terminals |

Use a common ground between the ESP32 and modules. Follow the board and amplifier specifications for supply voltage and speaker wiring; do not connect a speaker directly to ESP32 GPIO pins.

## Software requirements

- Arduino IDE or another compatible Arduino build environment.
- ESP32 board support package compatible with the `ESP_I2S.h` API used by the sketch.
- **Adafruit GFX Library**
- **Adafruit SSD1306**
- The ESP32 Arduino core's `ESP_I2S.h` library/API.

The source includes:

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP_I2S.h>
#include <math.h>
```

`Wire` and `math.h` are generally supplied with the Arduino/ESP32 environment. Install the Adafruit libraries through Arduino IDE's Library Manager if they are not already installed.

## Getting started

1. Connect the OLED, button, and MAX98357 module using the wiring tables above.
2. Install the required libraries and select the correct ESP32 board and port in Arduino IDE.
3. Open `Chrome_Dino.ino`.
4. Verify that the GPIO assignments match your board and physical wiring.
5. Compile and upload the sketch.
6. Open Serial Monitor at **115200 baud** to view initialization messages and debugging output.
7. When the `DINO` / `RUNNER` menu appears, press the button to start.
8. Press the button while playing to jump. After game over, press it again to start a new run.

## Gameplay

- The dinosaur jumps only when it is on the ground; button presses while airborne are ignored.
- Each cactus passed increases the score.
- The movement speed increases with the score, up to `MAX_SPEED`.
- Hitting a cactus costs one life. The dinosaur and cacti reset while lives remain.
- When all three lives are lost, the game-over screen displays the final score.
- Press the button on the game-over screen to restart.

## Main configuration

The following values are defined near the top of the sketch and can be adjusted:

| Setting | Current value | Meaning |
|---|---:|---|
| `SCREEN_WIDTH` | `128` | OLED width in pixels |
| `SCREEN_HEIGHT` | `64` | OLED height in pixels |
| `OLED_ADDRESS` | `0x3C` | OLED I²C address |
| `BUTTON_PIN` | `32` | Button input |
| `I2S_BCLK` | `27` | I²S bit clock |
| `I2S_LRC` | `25` | I²S word/left-right clock |
| `I2S_DIN` | `26` | I²S data output from ESP32 |
| `FRAME_TIME` | `30` ms | Target game update interval (about 33 frames/s) |
| `MAX_LIVES` | `3` | Starting lives |
| `INITIAL_SPEED` | `1.7` | Initial cactus movement speed |
| `MAX_SPEED` | `4.0` | Maximum cactus movement speed |
| `GRAVITY` | `0.32` | Downward acceleration per update |
| `JUMP_FORCE` | `-5.2` | Initial upward velocity |

## Troubleshooting

- **OLED stays blank:** Check power, GND, SDA/SCL wiring, I²C address (`0x3C`), and the selected board's I²C pin support.
- **Button does not respond:** Confirm the button connects GPIO 32 to GND when pressed and that the board actually exposes GPIO 32.
- **No sound:** Check I²S BCLK/LRC/DIN wiring, amplifier power, common ground, speaker connections, and whether the selected ESP32 core supports the `ESP_I2S.h` API used here.
- **Compilation fails on `ESP_I2S.h`:** Check your installed ESP32 Arduino core version and its I²S API. This sketch uses the newer `I2SClass` interface.
- **Wrong orientation:** The sketch calls `display.setRotation(2)`. Change the rotation value if your module is mounted differently.
- **Pins differ on your board:** Update the pin definitions at the top of the sketch to match the board's pinout before wiring or uploading.

## Repository contents

A simple repository layout could be:

```text
chrome-dino-esp32/
├── Chrome_Dino.ino
├── README.md
├── images/
│   └── project-photo.jpg
└── docs/
    └── wiring-diagram.png
```

Add your own project photos, wiring diagram, and a demonstration-video link when available. Only include files that actually exist in your repository.

## Credits

Created as part of **Aether Lab**, a project collection focused on DIY electronics, embedded systems, and hands-on builds.

This is a Chrome Dino-inspired fan project and is not an official Google product.

## License

No license is specified by this README. Add a `LICENSE` file and update this section if you want to define how others may use, modify, or distribute the code.
