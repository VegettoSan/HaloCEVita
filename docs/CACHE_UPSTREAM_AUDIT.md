# Xbox-v5 cache / decompression audit

Date: 2026-10-03
Engineering record: **A116 / D062**

This document records the source audit that replaced HaloCEVita's temporary
`ui.map.vita-logical.tmp` resource backing with the original Halo cache-file
lifecycle adapted to Vita storage. It is intentionally separate from map data:
no retail `.map` file is committed or packaged.

A116/D062 explicitly supersede D036's per-bind temporary-file policy and the
cache-storage clause in D061. Their historical performance/runtime evidence
remains valid for the builds that used them; the original-runtime decision in
D061 remains otherwise active.

## Authority used for this audit

Primary source:

- repository: `https://github.com/cybersecurity/halo-ce-universal`
- audited upstream commit: `80d30410c8db28f4008b92f4e012a1b046ece14e`
- audit date: 2026-10-03

The audit did **not** use HaloCEVita's imported copies as evidence for original
behavior. The following files were read directly from the upstream commit:

- `source/cache/cache_files.c`
- `source/cache/cache_files_windows.c`
- `source/cache/cache_files_decompress_windows.c`
- `port/linux/port.json`
- `port/linux/src/xbox_files.c`

The original source imported into HaloCEVita remains the historical baseline
recorded by `.upstream-base-sha`; this audit does not silently re-import or merge
upstream source.

## What original Halo actually does

### Source maps and runtime cache files are different objects

The retail cache subsystem reads the source map from the `d:\maps` volume, but
runtime `cache_file_open` / `cache_file_read` do not consume the compressed DVD
map directly. Halo maintains six writable cache slots on `z:`:

| Scenario type | Original cache slots |
| --- | --- |
| solo / Campaign | `z:\cache000.map`, `z:\cache001.map` |
| Main Menu | `z:\cache002.map` |
| multiplayer | `z:\cache003.map` .. `z:\cache005.map` |

The current upstream Linux port keeps this subsystem instead of replacing it:
its file adapter maps `d:` to the map-data location and other Xbox drives,
including `z:`, to persistent writable host storage.

Therefore the existence of a decompressed disk copy is **not itself a Vita
workaround or corruption bug**. The important contract is when and how that
copy becomes valid and which file later resource reads use.

### Decompression/commit sequence

`cache_files_decompress_windows.c` uses the cache header as the publication
marker. In original order:

1. choose/preallocate a cache slot;
2. write a blank/invalid `0x800`-byte header to destination offset `0`;
3. read and verify the real source header;
4. start source input at offset `0x800`;
5. start destination payload output at offset `0x800`;
6. inflate the zlib stream while writing the exact logical payload;
7. wait for all payload writes to finish;
8. only after successful completion, write the **original source header** to
   destination offset `0`.

A partial copy therefore cannot look like a valid cache merely because a valid
header was written before the payload completed. The decompressor does not
rewrite tag structures, bitmap metadata, resource offsets or UI coordinates.
Its output is logically:

```text
original source header [0x000..0x7ff]
+
inflated source payload [0x800..logical EOF]
```

### Cache reuse/identity

The original cache manager associates a cached slot with the source map header
and checksum. `cache_file_open` selects a prepared slot; it is not the phase
that starts decompression. Closing the active map does not delete all cache
slots. The slots exist to be reused/replaced according to the cache policy.

## Verification against the supplied retail `ui.map`

The user-supplied `ui.map` was inspected read-only outside the repository.
No bytes were changed or committed.

Observed facts:

- source SHA-256:
  `35e3e560478d85178749be310ad13d6d6ecde618d32675261a3554592333a833`
- compressed file size: `14,145,536` bytes
- header logical `file_length`: `33,582,080` bytes
- zlib input begins at source offset `0x800`
- inflated payload length: `33,580,032` bytes
- `0x800 + 33,580,032 = 33,582,080` exactly
- zlib reaches stream completion successfully
- trailing bytes after the zlib stream: `27` bytes (padding outside the logical
  decompressed payload)
- `tag_data_offset`: `0x01e75c00`
- `tag_data_size`: `1,642,244` bytes
- header checksum: `0xcdc1a39a`
- scenario type: `2` (`_scenario_type_main_menu`)
- build string: `01.10.12.2276`
- SHA-256 of reconstructed logical cache image:
  `8556b647b82742d07282fe4e6db0cf847b542c046484e91e71ec31fea5f5fd7e`

Conclusion from these bytes: there is no evidence that the previous zlib
algorithm itself altered the map data. The payload expands to the exact logical
length declared by the source header and validates successfully. The previous
**lifecycle/storage architecture** nevertheless differed from Halo and was an
uncontrolled variable worth removing.

## Previous HaloCEVita divergence

A078/D036 introduced a process-owned `<map>.vita-logical.tmp` created during
resource binding. That solved repeated full-prefix inflation and was validated
for exact reads, but it differed from original Halo in several important ways:

- decompression was tied to resource bind/open rather than the precache phase;
- the derived file lived beside the source map for part of its history;
- it was deleted on unbind instead of behaving like a cache slot;
- the public cache API could treat a directly supplied compressed source as
  resource backing and materialize it implicitly;
- Main Menu tags could be loaded from one path while later resource reads were
  bound through a separately materialized path/lifetime.

None of those differences proves the white-panel/misplaced-widget symptom was
caused by corrupted bytes. They do make the storage lifetime unlike the engine
whose offsets/ownership we are trying to preserve.

D036's per-launch scratch policy and the storage clause in D061 are superseded
by A116/D062. Their historical performance evidence remains valid for the
builds that used them.

## Vita adaptation now implemented

