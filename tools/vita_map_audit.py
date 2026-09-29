#!/usr/bin/env python3
"""Read Xbox v5 bitmap metadata in place; never extract pixels or modify maps.

Layouts: source/cache/cache_files.c, source/tag_files/tag_groups.h and
source/bitmaps/bitmap_group.h. Bitmap types: source/bitmaps/bitmaps.c.
Only already-decompressed tag sections are supported by this host-side audit.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import struct

TAG_BASE = 0x803A6000
BITM = int.from_bytes(b"bitm", "big")


def inspect(path):
    with path.open("rb") as stream:
        header = stream.read(0x800)
        if len(header) != 0x800:
            raise ValueError("short cache header")
        magic, version, logical_size, _, offset, length = struct.unpack_from("<6I", header)
        if magic != int.from_bytes(b"head", "big") or version != 5:
            raise ValueError("not an Xbox v5 cache")
        if struct.unpack_from("<I", header, 0x7FC)[0] != int.from_bytes(b"foot", "big"):
            raise ValueError("invalid cache footer")
        result = {"file": path.name, "version": version,
                  "build": header[64:96].split(b"\0")[0].decode("ascii"),
                  "logical_size": logical_size, "disk_size": path.stat().st_size}
        if offset + length > result["disk_size"]:
            result["status"] = "SKIPPED: compressed tag section; no decompression attempted"
            return result
        if not 36 <= length <= 64 * 1024 * 1024:
            raise ValueError("tag section exceeds audit bounds")
        stream.seek(offset)
        tags = stream.read(length)

    def read(pointer, size):
        position = pointer - TAG_BASE
        if position < 0 or size < 0 or position + size > len(tags):
            raise ValueError(f"tag pointer out of bounds: {pointer:#x}, size={size}")
        return tags[position:position + size]

    def name(pointer):
        position = pointer - TAG_BASE
        read(pointer, 1)
        value = tags[position:position + 256]
        if b"\0" not in value:
            raise ValueError("unterminated tag name")
        return value.split(b"\0")[0].decode("ascii")

    table, _, _, count = struct.unpack_from("<4I", tags)
    if struct.unpack_from("<I", tags, 32)[0] != int.from_bytes(b"tags", "big"):
        raise ValueError("invalid tag section signature")
    read(table, count * 32)
    types = Counter()
    groups = 0
    volumes = []
    for index in range(count):
        group, _, _, _, label, data, _, _ = struct.unpack("<8I", read(table + 32 * index, 32))
        if group != BITM:
            continue
        groups += 1
        bitmap_count, bitmaps, _ = struct.unpack_from("<3I", read(data, 108), 96)
        if bitmap_count:
            read(bitmaps, bitmap_count * 48)
        for bitmap_index in range(bitmap_count):
            signature, width, height, depth, kind, fmt, _ = struct.unpack_from(
                "<I5hH", read(bitmaps + 48 * bitmap_index, 48))
            if signature != BITM or kind not in (0, 1, 2):
                raise ValueError("invalid bitmap signature/type")
            types[kind] += 1
            if kind == 1:
                volumes.append({"tag": name(label), "bitmap_index": bitmap_index,
                                "size": [width, height, depth], "format": fmt})
    result.update(status="METADATA ONLY", tag_count=count, bitmap_groups=groups,
                  bitmap_counts={"2d": types[0], "3d": types[1], "cube": types[2]},
                  volumes=volumes)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("maps", type=Path, nargs="+")
    args = parser.parse_args()
    failed = False
    for path in args.maps:
        try:
            result = inspect(path)
        except (OSError, ValueError, struct.error, UnicodeError) as error:
            result = {"file": path.name, "error": str(error)}
            failed = True
        print(json.dumps(result, sort_keys=True))
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
