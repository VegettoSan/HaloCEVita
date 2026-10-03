#!/usr/bin/env python3
"""Run the actual original streaming worker over POSIX I/O and issuing-thread APCs.

LP64 hosts project source `long` to 32-bit `int` and omit XDK sizeof assertions;
the game algorithm is otherwise unchanged. This verifies behavior, not ARM ABI,
native page protection or hardware performance. CI's real SDK owns those gates.
No maps/assets are bundled. Optional map paths are inspected read-only.
"""
from pathlib import Path
import hashlib
import random
import re
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = (ROOT / 'source/cache/cache_files_decompress_windows.c').read_text()
    source = re.sub(r'^#include.*$', '', source, flags=re.M)
    source = re.sub(r'typedef char verify_[^;]+;', '', source)
    assert not re.search(r'\blong\s+long\b', source), 'projection requires a separate 64-bit type rule'
    source = re.sub(r'\blong\b', 'int', source)
    prelude = (ROOT / 'tools/fixtures/vita_cache_worker_host.h').read_text()
    # Use the production queue rather than freezing another APC implementation.
    kernel = (ROOT / 'port/linux/src/xbox_kernel.c').read_text()
    apcs = kernel.split('/* ---------- asynchronous procedure calls */')[1].split('/* ---------- waiting */')[0]
    prelude = prelude.replace('/* PRODUCTION_APCS */', apcs)
    tail = (ROOT / 'tools/fixtures/vita_cache_worker_main.c').read_text()
    with tempfile.TemporaryDirectory(prefix='halo-original-cache-') as tmp:
        work = Path(tmp)
        unit = work / 'worker.c'
        unit.write_text(prelude + source + tail)
        binary = work / 'worker'
        historical = work / 'historical'
        for target, flags in ((binary, ['-DHALO_VITA']), (historical, [])):
            subprocess.run(['gcc', '-std=c11', '-O0', '-Wno-int-to-pointer-cast', *flags,
                            str(unit), '-lz', '-pthread', '-o', str(target)], check=True)

        def run(path, target=binary, valid=True):
            output = work / 'cache002.map'
            capacity = {0: 0x11600000, 1: 0x02F00000, 2: 0x02300000}[struct.unpack_from('<h', path.read_bytes(), 96)[0]]
            result = subprocess.run([str(target), str(path), str(output), str(capacity)],
                                    capture_output=True, timeout=15)
            data = output.read_bytes()
            if valid:
                assert result.returncode == 0, result.stderr.decode()
                source_bytes = path.read_bytes()
                expected = source_bytes[:2048] + zlib.decompress(source_bytes[2048:])
                assert data[:len(expected)] == expected, 'original worker changed logical cache bytes'
                assert len(data) == capacity, 'original fixed slot capacity changed'
                print('PASS original copy:', path.name, 'logical=', len(expected),
                      'sha256=', hashlib.sha256(expected).hexdigest())
            else:
                assert not any(data[:2048]), 'failed copy published a valid header'
                assert result.returncode != 0, 'failed copy reported successful status'
            return data

        for count in (2 * 1024 * 1024, 4 * 1024 * 1024, 8 * 1024 * 1024):
            payload = random.Random(42).randbytes(count)
            header = bytearray(2048)
            struct.pack_into('<III', header, 0, 0x68656164, 5, 2048 + count)
            struct.pack_into('<I', header, 2044, 0x666f6f74)
            struct.pack_into('<h', header, 96, 2)
            encoded = zlib.compress(payload)
            path = work / 'ui.map'
            path.write_bytes(header + encoded)
            run(path)
            if count == 2 * 1024 * 1024:
                # Historical worker commits a length-mismatched payload. The
                # Vita validation boundary must keep its header invalid.
                struct.pack_into('<I', header, 8, 2048 + count // 2)
                path.write_bytes(header + encoded)
                old = subprocess.run(
                    [str(historical), str(path), str(work / 'historical.map'), str(0x02300000)],
                    capture_output=True, timeout=15)
                assert old.returncode == 0
                assert (work / 'historical.map').read_bytes()[:2048] == header
                run(path, valid=False)
                struct.pack_into('<I', header, 8, 2048 + count)
                corrupt = bytearray(encoded)
                corrupt[-1] ^= 1
                path.write_bytes(header + corrupt)
                run(path, valid=False)

        for argument in sys.argv[1:]:
            path = Path(argument)
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            run(path)
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'source map mutated'

    print('PASS: original decompressor/APC sequencing, exact logical images and full 4 MiB boundaries; malformed length/checksum never publish; historical failure reproduced')


if __name__ == '__main__':
    main()
