#!/usr/bin/env python3

import sys
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

INPUT_FILE  = 'example_bmp_serial_pixel_data__160x120.txt'
WIDTH       = 160
HEIGHT      = 120
BLOCK_W     = 16
BLOCK_H     = 8

def read_pixels(path):
    """Read whitespace-separated hex words into a list of ints."""
    try:
        with open(path, 'r') as f:
            tokens = f.read().split()
    except FileNotFoundError:
        print(f"Error: file not found: {path}", file=sys.stderr)
        sys.exit(1)
    pixels = []
    for tok in tokens:
        try:
            pixels.append(int(tok, 16))
        except ValueError:
            pass  # silently skip anything that isn’t valid hex
    return pixels

def rgb565_to_rgb888(p):
    """Convert one 16-bit RGB565 word to an (R, G, B) tuple in 0-255."""
    r = (p >> 11) & 0x1F
    g = (p >>  5) & 0x3F
    b =  p        & 0x1F
    return (
        (r * 255) // 31,
        (g * 255) // 63,
        (b * 255) // 31
    )

def rebuild(pixels):
    """Reassemble flat pixel stream into an H×W×3 ndarray."""
    img = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)
    idx = 0
    total = WIDTH * HEIGHT

    for yb in range(0, HEIGHT, BLOCK_H):
        h = min(BLOCK_H, HEIGHT - yb)
        for xb in range(0, WIDTH, BLOCK_W):
            w = min(BLOCK_W, WIDTH - xb)
            for dy in range(h):
                for dx in range(w):
                    if idx >= len(pixels):
                        raise ValueError(f"Ran out of data at tile ({xb},{yb})")
                    img[yb + dy, xb + dx] = rgb565_to_rgb888(pixels[idx])
                    idx += 1

    if idx != total:
        print(f"Warning: used {idx}/{total} pixels", file=sys.stderr)
    return img

def main():
    px = read_pixels(INPUT_FILE)
    needed = WIDTH * HEIGHT
    if len(px) < needed:
        print(f"Error: only {len(px)} pixels (need {needed})", file=sys.stderr)
        sys.exit(1)

    out = rebuild(px)

    # Display on a canvas
    plt.figure(figsize=(WIDTH/100, HEIGHT/100), dpi=100)
    plt.imshow(out)
    plt.axis('off')
    plt.tight_layout(pad=0)
    plt.show()

if __name__ == '__main__':
    main()
