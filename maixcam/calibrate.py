"""
Threshold-finding helper: streams the image plus a live histogram-style readout
so you can point the camera at a block/your robot marker and read off good LAB
bounds instead of guessing them.

Prefer MaixVision IDE's built-in "Find Color" tool if available (it does this
with a nicer UI) — this script is the fallback if you're on-device only.
"""
from maix import camera, display, image, app

cam = camera.Camera(320, 240)
disp = display.Display()

print("Point the camera at the object, hold steady. Printing avg LAB every ~1s.")

frame_count = 0
while not app.need_exit():
    img = cam.read()
    frame_count += 1

    if frame_count % 30 == 0:
        # Sample a small box in the center of the frame — put the object there.
        cx, cy, size = 160, 120, 20
        stats = img.get_pixel(cx, cy)  # (varies by maix.image version; see wiki if this errors)
        print("center pixel:", stats)

    img.draw_rect(150, 110, 20, 20, color=image.COLOR_WHITE)
    disp.show(img)
