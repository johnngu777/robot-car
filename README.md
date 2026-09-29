# Group 7 — Autonomous Block-Pushing Robot

Split into two independently-flashed pieces that talk over WiFi/UDP:

- `maixcam/` — MicroPython (MaixPy) code that runs *on* the MaixCam Pro. Overhead vision:
  finds the blocks and the robot, decides where to push, sends short JSON commands.
- `esp32/` — PlatformIO/Arduino C++ firmware for the ESP32. Receives commands over WiFi,
  drives the 4 omniwheels via the encoded DC motors, reads the IMU, and runs a local
  fallback behavior if WiFi drops.
- `docs/PROTOCOL.md` — the wire format between the two, so both sides stay in sync as you
  change either one.

## Quick start

1. **MaixCam**: copy `maixcam/vision.py` onto the device (MaixVision IDE → open/run, or `mpremote cp`).
   Set `ESP32_IP` in the file to whatever the ESP32 prints on its serial monitor once it
   connects to your WiFi/AP.
2. **ESP32**: open `esp32/` in PlatformIO (VS Code extension), fill in the `// TODO` pin
   numbers and WiFi credentials in `esp32/src/main.cpp` and `esp32/include/config.h`, then
   `pio run -t upload`.
3. Watch the ESP32 serial monitor for its IP, punch that into the MaixCam script, run both.
