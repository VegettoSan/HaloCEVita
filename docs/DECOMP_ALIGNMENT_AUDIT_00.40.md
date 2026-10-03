# Decomp alignment audit — 00.40

2026-10-03. Source audit, host execution and native build are distinct from
console acceptance. No Vita was attached during this session.

## Authority and scope

- Starting main: `6ac0718cfe516b17a46d22178335ccc02a4f5ec0` (00.39).
- Direct reference: [cybersecurity/halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal/tree/23b542601f2ca505c7a0143703e92fbda6075e18),
  commit `23b542601f2ca505c7a0143703e92fbda6075e18`.
- Imported baseline remains `21714ac0860e9b9ca08fbdc8a1d620f1b8a03797`.
  This work is a focused alignment, not a blanket upstream merge.
- Final code checkpoint: `50280c69a420b9474be44ecf018f5044cb3a90dd`.
- Read project instructions/status/attempts/issues/decisions, cache audit,
  renderer plan, compatibility/contracts and 00.39 renderer audit before editing.
  Compared the actual source and selected CMake owners with upstream, including
  Linux Xbox file/kernel services. No alternative port executable was used.

Classes describe ownership, not a claim that an entire file is byte-identical:
**A** original behavior; **B** original behavior with platform APIs;
**C** temporary replacement; **D** demonstrated platform incompatibility.
One system can contain more than one class. Hardware acceptance stays separate.

## Findings and changes actually applied

**Confirmed:** the original streaming decompressor reads the supplied compressed
Xbox v5 maps. The old D061 statement that it cannot do so was wrong. Actual C
execution produced the same logical header and complete inflated payload for
both read-only maps. Removing the parallel owner is therefore justified by
behavioral evidence, rather than by appearance.

The default build now includes `cache_files_windows.c` and
`cache_files_decompress_windows.c` through narrow Vita boundary units. Public
cache initialization, precache/status/end, slot/LRU selection, open/close,
resource queue, promotion/wait and completion belong to those original units.
`vita_cache_prepare_slot`, `vita_cache_resource_bind` and
`vita_cache_resource_read` have no place in the default ELF; the native verifier
rejects them and checks the public reader's worker wake call in ARM disassembly.
The old manager and synchronous reader remain recovery-only until hardware
acceptance. The default UI unit no longer includes the staged map handoff.

Added native positioned `WriteFileEx` beside `ReadFileEx`, using the existing
Linux-port issuing-thread TLS APC queue. Physical I/O is synchronous; completion
remains deferred until the issuing thread's alertable wait. This preserves Halo's
observable completion ownership without pretending Vita has an Xbox HDD/DMA API.
The original worker still controls all buffers, flags, waits and zlib sequencing.

Three narrow corrections remain explicitly different from upstream: deterministic
zeroed invalid slot headers; stream-end/exact-output/failure validation before
publishing a header; completion-before-flags sampling after blocking status sleep.
The host negative fixture demonstrates that the historical worker publishes a
valid header for an inflated stream longer than the advertised logical length.
These corrections keep the original worker and protocol.

Removed RAM-only rewriting of retail build strings. Both original native header
acceptance and warm-slot recovery accept retail builds as the Linux port does.
Kept bounded header strings and tag/logical-file/22MiB-arena checks at the original
header verifier before reads. Tags/BSP are still registered/rebased at their
original consumers, after original completion. No map on disk is rewritten.

Restored upstream DirectSound packet `SetEvent` completion: the staged fatal
replacement was obsolete now that native event services exist. Actual mixer
execution verifies callback precedence, consumer-thread completion and publication
of byte count/status before notification.

## System classification

