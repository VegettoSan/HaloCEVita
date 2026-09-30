# Xbox cache resource loading on Vita — A033 source implementation

Status: **SOURCE IMPLEMENTED / HOST-RANGE TESTED; NOT YET VITA-VALIDATED**.

This note records the contract verified before enabling bitmap pixel reads from compressed Xbox maps. It does not claim that the Main Menu renders, that the new bridge links into a VPK, or that a retail bitmap has reached vitaGL on hardware yet.

## Verified original contract

The upstream path is preserved rather than replaced:

`bitmap_group / bitmap_data`
→ `texture_cache_bitmap_new()`
→ `texture_cache_start_loading_bitmap()`
→ `cache_file_read()`
→ destination resource buffer
→ original Xbox texture/D3D8 resource path
→ GL/vitaGL adaptation.

The important offset rule is:

```c
bitmap->pixels_offset += bitmap_group->pixel_data.file_offset;
```

Therefore the `pixels_offset` stored in a bitmap tag starts relative to `bitmap_group.pixel_data`, but after `texture_cache_bitmap_new()` it is an **absolute logical offset in the uncompressed Xbox `.map` address space**. `pixels_size` is the requested byte count. `base_address` is the destination resident buffer; it is not an offset base.

The Vita backend must consequently consume the `offset` passed to original `cache_file_read()` directly. It must not add the tag-cache base, tag-section offset, `pixel_data.file_offset` a second time, or any guessed resource-section base.

The original Xbox `cache_file_read()` rounds physical I/O to 512-byte sectors. That alignment belongs to the Xbox unbuffered transport and is not part of the bitmap payload contract. The Vita reader requests exactly `pixels_size` bytes.

## Logical address model for compressed maps

Xbox cache header bytes `[0, 0x800)` remain physically present and use the same logical offsets.

For a compressed cache, the zlib stream begins at physical offset `0x800`. Its decompressed output begins at logical offset `0x800`. The existing Vita tag reader already tracked this logical position while inflating `ui.map`; A033 factors that behavior into one checked range capture routine and reuses it for both tag-section reads and later resources.

There is **one decompression implementation**, not a tag decompressor plus a separate texture decompressor.

For a requested range `[logical_offset, logical_offset + size)` the compressed reader:

1. validates the cache header and requested bounds;
2. starts zlib at physical `0x800`;
3. tracks the produced logical position from `0x800`;
4. copies only the intersection with the requested range;
5. continues inflating to `Z_STREAM_END` even after the requested bytes have been copied;
6. accepts the read only if the zlib stream/checksum is valid and the final produced logical length equals the header's `logical_size`.

Continuing to the end is intentional correctness-first behavior: an early resource is not reported valid if the map's zlib stream is corrupt later in the file.

Uncompressed maps use the same logical-range API with checked `fseek`/exact reads.

## Vita bridge

`port/vita/src/vita_cache_bridge.c` implements the original game ABI:

```c
short cache_file_read(
    long tag_index,
    long offset,
    long size,
    void *buffer,
    boolean *completion_flag_reference,
    boolean blocking);
```

The first implementation is synchronous. `*completion_flag_reference` remains `FALSE` until the exact logical range has been read and validated; only then is it changed to `TRUE`. A failure leaves it false and emits `CACHE RESOURCE BLOCKED`.

This is deliberately not fake asynchronous completion. `cache_file_promote_read()` only diagnoses an unexpected request handle, and `cache_file_block_until_not_busy()` is a no-op while all successful Vita resource reads finish before `cache_file_read()` returns.

The resource reader is bound to the same validated `ui.map` path and header `logical_size` as the persistent tag mount. A later read rejects a map whose logical size no longer matches the validated mount.

No asset names, bitmap formats or menu resources are hardcoded.

## First retail bitmap instrumentation

The bridge observes the first real bitmap request issued through original `cache_file_read()`. If the original `tag_index`, mutated `pixels_offset` and `pixels_size` match a `bitmap_data` record, it logs:

- bitmap tag datum;
- tag path from the mounted original tag directory;
- bitmap index;
- absolute logical `pixels_offset`;
- `pixels_size`;
- bitmap format enum;
- width, height and depth;
- mipmap count;
- destination resource buffer;
- logical resource read PASS/FAIL.

Expected log prefixes are:

```text
[VITA CACHE] first original bitmap resource request:
[VITA CACHE] first original bitmap resource read result=
[VITA CACHE] original cache_file_read logical resource PASS:
CACHE RESOURCE BLOCKED:
```

Known Main Menu names such as `halo_logo` are useful only as external cross-checks. They are not used to select or load anything.

## GL upload boundary

A033 intentionally does **not** log a fake `GL upload result`.

The current baseline does not yet prove the complete original `xbox_textures`/D3D8 render closure on Vita. The correct next proof after a real bitmap resource request is observed is to connect that existing texture path and instrument the actual vitaGL texture upload result there. Until that code executes, `GL upload PASS` would be an unsupported claim.

Likewise this change does not repeat A032's forced `render_ui_widgets` closure experiment.

## Performance caveat

Correctness is prioritized before caching/async work. The first resource backend reopens the bound map and, for a compressed map, reinflates the zlib stream from `0x800` for each resource request so that every read is independently checked through `Z_STREAM_END`.

That is expected to be too expensive for the final texture streaming path. It is acceptable only as the first integration proof. After hardware evidence confirms the original resource contract, optimize without changing caller semantics, for example with a persistent sequential stream, validated decompressed window/cache, or a bounded resource cache appropriate for Vita memory.

Do not optimize by introducing a second manual asset system.

## Required Vita evidence

On the next console run, do not mark this path working merely because the source compiles. Capture:

1. the normal validated `ui.map` tag mount;
2. `cache ... resource reader bound`;
3. the first `[VITA CACHE] first original bitmap resource request` line;
4. its logical read PASS or exact `CACHE RESOURCE BLOCKED` error;
5. whether execution continues into the original texture/resource layer;
6. only after the real xgpu/vitaGL upload path is connected, the actual GL upload result.

A successful A033 test proves **resource bytes reached the original cache consumer**. It still does not by itself prove `HALO DRAW REACHED`, `HALO DRAW RENDERS`, or a visible Main Menu.
