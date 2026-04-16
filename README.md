# EDNA 2.0 — DIY ESP32 Quadcopter

**Michael Sell & Ben Schaser**

---

## Table of Contents

- [Introduction](#introduction)
- [Resources](#resources)
- [Materials](#materials)
- [Construction](#construction)
- [Software Setup](#software-setup)
- [Bluepad32 + ESP-IDF v6.0 Integration](#bluepad32--esp-idf-v60-integration)
- [Connectivity](#connectivity)
- [ESC Calibration](#esc-calibration)
- [Troubleshooting](#troubleshooting)
- [PID](#pid)

---

## Introduction

This project is intended to provide a research experience within computer science. It includes reading scientific literature, exploring unsolved problems, and making progress on difficult problems within CS. The goal is to understand the hardware and software associated with quadcopter drones — building a drone with a frame, motors, ESCs, a microcontroller, and a battery, controlled by software that handles motor control, Bluetooth gamepad input, and stable flight via a PID controller.

### Objectives

- Gain an intimate knowledge of drone hardware and commonly used software techniques
- Build a drone that can be flown via remote control
- Gain experience writing documentation for hardware and software

---

## Resources

- [Bluepad32 Game Controller Library](https://retro.moe/2020/11/24/bluepad32-gamepad-support-for-esp32/)
- [ESP Proprietary Drones](https://espressif-docs.readthedocs-hosted.com/projects/espressif-esp-drone/en/latest/gettingstarted.html)
- [ESP with PWM and Servo Control](https://dronebotworkshop.com/esp32-servo/#ESP32_PWM)
- [How Brushless Motors Work and How to Control Them](https://www.youtube.com/watch?v=uOQk8SJso6Q)
- [Quadcopter Physics and Control Software](https://andrew.gibiansky.com/blog/physics/quadcopter-dynamics/)
- [Drone Mechanics Review](Resources/review-drones.pdf)
- [Quadcopter Calculator](https://www.ecalc.ch/xcoptercalc.php)
- [ESC Calibration Video](https://youtu.be/t-w5Oog8Jcg)
- [ESC Calibration Library](https://github.com/lobodol/ESC-calibration)

**Motor Bench Tests** — data from three similar-spec motor tests averaged to estimate thrust:

- [Test 1](https://www.youtube.com/watch?v=77WlZwNHjo8&t=430s)
- [Test 2](https://www.youtube.com/watch?v=T0EzXr54jb8)
- [Test 3](https://www.youtube.com/watch?v=yRARMQXxQSY)

---

## Materials

| Component                             | Link                                                                                                                                                                                                                                   |
| ------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ESP32-WROOM-32D                       | [Amazon](https://www.amazon.com/HiLetgo-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0718T232Z) · [Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf) |
| 295mm Carbon Fiber Frame (7")         | [Amazon](https://www.amazon.com/dp/B086X2JZD6)                                                                                                                                                                                         |
| iFlight XING-E Pro 2306 2450KV Motors | [Amazon](https://www.amazon.com/iFlight-2450KV-Brushless-Racing-Quadcopter/dp/B096RTCGDT)                                                                                                                                              |
| 4-in-1 ESC                            | [Amazon](https://www.amazon.com/dp/B09SNWZRDG)                                                                                                                                                                                         |
| 4S LiPo Battery (3000–5500mAh)        | [Amazon](https://www.amazon.com/HOOVO-Battery-5500mAh-Connector-Compatible/dp/B09FJZKPKV)                                                                                                                                              |
| 7" Propellers                         | [Amazon](https://www.amazon.com/12PCS-HQProp-7X4X3-Light-Range/dp/B09NV9CGG2)                                                                                                                                                          |
| Buck Converter (5V output)            | [Amazon](https://www.amazon.com/DZS-Elec-Adjustable-Electronic-Stabilizer/dp/B06XRN7NFQ)                                                                                                                                               |
| LiPo Charger                          | [Amazon](https://www.amazon.com/Haisito-HB6-lipo-Charger/dp/B08C592PNV)                                                                                                                                                                |
| EC5 Battery Adapters                  | [Amazon](https://www.amazon.com/FLY-RC-Connector-Silicone-11-8inch/dp/B07C23S3RK)                                                                                                                                                      |

---

## Construction

### Frame

Follow the included frame instructions. The ESC mounts with included screws, shock absorbers, and nuts.

### Soldering / ESC

Hold the iron at an angle to maximize contact with the ESC pads. Pre-tin the pads before placing the wire. Let the chip cool between solder joints to avoid heat damage. The motor lead pins on top of the ESC didn't hold solder well — splice instead to the short Molex connector included with the ESC.

### Buck Converter

Tune to approximately 5.41V output using the onboard potentiometer before connecting to the ESP32.

### ESP32

Mount on a prototype PCB. Solder the ESP into the PCB and route wiring underneath to connect to the ESC leads at the back of the frame.

### MPU-6050 Gyro

Orient with `x` axis facing down, `y` axis pointing backward, `z` axis pointing left. Apply 90° manual offsets in software to account for this. Occasional overshoot from the Kalman filter is acceptable within expected range of motion.

---

## Software Setup

### Prerequisites

- [ESP-IDF v6.0](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32/get-started/index.html) installed at `C:\esp\v6.0\esp-idf`
- Git with submodule support
- Python in PATH
- VS Code + ESP-IDF extension (optional but recommended)

### Terminal Setup (Every New Session)

Several commands need to be run in order to setup the ESP-IDF environment to be able to build and flash programs to the ESP32. There is provided a setup script that performs these commands in the proper sequence.

> ! Always setup the shell environment before executing any `idf.py` command

#### Windows

```bash
# from root project dir

# init pyenv
pyenv shell 3.12.9

# expose idf.py
cd %USERPROFILE%/esp/v6.0/esp-idf/export.bat
call export.bat

# return to project dir
cd /path/to/root/project/dir
```

```bash
# alternative - run setup.bat from project dir

call setup.bat
```

#### macOS / Linux

```bash
# from root project dir

# init pyenv
pyenv shell 3.12.9 # init pyenv

# expose idf.py
cd ~esp/esp-idf-v6.0/export.sh
. ./export.sh

# return to project dir
cd /path/to/root/project/dir
```

```bash
# alternative - run setup.bat from project dir

# make sure the bash file is executable
chmod +x setup.sh

# run
. ./setup.sh
# or
source ./setup.sh
```

Test your shell environment by clearing the build folder with `idf.py fullclean`

### Clone and Build

```bash
git clone https://github.com/CherryWookie/EDNA_2.0
cd EDNA_2.0
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash monitor
```

Replace `COM3` with your actual port (check Device Manager → Ports with ESP32 plugged in).

---

## Bluepad32 + ESP-IDF v6.0 Integration

Bluepad32 v4.2.0 is not natively compatible with ESP-IDF v6.0. The following documents every patch required. All paths are relative to the project root `EDNA_2.0\` unless otherwise noted.

### Step 1 — Clone Bluepad32 with Submodules

> **Run from:** anywhere outside your project (e.g. `Documents\code\`)

```cmd
git clone --recursive https://github.com/ricardoquesada/bluepad32.git
```

Do **not** download as a ZIP — it won't include the `external/btstack` submodule that bluepad32 depends on.

### Step 2 — Run integrate_btstack.py

> **Run from:** `bluepad32\external\btstack\port\esp32\`

```cmd
cd bluepad32\external\btstack\port\esp32
set IDF_PATH=..\..\..\..\src
python integrate_btstack.py
```

`IDF_PATH=../../../../src` installs BTstack into `bluepad32/src/components/btstack/` (inside the bluepad32 tree) rather than your system ESP-IDF. Without this, it pollutes your global IDF and causes version conflicts.

Verify it worked:

```cmd
dir bluepad32\src\components\btstack\CMakeLists.txt
```

**Immediately reset IDF_PATH** or all subsequent `idf.py` commands will fail:

```cmd
set IDF_PATH=C:\esp\v6.0\esp-idf
C:\esp\v6.0\esp-idf\export.bat
```

### Step 3 — Copy Components into Your Project

> **Run from:** `EDNA_2.0\`

```cmd
xcopy /E /I "path\to\bluepad32\src\components\bluepad32"         "components\bluepad32_component"
xcopy /E /I "path\to\bluepad32\src\components\btstack"           "components\btstack"
xcopy /E /I "path\to\bluepad32\src\components\cmd_nvs"           "components\cmd_nvs"
xcopy /E /I "path\to\bluepad32\src\components\cmd_system"        "components\cmd_system"
xcopy /E /I "path\to\bluepad32\src\components\bluepad32\include" "components\bluepad32_component\include"
```

The last line copies subfolders (`platform/`, `controller/`, `bt/`) that the first xcopy misses.

---

### Required Patches

IDF v6.0 split the monolithic `driver` component into discrete `esp_driver_*` sub-components, fully removed `driver/timer.h`, and made `esp_log`/`freertos` implicit dependencies. Apply all patches below.

<details>
<summary><strong>Patch 1 — components/btstack/CMakeLists.txt</strong></summary>

**Error:** `fatal error: driver/gpio.h` / `driver/uart.h: No such file or directory`

Find the `set(priv_requires ...)` block and add:

```cmake
set(priv_requires
    "nvs_flash" "bt" "driver" "lwip" "vfs"
    "esp_driver_gpio"
    "esp_driver_uart"
    "esp_driver_i2s"
)
```

</details>

<details>
<summary><strong>Patch 2 — components/cmd_system/CMakeLists.txt</strong></summary>

**Error:** `fatal error: driver/uart.h: No such file or directory`

```cmake
if("${IDF_VERSION_MAJOR}" GREATER_EQUAL 5)
idf_component_register(SRCS "cmd_system.c"
    INCLUDE_DIRS .
    REQUIRES console spi_flash driver esp_driver_gpio esp_driver_uart)
endif()
```

</details>

<details>
<summary><strong>Patch 3 — components/bluepad32_component/CMakeLists.txt</strong></summary>

**Errors:** `driver/ledc.h`, `driver/spi_slave.h`, `driver/timer.h` not found; `VSPI_HOST` undeclared

**3a.** Update `requires` in the `if(IDF_TARGET)` block:

```cmake
set(requires "nvs_flash" "btstack" "app_update" "esp_timer"
    "esp_driver_ledc"
    "esp_driver_spi"
    "esp_driver_gptimer"
    "esp_driver_gpio"
    "esp_driver_uart")
```

**3b.** Remove `uni_mouse_quadrature.c` from the `if(IDF_TARGET)` srcs block — uses the fully removed `driver/timer.h`, only needed for Unijoysticle hardware:

```cmake
if(IDF_TARGET)
    list(APPEND srcs
         "arch/uni_console_esp32.c"
         "arch/uni_system_esp32.c"
         "arch/uni_log_esp32.c"
         "arch/uni_property_esp32.c"
         "uni_gpio.c")
endif()
```

**3c.** Strip the `if(CONFIG_IDF_TARGET_ESP32)` block. Remove `uni_platform_nina.c` (uses renamed `VSPI_HOST`) and all `uni_platform_unijoysticle*.c` files. Do **not** use `#` comments inside CMake `list()` calls — delete the lines entirely:

```cmake
if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND srcs
        "platform/uni_platform_mightymiggy.c")
endif()
```

</details>

<details>
<summary><strong>Patch 4 — components/bluepad32_component/arch/uni_console_esp32.c</strong></summary>

**Error:** `undefined reference to uni_mouse_quadrature_get_scale_factor`

Find the `mouse_scale` function (~line 113) and replace its body:

```c
static int mouse_scale(int argc, char** argv) {
    logi("Mouse quadrature not supported in this build\n");
    return 0;
}
```

</details>

<details>
<summary><strong>Patch 5 — components/bluepad32_component/platform/uni_platform.c</strong></summary>

**Error:** `undefined reference to uni_platform_unijoysticle_create`

Replace the entire `#ifdef` chain inside `uni_platform_init` with the custom platform path hardcoded:

```c
void uni_platform_init(int argc, const char** argv) {
    // Hardcoded to custom platform — uni_platform_set_custom() must be
    // called before uni_init().
    if (!platform_) {
        while (1)
            loge("Error: call uni_platform_set_custom() before calling uni_main\n");
    }
    logi("Platform: %s\n", platform_->name);
    platform_->init(argc, argv);
}
```

</details>

<details>
<summary><strong>Patch 6 — Your Own Components</strong></summary>

**Remove `esp_log` and `freertos` from all `REQUIRES` lists** — they are implicit in IDF v6.0 and cause "unknown component" errors if declared.

**`MACSTR` macro** — can no longer be used in string literal concatenation. Replace:

```c
ESP_LOGI(TAG, "MAC: " MACSTR, MAC2STR(e->mac));
```

With:

```c
#include "esp_mac.h"
ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(e->mac));
```

**Backslash in `//` comments** — `-Werror=comment` treats lines ending in `\` as errors. Add a space before any trailing backslash in ASCII art comments.

**`esp_timer_get_time()`** — add `#include "esp_timer.h"` if used.

</details>

---

### Bluepad32 v4.2.0 API

This version does not have `bluepad32.h`. Use `uni.h` instead. The platform struct is `struct uni_platform` (not a typedef). The data callback must use `uni_controller_t*` assigned to `.on_controller_data`:

<details>
<summary><strong>controller.c integration example</strong></summary>

```c
#include "uni.h"
#include "uni_init.h"
#include "platform/uni_platform.h"
#include "platform/uni_platform_custom.h"
#include "controller/uni_gamepad.h"

static void on_controller_data(uni_hid_device_t *d, uni_controller_t *ctl) {
    uni_gamepad_t *gp = &ctl->gamepad;
    // gp->axis_x, gp->axis_y   — left stick  (-512 to 511)
    // gp->axis_rx, gp->axis_ry — right stick (-512 to 511)
    // gp->throttle              — left trigger (0–1023)
}

static struct uni_platform s_platform = {
    .name                   = "ESP32Drone",
    .init                   = NULL,
    .on_init_complete       = NULL,
    .on_device_discovered   = NULL,
    .on_device_connected    = on_connected,
    .on_device_disconnected = on_disconnected,
    .on_device_ready        = NULL,
    .on_gamepad_data        = NULL,
    .on_controller_data     = on_controller_data,
    .get_property           = NULL,
    .on_oob_event           = NULL,
    .device_dump            = NULL,
    .register_console_cmds  = NULL,
};

void controller_init(void) {
    uni_platform_set_custom(&s_platform);
    uni_init(0, NULL);
}
```

</details>

---

### Configuration Files

#### `sdkconfig.defaults` — create in project root

```
CONFIG_BLUEPAD32_PLATFORM_CUSTOM=y
CONFIG_BT_ENABLED=y
CONFIG_BT_CLASSIC_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=n
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

`CONFIG_BLUEPAD32_PLATFORM_CUSTOM=y` is essential — without it the linker will fail looking for `uni_platform_unijoysticle_create`.

#### `partitions.csv` — create in project root

The bluepad32 binary exceeds the default 1MB factory partition:

```csv
# Name,   Type, SubType, Offset,   Size,  Flags
nvs,      data, nvs,     0x9000,   24K,
phy_init, data, phy,     0xf000,   4K,
factory,  app,  factory, 0x10000,  1500K,
coredump, data, coredump,,         64K,
```

---

### IDF v6.0 Driver Split — Quick Reference

| Old header                            | New `REQUIRES` entry                                                     |
| ------------------------------------- | ------------------------------------------------------------------------ |
| `driver/gpio.h`                       | `esp_driver_gpio`                                                        |
| `driver/uart.h`                       | `esp_driver_uart`                                                        |
| `driver/i2s.h`                        | `esp_driver_i2s`                                                         |
| `driver/ledc.h`                       | `esp_driver_ledc`                                                        |
| `driver/spi_master.h` / `spi_slave.h` | `esp_driver_spi`                                                         |
| `driver/i2c.h`                        | `esp_driver_i2c` _(EOL in v6.0, removed in v7.0)_                        |
| `driver/timer.h`                      | **Fully removed** — migrate to `driver/gptimer.h` + `esp_driver_gptimer` |

<details>
<summary><strong>Warnings you can safely ignore</strong></summary>

- CMake warnings about lwip files "being built by btstack" — harmless
- `es8388.c` deprecation warning about `driver/i2c.h` EOL — harmless until IDF v7.0
- `esp_sleep_get_wakeup_cause` deprecated warning in `cmd_system.c` — harmless
</details>

---

## Connectivity

The project uses Ricardo Quesada's Bluepad32 library for Bluetooth gamepad input, supporting PS4, PS5, Xbox, Switch Pro, and most other modern controllers. WiFi is established as an AP so the MBL Quadcopter Telemetry Console (in `pythonscripts/`) can connect to manipulate PID constants and observe telemetry. The ESP32 acts as a UDP server on network `esp32drone`.

---

## ESC Calibration

_Steps may vary by ESC model._

1. Power the ESP32 only (no battery) while sending 2000µs PWM (max throttle)
2. Plug in the battery. A series of beeps should follow
3. While the ESC is powered, send 1000µs PWM (min throttle). A different series of beeps follows
4. Unplug the battery. Calibration complete

A successful calibration results in a single synchronized 3-beep series on power-up instead of varying patterns.

<details>
<summary>Failed Attempts</summary>

The lobodol ESC-Calibration library kept returning `Timed out waiting for packet header` on the ESP32 in both Arduino IDE and VSCode. We ended up using an Arduino UNO to perform initial calibration. When testing all four motors simultaneously with Arduino, at least one motor remained motionless. Switching back to the ESP32 with our own code resolved this — all four motors worked simultaneously without issue.

</details>

---

## Troubleshooting

### Build Commands

> **Run from:** `EDNA_2.0\` (project root)

```cmd
:: Source IDF — run every new terminal
C:\esp\v6.0\esp-idf\export.bat

:: First build or after fullclean
idf.py set-target esp32
idf.py build

:: Flash and monitor
idf.py -p COM3 flash monitor

:: If fullclean refuses due to non-CMake build directory
rmdir /s /q C:\Users\Jarvis\Documents\code\EDNA_2.0\build
idf.py set-target esp32
idf.py build
```

> `idf.py fullclean` wipes `sdkconfig` — always follow it with `idf.py set-target esp32`.

### On First Boot

Watch the serial monitor for:

```
Bluepad32 initialized — waiting for gamepad connection
ESCs armed
WiFi + telemetry OK
All tasks started
```

Put your gamepad in pairing mode and confirm `Gamepad connected` appears before testing motor response.

---

## PID

The PID system averages per-motor pitch and roll PWM values to produce a single PWM output per motor: `(M1_roll + M1_pitch) / 2`. The MPU-6050 provides pitch, roll, and yaw rate via a Kalman filter. The flight controller runs at 100Hz on Core 1, with a hard safety cutoff that kills all motors if attitude exceeds `ANGLE_LIMIT_DEG`.

Tuning is done live via the telemetry console over WiFi — connect to `esp32drone` and run `MBL_QTC.py` from the `pythonscripts/` folder. Requires Python `tkinter`.
