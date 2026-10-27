"""Generate the three Pomodoro GIF assets (240x240, looped, stb_image-compatible).

rocket.gif - work session: rocket ascending through starry night sky
clouds.gif - short break: drifting clouds in a day sky
beach.gif  - long break: beach scene with animated waves
"""
import math
import os
import random

from PIL import Image, ImageDraw

SIZE = (240, 240)
FRAMES = 24
FRAME_MS = 100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")


def lerp(a, b, t):
    return int(a + (b - a) * t)


def vgrad(draw, w, h, top, bottom):
    for y in range(h):
        t = y / max(h - 1, 1)
        draw.line([(0, y), (w, y)], fill=(lerp(top[0], bottom[0], t),
                                          lerp(top[1], bottom[1], t),
                                          lerp(top[2], bottom[2], t)))


def save_gif(frames, name):
    path = os.path.join(OUT, name)
    frames[0].save(path, save_all=True, append_images=frames[1:],
                   duration=FRAME_MS, loop=0)
    kb = os.path.getsize(path) / 1024
    print(f"{name}: {len(frames)} frames, {kb:.0f} KB -> {path}")


# ---------------- rocket (transparent mascot, no background) ----------------
# Flat fully-opaque art on a transparent canvas -> exact palette mapping,
# zero halo. Used both as the always-on mini-widget mascot and the WORK gif.
def make_rocket():
    W, R, D = (255, 255, 255), (214, 48, 49), (45, 45, 60)  # white/red/outline
    BLUE, LBLUE = (90, 150, 235), (150, 205, 255)
    ORANGE, YELLOW = (255, 122, 26), (255, 214, 64)
    SMOKE, SMOKE_D = (168, 168, 185), (130, 130, 152)
    SPARK = (255, 240, 170)
    rgba = []
    for f in range(FRAMES):
        img = Image.new("RGBA", SIZE, (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        # twinkling plus-sparkles around the rocket (opaque, blink on/off)
        sparks = [(36, 52), (200, 40), (52, 180), (196, 190), (24, 118),
                  (216, 118), (120, 22), (120, 226)]
        for k, (sx, sy) in enumerate(sparks):
            if (f + k * 5) % 12 < 7:
                r = 5 if k % 2 == 0 else 4
                c = W if k % 3 else SPARK
                d.line([(sx - r, sy), (sx + r, sy)], fill=c, width=3)
                d.line([(sx, sy - r), (sx, sy + r)], fill=c, width=3)
        bob = int(8 * math.sin(f * 2 * math.pi / FRAMES))
        cx, cy = 120, 112 + bob
        # smoke puffs: opaque, rise + shrink + pop
        for k in range(3):
            t = ((f * 0.9 + k * 8) % 20) / 20
            r = int(11 * (1 - t) + 3)
            x = cx + int(9 * math.sin(k * 2.1 + f * 0.3))
            y = cy + 108 + int(t * 40)
            d.ellipse([x - r, y - r, x + r, y + r],
                      fill=SMOKE if k % 2 == 0 else SMOKE_D)
        # exhaust flame (solid, flickering length)
        flick = 0.75 + 0.25 * math.sin(f * 1.1) + 0.1 * math.sin(f * 3.7)
        fl = int(50 * flick) + 8
        d.polygon([(cx - 16, cy + 52), (cx + 16, cy + 52),
                   (cx, cy + 52 + fl)], fill=ORANGE)
        d.polygon([(cx - 9, cy + 52), (cx + 9, cy + 52),
                   (cx, cy + 52 + int(fl * 0.62))], fill=YELLOW)
        # fins
        d.polygon([(cx - 22, cy + 18), (cx - 22, cy + 52),
                   (cx - 44, cy + 58)], fill=R)
        d.polygon([(cx + 22, cy + 18), (cx + 22, cy + 52),
                   (cx + 44, cy + 58)], fill=R)
        # body outline (keeps white body visible over light wallpapers)...
        d.rounded_rectangle([cx - 25, cy - 49, cx + 25, cy + 55],
                            radius=16, fill=D)
        # ...body, nose, window
        d.rounded_rectangle([cx - 22, cy - 46, cx + 22, cy + 52],
                            radius=14, fill=W)
        d.rounded_rectangle([cx - 22, cy - 46, cx + 22, cy - 6],
                            radius=12, fill=R)
        d.ellipse([cx - 12, cy - 44, cx + 2, cy - 18], fill=(255, 130, 130))
        d.ellipse([cx - 13, cy - 2, cx + 13, cy + 24], fill=BLUE)
        d.ellipse([cx - 7, cy + 3, cx + 1, cy + 11], fill=W)
        d.rectangle([cx - 12, cy + 48, cx + 12, cy + 56], fill=(120, 120, 135))
        rgba.append(img)

    # exact palette: index 0 = transparent, rest = flat art colors
    colors = set()
    for img in rgba:
        for px in img.getdata():
            if px[3] >= 128:
                colors.add(px[:3])
    pal = sorted(colors)
    assert len(pal) <= 255, f"too many colors: {len(pal)}"
    lut = {rgb: i + 1 for i, rgb in enumerate(pal)}
    flat = [0, 0, 0]
    for rgb in pal:
        flat.extend(rgb)
    flat += [0] * (768 - len(flat))
    frames = []
    for img in rgba:
        p = Image.new("P", SIZE)
        p.putpalette(flat)
        p.putdata([0 if px[3] < 128 else lut[px[:3]]
                   for px in img.getdata()])
        frames.append(p)
    return frames


# ---------------- clouds (short break) ----------------
def cloud_shape(d, x, y, s, white=(255, 255, 255), shade=(214, 232, 245)):
    for ox, oy, r in [(-38, 8, 20), (-16, -6, 27), (8, -10, 30),
                      (32, -2, 24), (44, 10, 16)]:
        d.ellipse([x + (ox - r) * s, y + (oy - r) * s,
                   x + (ox + r) * s, y + (oy + r) * s], fill=white)
    d.rectangle([x - 52 * s, y + 2 * s, x + 54 * s, y + 24 * s], fill=white)
    d.rectangle([x - 52 * s, y + 14 * s, x + 54 * s, y + 24 * s], fill=shade)


def make_clouds():
    random.seed(21)
    birds = [(30 + i * 55, 40 + (i % 3) * 22) for i in range(4)]
    frames = []
    for f in range(FRAMES):
        img = Image.new("RGB", SIZE)
        d = ImageDraw.Draw(img)
        vgrad(d, 240, 240, (110, 190, 248), (224, 243, 255))
        # sun with pulsing glow
        sx, sy = 196, 42
        pulse = 26 + int(3 * math.sin(f * 2 * math.pi / FRAMES))
        for r, col in [(pulse + 16, (255, 244, 190)),
                       (pulse + 8, (255, 236, 150)),
                       (pulse, (255, 214, 90))]:
            d.ellipse([sx - r, sy - r, sx + r, sy + r], fill=col)
        # drifting clouds (wrap around)
        specs = [(0.55, 70, 9, 1), (0.35, 125, 14, -1), (0.8, 175, 11, 1)]
        for ci, (sp, base_y, speed, direction) in enumerate(specs):
            span = 360
            x = ((f * speed * direction + ci * 130) % span) - 60
            y = base_y + int(4 * math.sin(f * 0.4 + ci * 2))
            cloud_shape(d, x, y, sp)
        # birds: simple flapping "m" strokes drifting left
        for bi, (bx, by) in enumerate(birds):
            x = (bx - f * 2) % 260 - 10
            y = by + int(3 * math.sin(f * 0.6 + bi))
            flap = int(3 * math.sin(f * 1.2 + bi))
            d.arc([x - 8, y - 4 - flap, x, y + 4], start=200, end=340,
                  fill=(70, 90, 110))
            d.arc([x, y - 4 - flap, x + 8, y + 4], start=200, end=340,
                  fill=(70, 90, 110))
        frames.append(img)
    return frames


# ---------------- beach (long break) ----------------
def make_beach():
    frames = []
    for f in range(FRAMES):
        img = Image.new("RGB", SIZE)
        d = ImageDraw.Draw(img)
        vgrad(d, 240, 240, (125, 205, 250), (255, 240, 200))
        # sun + rotating rays
        sx, sy = 120, 78
        for r, col in [(40, (255, 246, 200)), (30, (255, 232, 140)),
                       (22, (255, 208, 90))]:
            d.ellipse([sx - r, sy - r, sx + r, sy + r], fill=col)
        for k in range(8):
            a = f * 0.08 + k * math.pi / 4
            x1, y1 = sx + int(34 * math.cos(a)), sy + int(34 * math.sin(a))
            x2, y2 = sx + int(46 * math.cos(a)), sy + int(46 * math.sin(a))
            d.line([(x1, y1), (x2, y2)], fill=(255, 220, 130), width=3)
        # sea
        d.rectangle([0, 118, 240, 178], fill=(38, 150, 205))
        d.rectangle([0, 118, 240, 126], fill=(90, 190, 230))
        for row in range(3):
            y = 136 + row * 13
            for x in range(0, 240, 4):
                off = int(4 * math.sin((x + f * (6 - row * 1.5)) * 0.09
                                       + row * 1.7))
                d.point((x, y + off), fill=(200, 240, 255))
        # foam edge sliding on the sand
        foam = int(4 * math.sin(f * 2 * math.pi / FRAMES))
        d.rectangle([0, 178 + foam - 3, 240, 184 + foam],
                    fill=(245, 252, 255))
        # sand
        d.rectangle([0, 184, 240, 240], fill=(244, 222, 158))
        d.rectangle([0, 184, 240, 188], fill=(238, 210, 140))
        # umbrella
        ux, uy = 62, 208
        d.line([(ux, uy), (ux, uy - 44)], fill=(150, 100, 60), width=4)
        for i, col in enumerate([(235, 90, 90), (255, 255, 255),
                                 (235, 90, 90), (255, 255, 255),
                                 (235, 90, 90), (255, 255, 255)]):
            x0 = ux - 34 + i * 11.4
            d.polygon([(ux, uy - 46), (x0, uy - 22),
                       (x0 + 11.4, uy - 22)], fill=col)
        d.ellipse([ux - 3, uy - 52, ux + 3, uy - 46], fill=(235, 90, 90))
        # beach ball with spinning stripes
        bx, by, br = 178, 214, 16
        d.ellipse([bx - br, by - br, bx + br, by + br],
                  fill=(255, 255, 255))
        for k in range(3):
            a0 = f * 0.25 + k * 2.09
            d.pieslice([bx - br, by - br, bx + br, by + br],
                       start=int(math.degrees(a0)),
                       end=int(math.degrees(a0) + 55),
                       fill=[(235, 90, 90), (90, 180, 90),
                             (90, 150, 235)][k])
        d.ellipse([bx - 5, by - 5, bx + 5, by + 5], fill=(255, 255, 255))
        d.ellipse([bx - br, by - br, bx + br, by + br],
                  outline=(200, 170, 120))
        frames.append(img)
    return frames


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    rocket = make_rocket()
    rpath = os.path.join(OUT, "rocket.gif")
    rocket[0].save(rpath, save_all=True, append_images=rocket[1:],
                   duration=FRAME_MS, loop=0, transparency=0, disposal=2)
    print(f"rocket.gif: {len(rocket)} frames, "
          f"{os.path.getsize(rpath) / 1024:.0f} KB (transparent) -> {rpath}")
    save_gif(make_clouds(), "clouds.gif")
    save_gif(make_beach(), "beach.gif")
