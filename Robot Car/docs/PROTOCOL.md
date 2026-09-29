# MaixCam <-> ESP32 wire protocol

UDP, not TCP: a dropped command is fine (the next frame's command replaces it),
but a stalled TCP handshake mid-match is not. Keeps latency low and avoids
head-of-line blocking.

- **Port:** 8888 (change in both `maixcam/vision.py` and `esp32/include/config.h` together)
- **Direction:** MaixCam -> ESP32 only, one packet per processed frame (~15-30Hz depending
  on how heavy your vision pipeline is)
- **Payload:** JSON, one object per packet

```json
{
  "seq": 142,
  "rx": 0.31, "ry": -0.55,
  "rheading": 87.2,
  "tx": 0.10, "ty": -0.20,
  "push": true
}
```

| Field      | Meaning                                                                 |
|------------|--------------------------------------------------------------------------|
| `seq`      | Monotonically increasing counter. ESP32 uses this to detect dropped/stale/out-of-order packets, not for anything else. |
| `rx`, `ry` | Robot's own position, normalized to the field: `-1..1` on each axis, origin at field center. |
| `rheading` | Robot's heading in degrees (0 = facing the opponent's side), *as seen by the overhead camera* — used as a sanity check against the ESP32's own IMU estimate, not the primary heading source. |
| `tx`, `ty` | Target point to drive toward this frame (nearest block worth pushing, or a push-through point beyond it). Same normalized coordinate space as `rx`/`ry`. |
| `push`     | `true` while a block is in front of the plow and should be driven through; `false` while just repositioning. Lets the ESP32 use a different (usually slower, higher-torque) drive profile. |

## Fallback behavior (WiFi dropout risk from slide 9)

If the ESP32 doesn't receive a packet for **300ms**, it must stop trusting `tx`/`ty`
and fall back to a local behavior (e.g. slow forward sweep, or hold position) using only
its own IMU — see `LAST_PACKET_TIMEOUT_MS` in `esp32/include/config.h`. Don't let a stale
target coordinate drive the robot off the field on a dropout.

## Coordinate convention

Pick one and write it here before you calibrate anything — it's the #1 source of
"robot drives the wrong way" bugs when two people work on the two sides independently:

- Origin: field center
- +x: toward the opponent's boundary (the direction you're pushing blocks)
- +y: TODO — pick left or right and note it here
- heading 0°: TODO — pick and note it here
