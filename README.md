# 🏎️ ESP32 & PS4 Controller Powered 4WD Robot Car

This project features a high-performance, low-latency **4WD Robot Car** controlled via a physical **PS4 Controller (DualShock 4)** using an ESP32 microcontroller and an L298N motor driver. Developed specifically for university spring festivals and robotics competitions, the architecture focuses heavily on embedded systems optimization, hardware debugging, and robust power management.

---

## 🛠️ Tech Stack & Hardware Components

* **Microcontroller:** ESP32 Dev Module (Dual-core Tensilica Xtensa 32-bit LX6)
* **Motor Driver:** L298N Dual H-Bridge Motor Driver
* **Chassis:** 4WD Smart Car Chassis with 4 DC Gear Motors
* **Controller:** PS4 Controller (Hacked Bluetooth / MAC spoofing via Host Interface)
* **Framework / IDE:** Arduino IDE & Embedded C++
* **Core Libraries:** `PS4Controller.h`, `nvs_flash.h`

---

## ⚙️ System Architecture & Circuit Logic

### Hardware Wiring Layout

| L298N Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **ENA** | GPIO 13 | Left Motors Speed (PWM Channel 0) |
| **IN1** | GPIO 33 | Left Motors Forward (*Moved from GPIO 12*) |
| **IN2** | GPIO 14 | Left Motors Backward |
| **IN3** | GPIO 27 | Right Motors Forward |
| **IN4** | GPIO 26 | Right Motors Backward |
| **ENB** | GPIO 25 | Right Motors Speed (PWM Channel 1) |

> ⚠️ **CRITICAL GND NOTE:** The ESP32 GND and L298N GND must be tied together to establish a common reference point. Without this, signal floating will cause severe communication noise and unresponsiveness.

---

## 💡 Engineering Challenges & Solutions (Post-Mortem Log)

### 1. The GPIO 12 Strapping Pin Trap (Bootloop Fix)
* **Problem:** Initially, `IN1` was mapped to **GPIO 12**. During boot, ESP32 detected a voltage pull up/down on this pin due to the L298N internal logic, which forced the flash voltage flash register into 1.8V mode instead of 3.3V. This threw continuous `RTCWDT_RTC_RESET` errors and locked the chip in a bootloop.
* **Solution:** Re-mapped `IN1` to **GPIO 33**, a completely safe, non-strapping pin. This resolved the startup sequence crash permanently.

### 2. Voltage Sag & Brownout Crashing
* **Problem:** When high torque was demanded by the 4WD motors, the instantaneous current draw caused a massive voltage drop (sag) across the battery array. This dropped the ESP32 logic level voltage below 3.3V, causing the chip to trigger its internal brownout detector and disconnect frequently.
* **Solution:** * **Power Isolation:** Isolated the logic power supply (ESP32 via a clean power bank) from the motor power supply (L298N via high-discharge Li-ion batteries).
  * **Software Soft-Start:** Implemented a software low-pass filter to ramp up motor speeds progressively, preventing current spikes.

### 3. Non-Volatile Storage (NVS) Overcrowding
* **Problem:** Constant resetting and improper memory initializing threw `invalid header: 0xffffffff` errors, rendering the onboard flash unreadable.
* **Solution:** Integrated safe NVS initialization block routines inside the `setup()` block to clear Bluetooth pairing cache tables cleanly without causing flash wearing.

---

## 🚀 Key Software Features

* **Differential Tank Drive:** Implemented custom kinematics converting analog X/Y vectors into independent left/right track configurations.
* **Dynamic Nitro Boost:** Binding the `Circle` button dynamically scaling up the global PWM multiplier for maximum throttle tracking.
* **Hardware Safe-Stop:** The vehicle automatically executes a fallback `moveMotors(0, 0, 0)` vector instantly if the Bluetooth connection RSSI drops or a disconnect event occurs.
* **Software-Driven Cruise Control:** Toggling the `Triangle` button locks the current forward vector speed, automatically overriding via an interrupt if the brake trigger (`L2`) is initiated.

---

## 💻 How to Flash and Deploy

1. Ensure you have the `PS4Controller` library installed in your Arduino workspace.
2. Hard-reset your PS4 Controller and pair its master register to your ESP32's legal MAC Address using `SixaxisPairTool` (Windows) or `bluez / btmgmt` tools (Linux).
3. Open the source code sketch, verify your destination target board parameters, and configure your target flashing speed to **115200 baud** for minimal line noise.
4. Flash the code to your ESP32 board. 

```bash
# Optional Arch Linux shortcut to check serial connections
ls -l /dev/ttyUSB*
sudo chmod 666 /dev/ttyUSB0