| System / original owner | Class | Actual selection, divergence and remaining acceptance |
| --- | --- | --- |
| Entry `shell_xbox.c::main` | C | Compiled as `halo_original_main`; Vita `main.c` still owns startup and the outer loop. Full original entry is not executed. |
| `shell_initialize` | C/B | Native context/arena and split wrappers call original errors/math/game-state/rasterizer/input/sound owners. `tag_files_open` currently runs before `main_menu_load`, later than original shell order. A complete shell/main transition must establish one initialization/teardown owner and avoid double allocations. |
| `game_initialize` | A | Actual original function selected, not staged subset. Body matches audited current upstream after line-ending normalization. |
| `main_menu_load`, `main_load_ui_scenario`, `main_new_map`, `game_load` | A/B | Original owners selected; these bodies match audited upstream. Filesystem/address/GPU APIs are Vita boundaries. Linked ownership does not prove runtime new-map success. |
| `game_precache_new_map` / cache manager | B | Original callers and manager, six slots, language/checksum validation, active-slot protection and LRU policy. General Xbox I/O replaces volume APIs. |
| Streaming decompressor | B | Original scratch ownership, read sequences, zlib and completion/publication protocol. Native scratch span validation replaces page protection; narrow demonstrated validation/status defects are corrected. |
| `cache_file_open`, `cache_file_read` / completion routine | A/B | Original open and queue/worker/callback. Reader, free-slot selector and callback bodies match upstream. Original sector-rounded reads remain; no synchronous Vita public reader. |
| `scenario_tags_load`, `scenario_structure_bsp_load` | B/D | Original reads/waits/GPU registration; checked tag/BSP address conversion runs after completion. Fixed Xbox VA is unavailable on Vita. Full campaign/BSP acceptance remains pending. |
| `game_initialize_for_new_map` | A with upstream drift | Imported original owner retained. Current upstream has newer random-seed, distributed-network and render-interpolation reset ordering; no broad merge during this task. |
| `main_screen_shell_load` | A/B/D | Default original UI compilation undefines staging macro: original filesystem thread, profiles, roots, history, keyboard, music and attract decisions run. Upstream null Bink backend means movies are unavailable. |
| `main_loop` / map transition processing | C | Original loop source exists but shipping native harness calls UI update and pregame render only. Deferred Campaign request is not consumed by a substitute Vita transition. It needs promotion of original shell/main init, main-loop timing/world/network/pause and teardown together. |
| Dispose / restart | C | Default harness retains active game/UI/cache/arena owners until process exit; no accepted original `game_dispose`/`shell_dispose` integration or in-process restart. Introducing partial staged cleanup over a full live graph would be unsafe. |
| `process_ui_widgets`, event traversal / callback tables | A/B | Complete original functions/tables selected. Vita exposes controller state; desktop mouse traversal is excluded because there is no mouse. Recovery selector is excluded. |
| Focus, history, modal windows, navigation | A | Original widget state and events; no Vita focus choice, synthesized menu or per-widget positioning. Console coverage of all windows remains pending. |
| Keyboard / textbox / spinner / bitmap widgets | A/B | Original virtual keyboard, profile/game-data callbacks and recursive renderer. Vita has no Xbox debug keyboard; keystroke API reports no data. Physical input still uses XInput. |
| Authored coordinates, alpha and colors | A | Retail tag values retained. Current upstream additionally widens some flat full-screen fills for desktop wide screens; Vita retains original 640x480 logical layout. No symptom-based opacity or coordinate edits. |
| Controller / original input abstraction | B/D | `xinput_vita.c` translates native axes/buttons into the expected ABI and packet state. Original dead zones and event collection remain. Rumble returns unsupported; no motors. Device-0-only adapter and hotplug need console coverage. |
| Rasterizer / D3D8 / `d3d8_gl.c` | B | Original rasterizer/device/state path reaches existing GL translator and vitaGL; no alternate widget renderer. Details and actual-function regressions remain in 00.39 audit/contracts. |
| NV2A vertex/pixel equations | A/B/D | Original arithmetic, register indices, stages/combiners. Syntax/storage adapters omit unsupported invariant and bound existing register-array declarations. Multipass invariance and every retail program are not proven. |
| Texture stages / binding | B | Existing original stages; select unit before cold work and reapply complete original state afterwards. No stage selection by a Vita widget layer. |
| Blend / alpha test / culling | B | Original D3D state translation; native GL state only. No forced alpha/blend equation to conceal panels. Actual cold/warm state tests pass; visual faults remain open. |
| Depth / stencil / render targets | B/D | Original target/state identities with native renderbuffer/FBO boundary. vitaGL lacks original packed-depth texture path. Depth-only/sampling and cross-scene depth/stencil persistence are not accepted. |
| Viewport / scissor / logical resolution | B | 640x480 design space, currently 640x480 render policy (source/log correction A126) and 960x544 display. Conversion belongs to backend/presentation; authored tag coordinates are unchanged. |
| Indexed drawing / base vertex | B | Upstream `SetIndices` base retained; indexed stream address includes original base/minimum index. Bound stream/index transfers and reuse barriers adapt native fixed storage. Nonzero/large base and empty draws are tested. |
| Vertex streams / registers / shader constants | B/D | Original data/topology/indices, reflected whole-array native uniform uploads. Native capacity limits reject unrepresentable programs. No register renumbering or new equations. |
| Program / shader cache | B | Original cache and shader-pair ownership with native compile/link checks and effective-source dumps. Dynamic program coverage requires hardware. |
| `D3DDevice_Present` / swap | B | Original aspect-preserving composition, then one native swap. No second Vita blit. Full `main_loop` presentation timing is not yet exercised. |
| DXT1/3/5, mips and cube faces | B | Original decoder/resource loader and BGRA uploader retained from 00.39; no parallel Vita decoder. Mip/transfer/cube semantics tested on host, complete hardware scene residency not proven. |
| Volume textures / sampler capabilities | D | vitaGL dependency lacks required 3D upload/sampling. Nonzero LOD bias, border-color/anisotropy and visibility query capabilities remain documented gaps. Do not replace Halo shader equations with approximations. |
| Bitmap resource selection / cache | A/B | Original texture cache, serialized offsets and queue-owned resource reads. Native GPU address registration remains necessary. |
| Sound selection / music / triggers / volume / lifetime | A/B | Default uses full original game sound/cache/manager and existing DirectSound SDL backend; recovery-only menu sound wrappers are unselected. Native device failure is explicit. |
| Audio output / packet completion | B | SDL3 native output; upstream PCM/ADPCM/mixer and consumer completion. Original callback or event notification restored, no replacement game sound logic. |
| Spatial audio fidelity | D/upstream limitation | Existing upstream mixer does not model Doppler, high-frequency filters, cones or I3DL2 reverb completely. Native menu audio evidence does not prove gameplay/spatial audio. |
| Physical memory / game state | B/D | Hardware-proven movable 96MiB arena preserves original Xbox subregion offsets; XPhysicalAlloc address conversion/alignment checks. Vita cannot reserve the Linux fixed Xbox virtual window. Budgets for world maps remain open. |
| Tag accessors / nested pointers | B/D | Typed ownership/span checks translate serialized pointers only for active compiled image owners. No arbitrary word scanning. Existing top-level scenario/BSP rules remain; complete tag-class/gameplay coverage is unaccepted. |
| Dirty-page tracking / protection | B/D | Native memory-watch boundary differs from Xbox page protection. CPU-only decompressor scratch uses span validation because Ex copy finishes before return. Game-state save/dirty-page fidelity needs separate acceptance. |
| General Xbox filesystem / save enumeration | B | D: maps to `ux0:data/HaloCE/`; Z:/U:/T: to persistent subdirectories. Original APIs use native open/count/seek/pread/pwrite/truncate, case folding, times, find/move/delete. No cache-specific filesystem. Save/profile hardware tests remain required. |
| Events / threads / waits / clocks / priorities | B | Reused Linux-port pthread handles/TLS APCs with Vita timing boundary; scheduler priority is a valid-object hint as upstream. Contract tests do not measure real storage/thread latency. |
| Network / Bink / desktop integration | B/D | Native socket platform layer exists; native LAN/full world execution unaccepted. Upstream null Bink, no desktop pointer/invite UI backend. These are not complete Xbox API coverage. |

