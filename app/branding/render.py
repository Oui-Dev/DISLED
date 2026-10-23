# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 The DISLED contributors
"""Renders the DISLED logo and the installer images.

The logo is the lamp in isometric view, at the enclosure's dimensions: a
41 x 41 mm footprint, every edge rounded to 4.25 mm, the black base
11.5 mm high, the white cube up to 46 mm, lit green from inside. It is
ray-marched, so the rounded edges shade correctly.

From the repository root:

    uv run --with numpy --with pillow python app/branding/render.py
    cd app && npx tauri icon branding/logo.png

writes app/branding/logo.png (the source of every app icon) and, in
app/src-tauri/installer/, the Windows installer bitmaps and the macOS disk
image background. The images carry no text.
"""

from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

HERE = Path(__file__).resolve().parent
INSTALLER = HERE.parent / "src-tauri" / "installer"

HALF = 20.5  # half the footprint, mm
R = 4.25  # every edge
BASE_H = 11.5
TOP = 46.0
GREEN = np.array([0x2E, 0xCC, 0x71], dtype=np.float32)
WHITE = np.array([0xF7, 0xF8, 0xF6], dtype=np.float32)
BLACK = np.array([0x22, 0x24, 0x28], dtype=np.float32)
COS30 = np.cos(np.radians(30.0))
VIEW = np.array([1.0, 1.0, 1.0]) / np.sqrt(3.0)  # toward the camera
LIGHT = np.array([0.35, 0.6, 1.0]) / np.linalg.norm([0.35, 0.6, 1.0])


def rounded_box(x, y, z, z0, z1):
    """Signed distance to the footprint extruded from z0 to z1, every edge R."""
    qx = np.abs(x) - (HALF - R)
    qy = np.abs(y) - (HALF - R)
    d2 = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0)
    dz = np.abs(z - (z0 + z1) / 2) - ((z1 - z0) / 2 - R)
    return np.minimum(np.maximum(d2, dz), 0) + np.hypot(np.maximum(d2, 0), np.maximum(dz, 0)) - R


def lamp(x, y, z):
    # The cube's bottom edges and the base's top edges lie inside each
    # other: only the cube's top edges and the base's bottom edges show.
    return np.minimum(rounded_box(x, y, z, BASE_H - R, TOP), rounded_box(x, y, z, 0.0, BASE_H + R))


def render_lamp(size, margin):
    """The lamp on a transparent square, and the mask of its lit cube."""
    corners = np.array([[x, y, z] for x in (-HALF, HALF) for y in (-HALF, HALF) for z in (0.0, TOP)])
    sx = (corners[:, 0] - corners[:, 1]) * COS30
    sy = (corners[:, 0] + corners[:, 1]) / 2 - corners[:, 2]
    k = size * (1 - 2 * margin) / max(np.ptp(sx), np.ptp(sy))
    cx, cy = (sx.min() + sx.max()) / 2, (sy.min() + sy.max()) / 2

    col, row = np.meshgrid(np.arange(size) + 0.5, np.arange(size) + 0.5)
    u = (col - size / 2) / k + cx
    v = (row - size / 2) / k + cy
    # Each pixel's ray, parallel to VIEW, through the plane x + y + z = 0.
    origin = np.stack([v / 3 + u / (2 * COS30), v / 3 - u / (2 * COS30), -2 * v / 3])
    d = -VIEW
    lo = np.array([-HALF, -HALF, 0.0])
    hi = np.array([HALF, HALF, TOP])
    t_in = np.full(u.shape, -np.inf)
    t_out = np.full(u.shape, np.inf)
    for axis in range(3):
        a = (lo[axis] - origin[axis]) / d[axis]
        b = (hi[axis] - origin[axis]) / d[axis]
        t_in = np.maximum(t_in, np.minimum(a, b))
        t_out = np.minimum(t_out, np.maximum(a, b))
    rays = np.nonzero(t_out > t_in)  # rays through the bounding box
    o = origin[:, rays[0], rays[1]]
    t = t_in[rays]
    t_end = t_out[rays]
    hit = np.zeros(t.shape, dtype=bool)
    active = np.ones(t.shape, dtype=bool)
    for _ in range(120):
        a = np.nonzero(active)[0]
        if a.size == 0:
            break
        dist = lamp(*(o[:, a] + d[:, None] * t[a]))
        t[a] += dist
        hit[a[dist < 1e-3]] = True
        active[a[(dist < 1e-3) | (t[a] > t_end[a])]] = False

    h = np.nonzero(hit)[0]
    x, y, z = o[:, h] + d[:, None] * t[h]
    e = 1e-3
    n = np.stack([
        lamp(x + e, y, z) - lamp(x - e, y, z),
        lamp(x, y + e, z) - lamp(x, y - e, z),
        lamp(x, y, z + e) - lamp(x, y, z - e),
    ], axis=1)
    n /= np.linalg.norm(n, axis=1, keepdims=True)
    lambert = np.clip(n @ LIGHT, 0, 1)[:, None]
    reflect = 2 * (n @ LIGHT)[:, None] * n - LIGHT
    spec = (np.clip(reflect @ VIEW, 0, 1) ** 24)[:, None]

    # Cube: light from inside, greener low on the walls, blended through the
    # rounded edge into the top, which is paler in its middle.
    wall = 0.97 + (0.60 - 0.97) * np.clip((z - BASE_H) / (TOP - BASE_H), 0, 1)
    top = 0.48 + 0.16 * np.clip(np.hypot(x, y) / (HALF * 1.25), 0, 1)
    w_top = np.clip((n[:, 2] - 0.15) / 0.8, 0, 1) ** 1.5
    g = (wall * (1 - w_top) + top * w_top)[:, None]
    cube_rgb = (WHITE * (1 - g) + GREEN * g) * (0.86 + 0.14 * lambert)
    # Base: black, lit from above, the cube's light spilling on its top.
    spill = (0.55 * np.clip(z / BASE_H, 0, 1) ** 3)[:, None]
    base_rgb = (BLACK * (1 - spill) + GREEN * spill) * (0.45 + 0.75 * lambert) + 255 * 0.08 * spec

    cube = z >= BASE_H
    rgb = np.where(cube[:, None], cube_rgb, base_rgb)
    image = np.zeros((size, size, 4), dtype=np.uint8)
    rr, cc = rays[0][h], rays[1][h]
    image[rr, cc, :3] = np.clip(rgb, 0, 255).astype(np.uint8)
    image[rr, cc, 3] = 255
    mask = np.zeros((size, size), dtype=np.uint8)
    mask[rr[cube], cc[cube]] = 255
    return Image.fromarray(image, "RGBA"), Image.fromarray(mask, "L")


