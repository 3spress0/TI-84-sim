#!/usr/bin/env python3
"""Generate MetroCE's original 16-pixel isometric pixel-art sprites."""
from pathlib import Path
from PIL import Image, ImageDraw
import random

ROOT = Path(__file__).resolve().parents[1] / "src" / "gfx"
ROOT.mkdir(parents=True, exist_ok=True)
TRANSPARENT = (255, 0, 128)
random.seed(84)


def canvas(height=8):
    return Image.new("RGB", (16, height), TRANSPARENT)


def diamond(draw, y=0, fill=(83, 139, 67), outline=None):
    points = [(8, y), (15, y + 3), (8, y + 7), (0, y + 3)]
    draw.polygon(points, fill=fill, outline=outline)
    return points


def grass(name, color=(83, 139, 67), detail=True):
    im = canvas()
    d = ImageDraw.Draw(im)
    diamond(d, fill=color, outline=(58, 105, 53))
    if detail:
        for _ in range(5):
            x, y = random.randrange(3, 13), random.randrange(2, 6)
            if abs(x - 8) / 8 + abs(y - 3.5) / 4 < 0.8:
                d.point((x, y), fill=random.choice([(103, 153, 72), (67, 123, 59), (129, 157, 75)]))
    im.save(ROOT / f"{name}.png")


def zone(name, color):
    im = canvas()
    d = ImageDraw.Draw(im)
    diamond(d, fill=(91, 130, 67), outline=color)
    for p in [(3, 3), (8, 1), (12, 4), (8, 6)]:
        d.point(p, fill=color)
    im.save(ROOT / f"{name}.png")