## Recovery code still present and why

`halo_cache_windows_recovery.c`, `vita_cache_read.c`'s old serialization/bind/read,
manual menu mount/journal, staged UI/event selector, staged audio guards and
`vita_menu_handoff.c` remain available for explicit recovery. The default selection
and ELF gates exclude the parallel owners. Keeping them until the new cache route
has console acceptance provides a reproducible rollback; it is not an endorsement
of retaining two shipping caches. Remove them once cold/warm UI/resource/audio
and error handling are accepted. Their maintenance risk is drift and accidental
selection; CMake guards and native forbidden-symbol checks bound that risk.

The harness remains necessary today because the full main/shell/timing/world and
safe teardown contract has not been demonstrated on Vita. Keeping it indefinitely
would hide Campaign progress and lifecycle faults. The next milestone is original
entry/main-loop integration after acceptance of this cache route, not a new Vita
Campaign state machine.

## Cache storage and exact read-only evidence

| Slot | Original capacity | Native location |
| --- | --- | --- |
| 000–001, solo | 278MiB each | `ux0:data/HaloCE/z/cache000.map`, `cache001.map` |
| 002, Main Menu | 35MiB | `ux0:data/HaloCE/z/cache002.map` |
| 003–005, multiplayer | 47MiB each | `ux0:data/HaloCE/z/cache003.map` through `cache005.map` |