def logo(size, margin=0.08, halo=0.06):
    """The lamp with its green halo, drawn at twice the size then reduced."""
    big = 2 * size
    body, cube = render_lamp(big, margin)
    out = Image.new("RGBA", (big, big), tuple(int(c) for c in GREEN) + (0,))
    out.putalpha(cube.filter(ImageFilter.GaussianBlur(big * halo)))
    out.alpha_composite(body)
    return out.resize((size, size), Image.LANCZOS)


def vertical_gradient(width, height, top, bottom):
    t = np.linspace(0, 1, height)[:, None, None]
    rows = np.array(top, dtype=np.float32) * (1 - t) + np.array(bottom, dtype=np.float32) * t
    return Image.fromarray(np.repeat(rows, width, axis=1).round().astype(np.uint8), "RGB")


def nsis_sidebar(master):
    """Welcome and finish pages of the Windows installer: 164 x 314."""
    image = vertical_gradient(164, 314, (0x1E, 0x21, 0x26), (0x12, 0x13, 0x17)).convert("RGBA")
    mark = master.resize((150, 150), Image.LANCZOS)
    image.alpha_composite(mark, (7, 64))
    return image.convert("RGB")


def nsis_header(master):
    """Header of the other installer pages, on the left of the title: 150 x 57."""
    image = Image.new("RGBA", (150, 57), (255, 255, 255, 255))
    mark = master.resize((57, 57), Image.LANCZOS)
    image.alpha_composite(mark, ((150 - 57) // 2, 0))
    return image.convert("RGB")


def dmg_background():
    """The disk image window, 660 x 400: the app at x 180 and Applications at
    x 480 (Tauri's default positions), an arrow between them, the light of
    the lamp along the bottom."""
    width, height = 660, 400
    image = vertical_gradient(width, height, (0xF7, 0xF8, 0xFA), (0xEC, 0xEE, 0xF2)).convert("RGBA")
    glow = Image.new("L", (width, height), 0)
    ImageDraw.Draw(glow).rectangle((0, height - 18, width, height), fill=150)
    glow = glow.filter(ImageFilter.GaussianBlur(28))
    light = Image.new("RGBA", (width, height), tuple(int(c) for c in GREEN) + (0,))
    light.putalpha(glow)
    image.alpha_composite(light)
    scale = 4  # arrow drawn larger, then reduced, for smooth edges
    arrow = Image.new("L", (width * scale, height * scale), 0)
    draw = ImageDraw.Draw(arrow)
    y = 170 * scale
    draw.line((268 * scale, y, 382 * scale, y), fill=255, width=6 * scale)
    draw.polygon([(406 * scale, y), (378 * scale, y - 17 * scale), (378 * scale, y + 17 * scale)], fill=255)
    arrow = arrow.resize((width, height), Image.LANCZOS)
    color = Image.new("RGBA", (width, height), (0x9A, 0xA1, 0xAB, 0))
    color.putalpha(arrow)
    image.alpha_composite(color)
    return image.convert("RGB")


def main():
    INSTALLER.mkdir(exist_ok=True)
    master = logo(1024)
    master.save(HERE / "logo.png")
    nsis_sidebar(master).save(INSTALLER / "nsis-sidebar.bmp")
    nsis_header(master).save(INSTALLER / "nsis-header.bmp")
    dmg_background().save(INSTALLER / "dmg-background.png")


if __name__ == "__main__":
    main()
