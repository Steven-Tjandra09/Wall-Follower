# WallRider 

An ESP32-based autonomous wall-following robot using a VL53L0X time-of-flight sensor and dual ultrasonic sensors. A Qt desktop dashboard connects over Wi-Fi via WebSocket, visualising live sensor data and providing full manual and autonomous control.

---

## How it works

The robot runs a rule-based control loop every 100 ms. Two ultrasonic sensors mounted at a 17.5° angle measure the lateral distance to the wall, their readings are corrected with cos(17.5°) and averaged to compute a side distance, while their difference gives a **heading** that indicates whether the chassis is angled toward or away from the wall and a VL53L0X ToF sensor faces forward to detect obstacles.

### Control logic

| Priority | Condition | Drive | Steer |
|----------|-----------|-------|-------|
| 1 — Collision avoidance | Front obstacle < 20 cm | Reverse | Hard right |
| 2 — Proximity correction | Side distance < 13 cm (too close) | Forward | Right (away from wall) |
| 3 — Proximity correction | Side distance > 17 cm (too far) | Forward | Left (toward wall) |
| 4 — Heading alignment | On target, heading > +2.0 cm | Forward | Right |
| 4 — Heading alignment | On target, heading < −2.0 cm | Forward | Left |
| 5 — Perfect trajectory | All conditions met | Forward | Straight |

Target wall distance: **15 cm** — Tolerance: **±2 cm**

---

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 |
| Front obstacle sensor | VL53L0X (I²C — SDA: GPIO21, SCL: GPIO22) |
| Side sensors (×2) | HC-SR04 ultrasonic — front trig/echo: GPIO17/16, rear: GPIO19/18 |
| Drive motor | L298N channel A — IN1: GPIO25, IN2: GPIO26, ENA: GPIO33 |
| Steering motor | L298N channel B — IN3: GPIO27, IN4: GPIO14, ENB: GPIO32 |

---

## Qt dashboard

The desktop application is built with **Qt 6** and connects to the robot over the ESP32's access point. It provides:

- **Live scrolling charts** — three separate plots (Front ToF, Right Front US, Right Rear US) showing the last 5 seconds of sensor data at 100 ms resolution
- **Real-time readouts** — labelled cm values for all three sensors updated on every WebSocket frame
- **Mode indicator** — displays `AUTONOMOUS` or `MANUAL` based on the robot's current state
- **Manual drive controls** — Forward, Backward, Stop, Left, Right, Center buttons
- **Autonomous toggle** — Auto ON / Auto OFF buttons

### Qt dependencies

| Module | Purpose |
|--------|---------|
| `Qt6::Widgets` | Main window and UI controls |
| `Qt6::Charts` | QChart / QLineSeries live plots |
| `Qt6::WebSockets` | QWebSocket client |
| `Qt6::Core` | QJsonDocument parsing |

---

## Software & dependencies

**ESP32 firmware (Arduino IDE):**

- [Arduino framework for ESP32](https://github.com/espressif/arduino-esp32)
- [Adafruit VL53L0X](https://github.com/adafruit/Adafruit_VL53L0X)
- [WebSocketsServer (Links2004)](https://github.com/Links2004/arduinoWebSockets)
- [ArduinoJson](https://arduinojson.org/)

**Qt dashboard (Qt 6):**

- Qt6 Widgets, Charts, WebSockets, Core — install via [Qt Online Installer](https://www.qt.io/download)

---

## Getting started

### 1 — Flash the ESP32

1. Open `wall_follower.ino` in the Arduino IDE.
2. Install the ESP32 firmware dependencies above via the Library Manager.
3. Select your ESP32 board and port, then upload.

### 2 — Connect and run the Qt dashboard

1. Open the Qt project in Qt Creator and build it (Qt 6 required).
2. Connect your PC to the ESP32's Wi-Fi access point:
   - **SSID:** `ESP32_WallFollower`
   - **Password:** `password123!`
3. Launch the dashboard and click **Connect** — it will open a WebSocket connection to `ws://192.168.4.1:81`.
4. Use **Auto ON** to start autonomous wall-following, or drive manually with the direction buttons.

---

## WebSocket API

The ESP32 runs a WebSocket server on port **81**. Messages are JSON.

**Sending commands (client → robot):**
```json
{ "auto": 1 }        // enable autonomous mode
{ "auto": 0 }        // disable autonomous mode
{ "drive": 1 }       // forward  (manual mode only)
{ "drive": 2 }       // reverse
{ "drive": 0 }       // stop
{ "steer": 1 }       // right
{ "steer": 2 }       // left
{ "steer": 0 }       // center
```

**Telemetry broadcast (robot → client), every 100 ms:**
```json
{
  "front":  18.4,
  "usFront": 14.2,
  "usRear":  15.1,
  "isAuto":  1
}
```
