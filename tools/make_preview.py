#!/usr/bin/env python3
"""Render a demonstration city with the same sprites used by the calculator build."""
from pathlib import Path
from PIL import Image, ImageDraw
import random

ROOT = Path(__file__).resolve().parents[1]
GFX = ROOT / "src" / "gfx"
random.seed(19)
W = H = 64
tiles = [[{"type": "empty", "level": 0, "v": random.randrange(20)} for _ in range(W)] for _ in range(H)]

for y in range(10, 55):
    for x in range(10, 55):
        if x % 4 == 0 or y % 4 == 0:
            tiles[y][x]["type"] = "road"
        elif random.random() < 0.79:
            distance = abs(x - 32) + abs(y - 32)
            if x < 23 and y > 38:
                kind = "ind"
                level = random.choices([1, 2, 3], [0.55, 0.4, 0.05])[0]
            elif distance < 14:
                kind = "com"
                level = random.choices([1, 2, 3], [0.12, 0.38, 0.5])[0]
            else:
                kind = "res" if random.random() < 0.8 else "com"
                level = random.choices([1, 2, 3], [0.42, 0.45, 0.13])[0]
            tiles[y][x]["type"] = kind
            tiles[y][x]["level"] = level
for x, y, kind in [(12, 12, "park"), (17, 17, "police"), (32, 22, "school"), (47, 47, "fire"),
                   (12, 52, "power"), (17, 52, "water"), (22, 52, "sewage"), (42, 17, "park")]:
    tiles[y][x] = {"type": kind, "level": 1, "v": 0}

im = Image.new("RGB", (320, 240), (20, 31, 50))
d = ImageDraw.Draw(im)


def sprite(name):
    s = Image.open(GFX / f"{name}.png").convert("RGBA")
    data = []
    for r, g, b, a in s.getdata():
        data.append((r, g, b, 0) if (r, g, b) == (255, 0, 128) else (r, g, b, a))
    s.putdata(data)
    return s


cache = {}
def paste(name, x, base_y):
    if name not in cache:
        cache[name] = sprite(name)
    s = cache[name]
    im.paste(s, (x, base_y + 8 - s.height), s)


cursor_x = cursor_y = 32
camx = 136
camy = 92 - (cursor_x + cursor_y) * 4
for total in range(W + H - 1):
    for x in range(max(0, total - H + 1), min(W - 1, total) + 1):
        y = total - x
        sx, sy = camx + (x - y) * 8, camy + (x + y) * 4
        if sx < -16 or sx >= 304 or sy < -8 or sy >= 232:
            continue
        t = tiles[y][x]
        kind, level, v = t["type"], t["level"], t["v"]
        if kind == "road":
            mask = 0
            if x and tiles[y][x - 1]["type"] == "road": mask |= 1
            if x < W - 1 and tiles[y][x + 1]["type"] == "road": mask |= 2
            if y and tiles[y - 1][x]["type"] == "road": mask |= 4
            if y < H - 1 and tiles[y + 1][x]["type"] == "road": mask |= 8
            paste(f"road{mask}", sx, sy)
            if (x * 7 + y * 11) % 11 == 0:
                d.rectangle((sx + 7, sy + 3, sx + 8, sy + 3), fill=(238, 205, 76))
            continue
        paste(f"grass{v % 4}", sx, sy)
        if kind == "empty":
            if v % 9 < 2: paste(f"tree{v % 3}", sx, sy)
        elif kind in ("res", "com", "ind"):
            paste(f"{kind}{level}_{v % 2}", sx, sy)
        elif kind != "empty":
            paste(kind, sx, sy)

# Sidebar, buttons, minimap, and status bar.
d.rectangle((288, 0, 319, 223), fill=(20, 31, 50))
labels = [("ROAD", (115, 125, 137)), ("ZONE", (83, 139, 67)), ("UTIL", (238, 205, 76)),
          ("SERV", (82, 184, 204)), ("TAX", (206, 194, 154)), ("PLAY", (83, 139, 67))]
for i, (label, color) in enumerate(labels):
    y = i * 28
    d.rectangle((290, y + 2, 317, y + 26), fill=(54, 62, 70))
    d.rectangle((292, y + 4, 315, y + 15), fill=color)
    d.text((293, y + 18), label, fill="white")
d.rectangle((288, 188, 319, 223), fill=(0, 0, 0))
for y in range(0, H, 2):
    for x in range(0, W, 2):
        kind = tiles[y][x]["type"]
        colors = {"empty": (83, 139, 67), "road": (115, 125, 137), "res": (255, 255, 255),
                  "com": (82, 184, 204), "ind": (238, 205, 76)}
        d.point((288 + x // 2, 190 + y // 2), fill=colors.get(kind, (213, 74, 67)))
d.rectangle((2, 2, 182, 19), fill=(20, 31, 50))
d.text((5, 3), "METROPOLIS     POP 24681", fill="white")
d.text((5, 12), "P100   W100   S100   H84", fill=(238, 205, 76))
d.rectangle((0, 224, 319, 239), fill=(20, 31, 50))
d.text((3, 227), "ROAD       Y1M4    $INF    RUN1    X32,32", fill="white")
im.resize((960, 720), Image.Resampling.NEAREST).save(ROOT / "preview.png")
print(ROOT / "preview.png")
