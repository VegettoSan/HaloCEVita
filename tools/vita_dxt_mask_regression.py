#!/usr/bin/env python3
"""Execute the original DXT decoder, including optional read-only retail masks.

Synthetic cases verify alpha placement, color order, odd dimensions, depth
and output guards. An optional Xbox ui.map compares all main-menu DXT3 alpha
statistics with the independent Python audit. No proprietary data is saved.
"""
import ctypes
from pathlib import Path
import struct
import subprocess
import sys
import zlib

from vita_ui_bitmap_audit import dxt_alpha, inspect

ROOT = Path(__file__).resolve().parents[1]
src = (ROOT / 'port/linux/src/xbox_textures.c').read_text()
enum = src[src.index('enum texel_kind'):src.index('struct format_information')]
colors = src[src.index('static unsigned long expand5'):src.index('static unsigned char clamp_byte')]
decoder = src[src.index('static unsigned long color565'):src.index('static GLenum compressed_format')]
decoder = decoder[:decoder.rfind('#endif')]
code = '#include <stddef.h>\ntypedef int BOOL;\n#define TRUE 1\n#define FALSE 0\n' + enum + colors + decoder + r'''
void decode(unsigned kind, const unsigned char *source, unsigned long w,
    unsigned long h, unsigned long d, unsigned long *out) {
    dxt_decode_level(kind == 14 ? _texel_dxt1 : kind == 15 ? _texel_dxt3 : _texel_dxt5,
        source, w, h, d, out);
}
'''
build = ROOT / 'build/vita/tests/dxt-mask'
build.mkdir(parents=True, exist_ok=True)
(build / 'decode.c').write_text(code)
subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
                str(build / 'decode.c'), '-o', str(build / 'decode.so')], check=True)
fn = ctypes.CDLL(str(build / 'decode.so')).decode
fn.argtypes = [ctypes.c_uint, ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong,
               ctypes.c_ulong, ctypes.c_void_p]


def decode(raw, w, h, fmt, depth=1):
    n = w * h * depth
    guard = 0x12345678
    out = (ctypes.c_ulong * (n+8))(*([guard] * (n+8)))
    buf = ctypes.create_string_buffer(raw)
    fn(fmt, buf, w, h, depth, ctypes.byref(out, 4*ctypes.sizeof(ctypes.c_ulong)))
    assert list(out[:4]) == [guard]*4 and list(out[n+4:]) == [guard]*4
    return list(out[4:n+4])


# Red color565, four-color mode, index0 throughout. Output words are ARGB,
# matching the uploader's GL_BGRA + UBYTE contract on the native ARM32 target.
color = struct.pack('<HHI', 0xF800, 0, 0)
alphas = sum(i << (4*i) for i in range(16)).to_bytes(8, 'little')
block = alphas + color
pixels = decode(block, 4, 4, 15)
assert pixels == [(i*17 << 24) | 0xFF0000 for i in range(16)]
assert decode(block, 3, 3, 15) == [pixels[y*4+x] for y in range(3) for x in range(3)]
two = decode(block*4, 3, 5, 15, 2)
assert two[:15] == two[15:] == [pixels[(y % 4)*4+x] for y in range(5) for x in range(3)]
# DXT1 punch-through is alpha0 only for palette index3 in c0 <= c1 mode.
assert decode(struct.pack('<HHI', 0, 0xFFFF, 0xFFFFFFFF), 4, 4, 14) == [0]*16
assert all(p >> 24 == 255 for p in decode(struct.pack('<HHI', 0xFFFF, 0, 0xFFFFFFFF), 4, 4, 14))
# DXT5 endpoint modes and all eight indices; these remain native on Vita,
# but enabling the shared decoder must preserve their existing Android path.
bits = sum((i % 8) << (3*i) for i in range(16)).to_bytes(6, 'little')
for a0, a1 in [(255, 0), (0, 255)]:
    table = [a0, a1] + ([((8-i)*a0 + (i-1)*a1)//7 for i in range(2, 8)] if a0 > a1
                        else [((6-i)*a0 + (i-1)*a1)//5 for i in range(2, 6)] + [0, 255])
    out = decode(bytes([a0, a1])+bits+color, 4, 4, 16)
    assert [p >> 24 for p in out] == table*2
print('PASS actual original DXT decoder: ARGB/BGRA, every DXT3 alpha nibble, odd edges, depth, guards, DXT1 punch-through and DXT5 endpoint modes')

if len(sys.argv) > 1:
    path = Path(sys.argv[1])
    audit = inspect(path)
    disk = path.read_bytes()
    size, = struct.unpack_from('<I', disk, 8)
    data = disk[:2048] + zlib.decompress(disk[2048:]) if len(disk) < size else disk
    offset, length = struct.unpack_from('<II', data, 16)
    tags = data[offset:offset+length]
    table, _, _, count = struct.unpack_from('<4I', tags)
    expected_names = {x['name'] for x in audit['masks']}
    checked = 0
    for i in range(count):
        group, _, _, _, label, ptr, _, _ = struct.unpack_from('<8I', tags, table-0x803A6000+i*32)
        if group != 0x6269746D:
            continue
        name = tags[label-0x803A6000:][:256].split(b'\0')[0].decode()
        if name not in expected_names:
            continue
        n, addr, _ = struct.unpack_from('<3I', tags, ptr-0x803A6000+96)
        for j in range(n):
            raw = tags[addr-0x803A6000+j*48:][:48]
            w, h, _, _, fmt = struct.unpack_from('<5h', raw, 4)
            po, ps = struct.unpack_from('<2I', raw, 24)
            payload = data[po:po+ps]
            alpha = [p >> 24 for p in decode(payload, w, h, fmt)]
            actual = dict(min=min(alpha), max=max(alpha), zero=alpha.count(0),
                          opaque=alpha.count(255), pixels=len(alpha))
            assert actual == dxt_alpha(payload, w, h, fmt), (name, j)
            checked += 1
    assert checked == len(audit['masks'])
    print(f'PASS {checked} original main-menu masks: C decoder vs independent alpha audit; read-only map SHA-256 {audit["sha256"]}')
