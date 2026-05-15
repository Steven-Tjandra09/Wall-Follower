# WallRider 

An ESP32-based autonomous wall-following robot using a VL53L0X time-of-flight sensor and dual ultrasonic sensors. A Qt desktop dashboard connects over Wi-Fi via WebSocket, visualising live sensor data and providing full manual and autonomous control.

---

## How it works

The robot runs a rule-based control loop every 100 ms. Two ultrasonic sensors mounted at a 17.5° angle measure the lateral distance to the wall — their readings are corrected with cos(17.5°) and averaged to compute a **side distance**, while their difference gives a **heading** that indicates whether the chassis is angled toward or away from the wall.