Total: **732MiB / 767557632 bytes**, plus maps/saves. Initial preallocation may
be expensive; no startup/FPS improvement is promised. The original worker borrows
5316608 bytes of texture-cache scratch, instead of a parallel Vita inflate buffer.
Existing A116 exact-size caches at the data root are not adopted by the Z: volume
owner and are not silently deleted. Warm reuse follows original capacity,
language/source-checksum policy, rather than A116's stronger full-header comparison.
Successful I/O completion preserves original publication ordering; no additional
power-loss durability/fsync guarantee is claimed.

| Read-only input | Source SHA-256 | Logical bytes / SHA-256 from original worker |
| --- | --- | --- |
| supplied ui.map | `35e3e560478d85178749be310ad13d6d6ecde618d32675261a3554592333a833` | 33582080 / `8556b647b82742d07282fe4e6db0cf847b542c046484e91e71ec31fea5f5fd7e` |
| supplied bloodgulch.map | `50fe52406f075d975e24100a65b26ff696458023dd3509878953052ab0ef858f` | 44328960 / `9eb0374cbe4571af640ae0d2e0ebf6119543cdbbece44d49543b4748c4cf0dc1` |

Only logical header/payload bytes are compared; fixed-capacity trailing padding is
not game data. Neither map, extracted payload nor copyrighted assets are published.

## Verification and limitations

**HOST VERIFIED:** all29 required commands in `vita-build.yml` passed locally;
final CI282 contracts passed. New tests execute the actual decompressor/APC queue,
slot/LRU/open/warm recovery/header verifier and request/callback functions.
Synthetic2/4/8MiB streams cover complete4MiB output boundaries; malformed logical
length/checksum keep headers invalid. Optional supplied maps are tested read-only.
LP64 projection models 32-bit `long` for worker behavior and omits XDK sizeof
assertions; it is explicitly not ARM ABI or hardware timing evidence. Slot helper
tests model provider I/O and do not simulate the whole native threaded game.

**NATIVE BUILD VERIFIED:** final CI282/run37144066788 on50280c6 passed all
required jobs, `vita_verify.py` and release publication. Independent downloaded
artifact/clean-source/hash/88-symbol/ELF32 ARM hard-float/SELF/SFO00.40/LiveArea
checks pass; no parallel cache/handoff owners are linked. BUILD.md records the
exact VPK and matching release symbols. Earlier CI279–281 also passed.
The real `vitasdk/vitasdk:2026.08` toolchain owns ABI/layout/native package gates.
The unrelated imported desktop/Android workflow still fails on its missing guest
runtime dependency, as before; no all-platform green claim.

**REAL VITA VERIFIED:** pending for exact00.40. Previous hardware milestones do
not accept this new cache/threaded resource route. White panels, text placement,
all windows/profile keyboard, sustained music, cold/warm startup and stability
remain open. No Campaign/a10/playable gameplay or complete main-loop claim.

Hypotheses requiring evidence: original queued resource reads may reduce main
thread stalls; persistent warm slots may reduce preparation time. Neither is a
measurement. Shader/state visual faults may remain completely unchanged.

## Console test to return

Install the exact CI VPK named in BUILD.md. Keep retail maps under
`ux0:data/HaloCE/maps/`; allow at least732MiB for the six Z: slots, in addition to
map/save storage. Run cold and then warm without deleting the generated cache.
Capture logo/menu, repeated D-pad/Cross/Circle entry/back, Settings/profile/name
keyboard (Start confirms), at least60 seconds of music and Select exit. Campaign
entry is a UI test; it does not promise that a10 will load or gameplay will start.

Return each fresh `debug.txt`, `gamestate.txt` if generated, all original and
`*_native*.glsl` dumps, photos/video before/after input, startup timing, free
storage, and any Vita crash dump. Preserve the matching CI ELF/map. Logs should
show original cache initialize/copy END/open and, on a warm launch, warm VALID.
If it fails, return the last completed marker and full log; do not substitute
logs from another binary. Cache filenames/sizes/header metadata are useful;
no additional full retail map upload is needed.