The Vita filesystem cannot reproduce Xbox HDD/DVD hardware scheduling exactly,
but it can preserve the serialized cache contract.

### Persistent slots

HaloCEVita now maps the six original slots to:

```text
ux0:data/HaloCE/cache000.map
ux0:data/HaloCE/cache001.map
ux0:data/HaloCE/cache002.map
ux0:data/HaloCE/cache003.map
ux0:data/HaloCE/cache004.map
ux0:data/HaloCE/cache005.map
```

User source maps remain exclusively under:

```text
ux0:data/HaloCE/maps/*.map
```

Main Menu uses slot `002`; Campaign uses `000/001`; multiplayer uses `003..005`.
Legacy `ui.map.vita-logical.tmp` and `cache0.vita-logical.tmp` names are no
longer resource backing and are removed during cache initialization when found.

### Header-last publication

`vita_cache_prepare_slot` now:

1. validates the source Xbox-v5 header;
2. reuses an existing slot only if logical size and the entire `0x800` header
   match the source exactly;
3. otherwise truncates/creates the selected `cacheNNN.map`;
4. writes a blank `0x800` header;
5. copies or inflates the payload beginning at `0x800`;
6. requires exact logical payload length and successful zlib checksum;
7. flushes and checks final logical file size;
8. seeks to zero and writes the original source header last;
9. closes/reopens and verifies the committed image before exposing it.

On any failure Vita removes the invalid slot. Xbox normally keeps a preallocated
slot whose blank header marks it invalid; removal is the Vita storage-policy
adaptation, while the important validity rule (no published valid header on
partial data) is preserved.

### Resource reads

`vita_cache_resource_bind` now accepts only an already prepared, uncompressed,
exact-logical-size cache image. Attempting to bind a compressed source map is an
error. Runtime bitmap/audio/BSP requests seek the committed cache directly and
do not invoke zlib.

`cache_file_close` closes the active handle but leaves the cache slot on disk.

### Scenario transition

The Campaign handoff now creates/reuses and validates the proper solo slot
before releasing a working Main Menu. Only a committed slot allows the original
`game_load -> scenario_tags_load` path to continue. This also fixes the prior
state where selecting `levels\a10\a10` could wait forever because the Vita
handoff checked `cache_files_precache_map_loaded` without first starting a
precache operation.

## Intentional platform differences from Xbox

These are adaptations, not claims of bit-for-bit hardware scheduling:

- Vita precache is currently synchronous; Xbox uses its asynchronous DVD/HDD
  request worker and priority machinery.
- Vita cache files are stored at exact logical size; Xbox reserves/preallocates
  fixed cache-slot capacities.
- Vita uses 32 KiB zlib input/output scratch buffers rather than duplicating the
  exact Xbox request/staging buffer sizes.
- failed Vita cache creation removes the invalid slot instead of retaining a
  preallocated file with an invalid/blank header.

None of these changes alter the committed map bytes. Host tests compare the
complete committed file against the expected logical source image.

## Imported-source build-string compatibility

The source snapshot originally imported into HaloCEVita predates current
upstream's native-build acceptance and still checks the historical
`01.01.14.2342` string outside `HALO_LINUX` in `cache_files.c`.

To avoid conflating that source-version issue with cache serialization:

- `cacheNNN.map` preserves the retail source header byte-for-byte, including
  `01.10.12.2276`;
- only the in-memory header copy handed to the older imported engine is
  normalized to `01.01.14.2342` where still required.

A future focused upstream-source synchronization may remove that compatibility
view. It must not rewrite the user's map or the persistent cache header.

## Regression gates

`tools/vita_cache_regression.py` now executes the actual portable C code and
checks, among other cases:

- compressed and uncompressed source parsing;
- exact tag/resource ranges;
- `cache002.map == expected logical map` byte-for-byte;
- source file remains unchanged;
- valid persistent slot is reused without rewriting;
- compressed source cannot be used directly as runtime resource backing;
- corrupted zlib/checksum never publishes a cache;
- source-header/checksum identity changes invalidate a prior slot;
- unbind/close leaves a valid slot persistent.

`tools/vita_resource_seek_regression.py` additionally wraps real `inflate` and
`fwrite` calls to verify:

- inflation occurs during precache only;
- 128 subsequent random resource reads cause no new inflate calls;
- blank-header, payload-write and final-header-commit failures cannot leave a
  valid cache;
- persistent reuse performs no new writes/inflates;
- uncompressed sources use the same commit protocol without zlib.

Required host regressions and the complete native Vita ELF/SELF/VPK verifier
passed on code commit `a0954a60868dde4bd854b374c641faa7626e0e00`
(Vita Build run 261). Hardware rendering/gameplay acceptance remains separate.

## What this does and does not prove

### Proven by source/host evidence

- Original Halo uses persistent `z:\cacheNNN.map` files.
- Main Menu is assigned cache slot 2.
- The valid source header is committed after successful payload completion.
- The supplied `ui.map` zlib payload reconstructs to its exact declared logical
  size and validates successfully.
- New host regressions require the produced cache image to match expected
  logical bytes exactly.

### Not yet proven

- That the old `.tmp` architecture caused the visible white panels, text errors
  or misplaced UI elements.
- That the new cache lifecycle fixes those symptoms on Vita hardware.
- That complete Campaign/BSP/gameplay now succeeds after the cache boundary.

If the exact visual symptoms remain after this change while `cache002.map`
passes identity/size checks, the corruption hypothesis at the decompression
boundary is substantially weakened and investigation should continue above the
cache layer (typed relocation/runtime mutation, render target/state, vertex
coordinates, text/color/bitmap interpretation, etc.) rather than changing map
bytes or zlib output.
