## 📁 Project Architecture

<details>
<summary><strong>main/</strong> — Entry point & top-level application logic</summary>

| File | Description |
|------|-------------|
| `main.c` | App entry point. Initializes all subsystems and starts FreeRTOS tasks. |
| `config.h` | Global constants: pin mappings, PWM limits, PID defaults, WiFi SSID. |
| `CMakeLists.txt` | IDF build manifest — registers all component dependencies. |

</details>

---

<details>
<summary><strong>components/flight_controller/</strong> — PID loop, motor mixing, stabilization</summary>

| File | Description |
|------|-------------|
| `pid.c` / `pid.h` | Generic PID controller struct. Computes output from error, integral, and derivative terms. |
| `flight_controller.c` / `.h` | Runs pitch/roll/yaw PID loops. Averages and distributes corrected PWM to each motor. |
| `motor_mix.c` / `motor_mix.h` | Quadcopter motor mixing table — translates throttle + PID outputs to M1–M4 values. |

</details>

---

<details>
<summary><strong>components/imu/</strong> — MPU-6050 driver & sensor fusion</summary>

| File | Description |
|------|-------------|
| `mpu6050.c` / `mpu6050.h` | I²C driver for the MPU-6050. Reads raw accel/gyro registers; applies 90° axis offsets for mounting orientation. |
| `kalman.c` / `kalman.h` | Kalman filter implementation. Fuses gyro integration with accelerometer data to output stable angle estimates. |

</details>

---

<details>
<summary><strong>components/esc/</strong> — PWM generation & ESC calibration</summary>

| File | Description |
|------|-------------|
| `esc.c` / `esc.h` | Configures LEDC PWM channels for M1–M4. Sets throttle via microsecond pulse width (1000–2000 µs). |
| `esc_calibration.c` / `.h` | Calibration routine: outputs max throttle, waits, then outputs min throttle per the ESC calibration procedure. |

</details>

---

<details>
<summary><strong>components/controller/</strong> — Gamepad input via Bluepad32</summary>

| File | Description |
|------|-------------|
| `controller.c` / `controller.h` | Bluepad32 callback wrapper. Normalizes stick axes to throttle/pitch/roll/yaw setpoints passed to the flight controller. |

</details>

---

<details>
<summary><strong>components/telemetry/</strong> — WiFi UDP server & live data streaming</summary>

| File | Description |
|------|-------------|
| `wifi_server.c` / `wifi_server.h` | Starts the ESP32 as a WiFi AP + UDP server. Handles incoming PID tuning commands from the ground station. |
| `telemetry.c` / `telemetry.h` | Packages IMU angles, motor PWM values, and PID state into UDP packets for the QTC console. |

</details>

---

<details>
<summary><strong>PythonScripts/</strong> — Ground station telemetry console</summary>

| File | Description |
|------|-------------|
| `MBL_QTC.py` | Tkinter GUI — connects to `esp32drone` WiFi, displays live telemetry, and allows real-time PID gain editing. |
| `udp_client.py` | UDP socket helper used by MBL_QTC. Sends and receives JSON packets to/from the ESP32. |

</details>

---

### Root files

| File | Description |
|------|-------------|
| `README.md` | This document. Project overview, wiring, build steps, and PID notes. |
| `sdkconfig` | ESP-IDF Kconfig output. Stores board, Bluetooth, WiFi, and task stack settings. |
| `CMakeLists.txt` | Top-level CMake file. Registers component directories for the IDF build system. |