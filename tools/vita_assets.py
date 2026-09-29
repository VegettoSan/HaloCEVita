#!/usr/bin/env python3
"""Generate original opaque, indexed PNG-8 LiveArea assets.

VitaSDK samples require indexed palettes for installer images. RGBA/type 6
can be a valid PNG yet fail promotion with 0x8010113D. Our authored artwork
uses exactly two RGB colors, so no lossy quantizer or extra dependency is needed.
"""
from pathlib import Path
import struct
import sys
import zlib

out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
def png(path, width, height, icon=False):
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            # Author-created geometric H glyph, independent of Halo branding.
            mark = icon and ((width//4 <= x < width//3 or 2*width//3 <= x < 3*width//4)
                             and height//4 <= y < 3*height//4 or
                             width//4 <= x < 3*width//4 and 7*height//16 <= y < 9*height//16)
            rows.append(1 if mark else 0)
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    palette = bytes((12, 30, 48, 170, 220, 245))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0))
                    + chunk(b'PLTE', palette)
                    + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))
png(out / 'icon0.png', 128, 128, True)
png(out / 'bg.png', 840, 500)
png(out / 'startup.png', 280, 158, True)
