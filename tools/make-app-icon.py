#!/usr/bin/env python3
"""Draws the program icon resources/AppIcon/DrumProgrammer.png (512 x 512) and DrumProgrammer.ico with
Pillow (AP11).

The PNG is the icon of the program (juce_add_gui_app ICON_BIG, also on Windows) and of the AppImage, the ICO
the one of the Windows installer.
A snare drum seen slightly from above with two crossed sticks, in the blue of the backing waveform on the
dark background of the program.

Usage: tools/make-app-icon.py [repository root]
"""

import pathlib
import sys

from PIL import Image, ImageDraw

SIZE = 512
SCALE = 4  # drawn larger and scaled down, for smooth edges
BACKGROUND = (30, 34, 43, 255)
SHELL = (59, 130, 246, 255)
SHELL_DARK = (37, 99, 235, 255)
HEAD = (226, 232, 240, 255)
RIM = (147, 197, 253, 255)
STICK = (250, 204, 21, 255)
STICK_TIP = (254, 240, 138, 255)


def draw(size):
    s = size / 512.0
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=int(96 * s), fill=BACKGROUND)

    left, right = 96 * s, 416 * s
    top, bottom = 250 * s, 390 * s
    ry = 52 * s
    # Shell: the lower half ellipse and the rectangle up to the head.
    d.ellipse([left, bottom - ry, right, bottom + ry], fill=SHELL_DARK)
    d.rectangle([left, top, right, bottom], fill=SHELL)
    # Tension rods
    for i in range(1, 6):
        x = left + i * (right - left) / 6
        d.rectangle([x - 6 * s, top + 18 * s, x + 6 * s, bottom + 30 * s], fill=SHELL_DARK)
    # Head with rim
    d.ellipse([left - 6 * s, top - ry - 6 * s, right + 6 * s, top + ry + 6 * s], fill=RIM)
    d.ellipse([left + 8 * s, top - ry + 8 * s, right - 8 * s, top + ry - 8 * s], fill=HEAD)

    # Two crossed sticks above the head
    for (x0, y0, x1, y1) in [(120, 70, 300, 236), (392, 70, 212, 236)]:
        d.line([x0 * s, y0 * s, x1 * s, y1 * s], fill=STICK, width=int(22 * s))
        d.ellipse([x1 * s - 16 * s, y1 * s - 16 * s, x1 * s + 16 * s, y1 * s + 16 * s], fill=STICK_TIP)
    return image


def main():
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parent.parent)
    image = draw(SIZE * SCALE).resize((SIZE, SIZE), Image.LANCZOS)
    target = root / "resources" / "AppIcon" / "DrumProgrammer.png"
    image.save(target)
    image.save(target.with_suffix(".ico"), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print(target)


if __name__ == "__main__":
    main()