def building(name, height, roof, left, right, style="res", variation=0):
    h = max(16, height + 10)
    im = canvas(h)
    d = ImageDraw.Draw(im)
    base = h - 8
    diamond(d, base, fill=(78, 123, 62), outline=(49, 88, 46))
    a = (8, base - height)
    b = (14, base + 3 - height)
    c = (8, base + 7 - height)
    dd = (2, base + 3 - height)
    ag = (8, base)
    bg = (14, base + 3)
    cg = (8, base + 7)
    dg = (2, base + 3)
    d.polygon([dd, c, cg, dg], fill=left, outline=(43, 47, 47))
    d.polygon([b, c, cg, bg], fill=right, outline=(43, 47, 47))
    d.polygon([a, b, c, dd], fill=roof, outline=(45, 45, 43))

    # Repeating windows and floor lines give the tiny buildings their scale.
    floors = max(1, height // 4)
    for floor in range(floors):
        yy = base - height + 3 + floor * 4
        if yy > base + 2:
            continue
        if style == "com":
            wc1, wc2 = (162, 211, 209), (81, 145, 157)
        else:
            wc1, wc2 = (220, 213, 151), (75, 94, 96)
        for xx in (4, 6):
            d.point((xx, yy + (xx - 4) // 2), fill=wc1)
        for xx in (10, 12):
            d.point((xx, yy + (12 - xx) // 2), fill=wc2)
        if style == "ind":
            d.line((3, yy + 1, 7, yy + 3), fill=(86, 78, 65))

    # Rooftop machinery, chimneys, or a small pitched-roof detail.
    if style == "res" and height <= 9:
        d.line((5, base - height + 3, 8, base - height + 1, 11, base - height + 4), fill=(238, 195, 139))
        d.point((11, base + 2), fill=(73, 47, 33))
    elif style == "com":
        d.line((7, base - height + 2, 10, base - height + 4), fill=(213, 237, 225))
        d.rectangle((8, base - height, 9, base - height + 1), fill=(101, 111, 109))
    else:
        d.rectangle((10, base - height - 3, 11, base - height + 1), fill=(91, 79, 69))
        d.point((10, base - height - 4), fill=(167, 168, 151))
        if variation:
            d.point((5, base - height + 1), fill=(211, 183, 98))
    im.save(ROOT / f"{name}.png")


def tree(name, color):
    im = canvas(18)
    d = ImageDraw.Draw(im)
    d.line((8, 11, 8, 17), fill=(88, 64, 39), width=2)
    for box in [(3, 5, 12, 13), (5, 1, 11, 10), (1, 7, 9, 13)]:
        d.ellipse(box, fill=color, outline=(42, 96, 53))
    d.point((5, 4), fill=(139, 183, 92))
    d.point((9, 3), fill=(116, 167, 82))
    im.save(ROOT / f"{name}.png")


def special(name, kind):
    heights = {"power": 32, "water": 25, "sewage": 12, "park": 18,
               "police": 23, "fire": 23, "school": 22}
    im = canvas(heights[kind])
    d = ImageDraw.Draw(im)
    base = heights[kind] - 8
    diamond(d, base, fill=(77, 128, 63), outline=(47, 88, 44))
    if kind == "park":
        d.line((3, base + 3, 12, base + 5), fill=(190, 169, 118), width=2)
        for x, y in [(5, base - 8), (10, base - 12), (12, base - 5)]:
            d.line((x, y + 5, x, base + 4), fill=(85, 58, 34))
            d.ellipse((x - 3, y, x + 3, y + 7), fill=(61, 134, 64), outline=(39, 93, 45))
    elif kind == "water":
        d.line((5, base + 3, 7, base - 5), fill=(103, 108, 103))
        d.line((11, base + 3, 9, base - 5), fill=(103, 108, 103))
        d.rectangle((4, base - 11, 12, base - 5), fill=(104, 160, 175), outline=(49, 83, 96))
        d.ellipse((4, base - 14, 12, base - 8), fill=(151, 193, 196), outline=(49, 83, 96))
        d.line((6, base - 9, 10, base - 9), fill=(219, 231, 215))
    elif kind == "sewage":
        d.ellipse((2, base - 1, 13, base + 7), fill=(113, 118, 107), outline=(55, 63, 60))
        d.ellipse((4, base + 1, 11, base + 5), fill=(65, 123, 128))
        d.line((7, base, 8, base + 5), fill=(184, 181, 147))
    elif kind == "power":
        d.polygon([(3, base + 4), (5, base - 11), (10, base - 11), (13, base + 4)],
                  fill=(146, 142, 126), outline=(68, 68, 63))
        d.ellipse((5, base - 13, 10, base - 9), fill=(75, 79, 75))
        d.rectangle((11, base - 19, 13, base + 2), fill=(118, 91, 73))
        d.line((11, base - 15, 13, base - 15), fill=(218, 204, 167))
        d.point((12, base - 21), fill=(177, 179, 163))
    else:
        colors = {"police": ((107, 139, 164), (65, 91, 118), (196, 213, 213)),
                  "fire": ((164, 91, 71), (113, 61, 49), (221, 199, 160)),
                  "school": ((178, 139, 87), (120, 99, 71), (215, 191, 135))}
        right, left, roof = colors[kind]
        h = 12
        d.polygon([(2, base - h + 3), (8, base - h + 7), (8, base + 7), (2, base + 3)], fill=left)
        d.polygon([(14, base - h + 3), (8, base - h + 7), (8, base + 7), (14, base + 3)], fill=right)
        d.polygon([(8, base - h), (14, base - h + 3), (8, base - h + 7), (2, base - h + 3)], fill=roof)
        d.rectangle((6, base + 1, 8, base + 5), fill=(57, 65, 62))
        if kind == "fire":
            d.line((10, base - 4, 12, base - 3), fill=(238, 219, 186), width=2)
        elif kind == "police":
            d.point((11, base - 4), fill=(73, 158, 202))
        else:
            d.line((8, base - h, 8, base - h - 5), fill=(70, 70, 62))
            d.polygon([(9, base - h - 5), (13, base - h - 4), (9, base - h - 3)], fill=(194, 68, 55))
    im.save(ROOT / f"{name}.png")


for i, color in enumerate([(83, 139, 67), (86, 135, 66), (80, 132, 65), (91, 140, 70)]):
    grass(f"grass{i}", color)

# Road adjacency bits: west/east/north/south in map coordinates (screen: NW, SE, NE, SW).
# Roads run from the tile centre to the exact edge midpoints, which line up with the
# neighbouring tile's opposite midpoint, so adjacent roads join seamlessly.
def road_tile(mask, k):
    """k = 1 for 16x8 tiles, 2 for 32x16 tiles."""
    W, H = 16 * k, 8 * k
    im = Image.new("RGB", (W, H), TRANSPARENT)
    d = ImageDraw.Draw(im)
    d.polygon([(8 * k, 0), (16 * k - 1, 4 * k - 1), (8 * k, 8 * k - 1), (0, 4 * k - 1)],
              fill=(83, 139, 67), outline=(58, 105, 53))
    cx, cy = 8 * k, 4 * k - (1 if k == 1 else 1)
    mids = [(4 * k, 2 * k), (12 * k, 6 * k - 1), (12 * k, 2 * k), (4 * k, 6 * k - 1)]  # W, E, N, S bits
    asphalt, shoulder, paint = (62, 66, 72), (128, 130, 122), (216, 200, 108)
    t = 1.5 * k
    def band(m, half, color):
        d.polygon([(cx, cy - half), (m[0], m[1] - half), (m[0], m[1] + half), (cx, cy + half)], fill=color)
    for bit, m in enumerate(mids):
        if mask & (1 << bit):
            band(m, t + 1, shoulder)
    r = (3.5 if mask else 5) * k
    d.polygon([(cx - r, cy), (cx, cy - r / 2 - 1), (cx + r, cy), (cx, cy + r / 2 + 1)], fill=shoulder)
    for bit, m in enumerate(mids):
        if mask & (1 << bit):
            band(m, t, asphalt)
    r2 = r - 1.5 * k / 1.0
    d.polygon([(cx - r2, cy), (cx, cy - r2 / 2), (cx + r2, cy), (cx, cy + r2 / 2)], fill=asphalt)
    for bit, m in enumerate(mids):
        if mask & (1 << bit):
            d.line((cx, cy, (cx + m[0]) / 2, (cy + m[1]) / 2), fill=paint, width=1)
    return im


for mask in range(16):
    road_tile(mask, 1).save(ROOT / f"road{mask}.png")
    road_tile(mask, 2).save(ROOT / f"z2_road{mask}.png")

zone("zone_res", (121, 199, 109))
zone("zone_com", (93, 174, 206))
zone("zone_ind", (212, 178, 83))

for variation in range(2):
    building(f"res1_{variation}", 6 + variation, (170, 89, 63), (194, 173, 133), (157, 130, 98), "res", variation)
    building(f"res2_{variation}", 16 + variation * 3, (107, 91, 79), (166, 112, 86), (124, 85, 70), "res", variation)
    building(f"res3_{variation}", 30 + variation * 5, (85, 82, 77), (137, 101, 85), (99, 79, 72), "res", variation)
    building(f"com1_{variation}", 7 + variation, (88, 105, 111), (116, 158, 166), (78, 120, 137), "com", variation)
    building(f"com2_{variation}", 19 + variation * 3, (82, 92, 96), (92, 143, 157), (64, 106, 126), "com", variation)
    building(f"com3_{variation}", 36 + variation * 5, (107, 117, 115), (90, 139, 151), (55, 96, 120), "com", variation)
    building(f"ind1_{variation}", 7 + variation, (113, 105, 88), (157, 133, 92), (115, 101, 76), "ind", variation)
    building(f"ind2_{variation}", 15 + variation * 3, (91, 87, 79), (137, 112, 82), (102, 88, 70), "ind", variation)
    building(f"ind3_{variation}", 24 + variation * 4, (82, 81, 75), (128, 105, 78), (91, 80, 66), "ind", variation)

for i, color in enumerate([(56, 129, 62), (67, 143, 68), (47, 117, 59)]):
    tree(f"tree{i}", color)
for kind in ["power", "water", "sewage", "park", "police", "fire", "school"]:
    special(kind, kind)


# 2x zoom set: nearest-neighbour upscale of every sprite except the roads (drawn natively above).
for png in sorted(ROOT.glob("*.png")):
    if png.name.startswith("z2_"):
        continue
    im = Image.open(png)
    im.resize((im.width * 2, im.height * 2), Image.NEAREST).save(ROOT / f"z2_{png.name}")

files = sorted(p.name for p in ROOT.glob("*.png"))
yaml = """palettes:
  - name: global_palette
    fixed-entries:
      - color: {index: 0, r: 255, g: 0, b: 128}
      - color: {index: 1, r: 255, g: 255, b: 255}
      - color: {index: 2, r: 20, g: 31, b: 50}
      - color: {index: 3, r: 146, g: 174, b: 213}
      - color: {index: 4, r: 115, g: 125, b: 137}
      - color: {index: 5, r: 54, g: 62, b: 70}
      - color: {index: 6, r: 83, g: 139, b: 67}
      - color: {index: 7, r: 238, g: 205, b: 76}
      - color: {index: 8, r: 213, g: 74, b: 67}
      - color: {index: 9, r: 82, g: 184, b: 204}
      - color: {index: 10, r: 0, g: 0, b: 0}
      - color: {index: 11, r: 206, g: 194, b: 154}
    images: automatic

converts:
  - name: sprites
    palette: global_palette
    transparent-color-index: 0
    style: rlet
    images:
"""
yaml += "".join(f"      - {name}\n" for name in files)
yaml += """
outputs:
  - type: c
    include-file: gfx.h
    palettes:
      - global_palette
    converts:
      - sprites
"""
(ROOT / "convimg.yaml").write_text(yaml)
print(f"Generated {len(files)} isometric sprites in {ROOT}")

