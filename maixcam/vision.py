"""
Overhead vision + strategy loop for the MaixCam Pro.
Finds the robot (by its own color marker) and the blocks, decides which
block to push next, and streams (robot pos, target pos) to the ESP32 as UDP/JSON.

Run this directly on the MaixCam (MaixVision IDE, or `mpremote run vision.py`).
See docs/PROTOCOL.md for the wire format and coordinate convention.
"""
from maix import camera, display, image, app, time
import socket
import ujson

# ---------------------------------------------------------------- config ---
ESP32_IP = "192.168.4.1"   # TODO: set to whatever the ESP32 prints on boot
ESP32_PORT = 8888

FRAME_W, FRAME_H = 320, 240

# LAB thresholds: [(L_min, L_max, A_min, A_max, B_min, B_max), ...]
# Don't guess these — use MaixVision's live "Find Color" tool under your actual
# match lighting and paste the values it gives you here.
BLOCK_THRESH = [(20, 80, 30, 90, 10, 80)]      # TODO: calibrate for your blocks
ROBOT_THRESH = [(10, 60, -30, 10, -80, -10)]   # TODO: calibrate for your robot's marker

MIN_BLOB_PIXELS = 40

# ------------------------------------------------------------- networking ---
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
seq = 0


def send_command(rx, ry, rheading, tx, ty, push):
    global seq
    seq += 1
    payload = ujson.dumps({
        "seq": seq,
        "rx": rx, "ry": ry,
        "rheading": rheading,
        "tx": tx, "ty": ty,
        "push": push,
    })
    sock.sendto(payload.encode(), (ESP32_IP, ESP32_PORT))


# ------------------------------------------------------------- geometry ---
def to_field_coords(px, py):
    """Pixel coords -> normalized field coords in [-1, 1]. TODO: replace with a
    proper homography once the camera is finally mounted (see calibrate.py) —
    a flat linear scale only works if the camera looks straight down."""
    nx = (px / FRAME_W) * 2 - 1
    ny = (py / FRAME_H) * 2 - 1
    return nx, ny


def pick_target(robot_xy, blocks_xy):
    """Nearest block to the robot. Swap for a smarter strategy once basic
    push-to-boundary works (e.g. prefer blocks already near your own boundary,
    or the one that's cheapest to reach without crossing others)."""
    if not blocks_xy:
        return None
    rx, ry = robot_xy
    return min(blocks_xy, key=lambda b: (b[0] - rx) ** 2 + (b[1] - ry) ** 2)


# ------------------------------------------------------------------ main ---
def main():
    cam = camera.Camera(FRAME_W, FRAME_H)
    disp = display.Display()

    while not app.need_exit():
        img = cam.read()

        robot_blobs = img.find_blobs(ROBOT_THRESH, pixels_threshold=MIN_BLOB_PIXELS,
                                      area_threshold=MIN_BLOB_PIXELS, merge=True)
        block_blobs = img.find_blobs(BLOCK_THRESH, pixels_threshold=MIN_BLOB_PIXELS,
                                      area_threshold=MIN_BLOB_PIXELS, merge=True)

        for b in block_blobs:
            img.draw_rect(b.x(), b.y(), b.w(), b.h(), color=image.COLOR_RED)
        for b in robot_blobs:
            img.draw_rect(b.x(), b.y(), b.w(), b.h(), color=image.COLOR_BLUE)

        if robot_blobs:
            robot_blob = max(robot_blobs, key=lambda b: b.pixels())
            robot_xy = to_field_coords(robot_blob.cx(), robot_blob.cy())
            # TODO: real heading needs either a two-blob marker (front/back dots)
            # or blob.angle() from a rotation-tracked blob — 0.0 is a placeholder.
            robot_heading = 0.0

            blocks_xy = [to_field_coords(b.cx(), b.cy()) for b in block_blobs]
            target = pick_target(robot_xy, blocks_xy)

            if target is not None:
                dist = ((target[0] - robot_xy[0]) ** 2 + (target[1] - robot_xy[1]) ** 2) ** 0.5
                pushing = dist < 0.08  # TODO: tune — "close enough to be driving through it"
                send_command(robot_xy[0], robot_xy[1], robot_heading,
                             target[0], target[1], pushing)

        disp.show(img)


if __name__ == "__main__":
    main()
