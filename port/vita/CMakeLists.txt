cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "Set VITASDK to the existing standard/native SDK (not SoftFP)")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE FILEPATH "Vita toolchain")
project(HaloCEVita C)
set(VITA_APP_VERSION "00.36")
option(HALO_VITA_MENU_BRINGUP "Start original Main Menu root after ui.map mount" ON)
option(HALO_VITA_ORIGINAL_RUNTIME "Run original UI/game lifetime and frame callers" ON)
option(HALO_VITA_MENU_UPDATE_PROBE "Link original process_ui_widgets closure (research only)" OFF)
option(HALO_VITA_MENU_RENDER_PROBE "Link original render_ui_widgets closure and rasterizer" ON)
option(HALO_VITA_MENU_AUDIO "Link original menu sound manager/cache and SDL mixer" ON)
if(HALO_VITA_MENU_AUDIO AND (NOT HALO_VITA_MENU_BRINGUP OR NOT HALO_VITA_MENU_RENDER_PROBE))
  message(FATAL_ERROR "Staged menu audio requires menu bring-up and renderer; disable HALO_VITA_MENU_AUDIO for other probes")
endif()
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(GLOB MUSL_MATH_SOURCES CONFIGURE_DEPENDS ${ROOT}/port/third_party/musl-math/src/*.c)

set(VITA_UI_WIDGET_SOURCE src/halo_ui_widget_vita.c)
set(VITA_UI_HANDLER_SOURCE src/halo_ui_event_handlers_vita.c)
if(HALO_VITA_ORIGINAL_RUNTIME)
  set(VITA_UI_WIDGET_SOURCE src/halo_ui_original.c)
  set(VITA_UI_HANDLER_SOURCE ${ROOT}/source/interface/ui_widget_event_handler_functions.c)
endif()

# Keep libc/Vita APIs separate from the game's 16-bit wchar and MSVC macros.
add_library(halo_core STATIC
  src/halo_core.c src/halo_menu_tags.c src/halo_services.c src/vita_cache_bridge.c src/xinput_vita.c src/xbox_memory_vita.c
  src/vita_memory_watch.c ${VITA_UI_WIDGET_SOURCE} src/halo_ui_events.c
  src/halo_ui_kernel.c src/halo_ui_file_search.c ${VITA_UI_HANDLER_SOURCE}
  ${ROOT}/source/cseries/cseries.c
  ${ROOT}/source/cseries/debug_memory.c
  ${ROOT}/source/cseries/profile.c
  ${ROOT}/source/math/random_math.c
  ${MUSL_MATH_SOURCES}
  ${ROOT}/source/memory/data.c
  ${ROOT}/source/memory/memory_pool.c
  ${ROOT}/source/memory/stack_memory_pool.c
  ${ROOT}/source/memory/crc.c
  ${ROOT}/source/tag_files/tag_groups.c
  ${ROOT}/source/tag_files/files.c
  ${ROOT}/source/tag_files/files_windows.c
  ${ROOT}/source/bitmaps/bitmap_group.c
  ${ROOT}/source/text/font_group.c
  ${ROOT}/source/text/text_group.c
  ${ROOT}/source/text/international_strings.c
  ${ROOT}/source/interface/player_ui.c
  ${ROOT}/source/networking/network_game_globals.c
  ${ROOT}/source/main/main.c
  ${ROOT}/source/editor/editor_stubs.c
  ${ROOT}/source/game/game.c
  ${ROOT}/source/game/game_engine.c
  "${ROOT}/source/saved games/player_profile.c"
  "${ROOT}/source/saved games/playlist_profile.c"
  "${ROOT}/source/saved games/saved_game_files.c"
  ${ROOT}/source/bungie_net/common/thread_win32.c
  ${ROOT}/port/linux/src/xbox_xapi.c
  ${ROOT}/source/interface/marketing_and_strategic_business_development.c
  ${ROOT}/port/linux/src/msvc_wide.c
  ${ROOT}/port/linux/src/halo_linker_common.c
  ${ROOT}/source/cache/cache_files.c
  ${ROOT}/source/cache/physical_memory_map.c
  "${ROOT}/source/saved games/game_state_xbox.c"
  ${ROOT}/port/linux/src/nv2a_vsh.c
  ${ROOT}/port/linux/src/nv2a_psh.c
  ${ROOT}/port/linux/src/xgpu_text.c)
# Original UI/rasterizer closure. It stays switchable for dependency diagnostics,
# but is enabled by default now that the complete closure links on Vita.
if(HALO_VITA_MENU_RENDER_PROBE)
  target_sources(halo_core PRIVATE
    src/halo_renderer_globals.c
    src/halo_renderer_diagnostics.c
    src/halo_renderer_runtime.c
    src/vita_texture_state.c
    ${ROOT}/source/bink/bink_playback.c
    ${ROOT}/source/cutscene/cinematics.c
    ${ROOT}/source/effects/particles.c
    ${ROOT}/source/effects/decals.c
    ${ROOT}/source/game/game_time.c
    ${ROOT}/source/interface/ui_widget_game_data_input_functions.c
    ${ROOT}/source/interface/ui_widget_text_search_and_replace_functions.c
    ${ROOT}/source/interface/interface.c
    ${ROOT}/source/text/draw_string.c
    ${ROOT}/source/text/unicode.c
    "${ROOT}/source/saved games/game_state.c"
    ${ROOT}/source/math/real_math.c
    ${ROOT}/source/math/matrix_math.c
    ${ROOT}/source/rasterizer/rasterizer.c
    ${ROOT}/source/rasterizer/rasterizer_lights.c
    ${ROOT}/source/rasterizer/rasterizer_geometry.c
    ${ROOT}/source/rasterizer/rasterizer_text.c
    ${ROOT}/source/rasterizer/rasterizer_cinematics.c
    ${ROOT}/source/rasterizer/rasterizer_debug.c
    ${ROOT}/source/rasterizer/rasterizer_memory_pool.c
    ${ROOT}/source/rasterizer/rasterizer_transparent_geometry.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_initialize.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_detail_objects.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_profile.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_environment_fog.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_errors.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_water.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_decals.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_widgets.c
    ${ROOT}/port/linux/src/d3d8_gl.c
    ${ROOT}/port/linux/src/d3d8_resources.c
    ${ROOT}/port/linux/src/xbox_textures.c
    ${ROOT}/source/cache/xbox_texture_cache.c
    ${ROOT}/source/memory/lruv_cache.c
    ${ROOT}/source/bitmaps/bitmaps.c
    ${ROOT}/source/math/integer_math.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_text.c
    ${ROOT}/source/rasterizer/rasterizer_swizzle.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_runtime.c
    ${ROOT}/source/rasterizer/rasterizer_frame_statistics.c
    ${ROOT}/source/interface/virtual_keyboard.c
    ${ROOT}/source/interface/hud_draw.c
    ${ROOT}/source/input/input_abstraction.c
    ${ROOT}/source/render/render.c
    ${ROOT}/source/render/render_cameras.c
    ${ROOT}/source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c
    ${ROOT}/source/interface/hud.c
    ${ROOT}/source/game/players.c
    ${ROOT}/source/game/player_control.c
    ${ROOT}/source/bitmaps/bitmap_utilities.c)
endif()
# Reintegrate the configured decomp units instead of synthesizing missing
# UI/world owners. The existing narrow target is retained as a recovery build
# while this original dependency closure is compiled and linked.
if(HALO_VITA_ORIGINAL_RUNTIME)
  file(READ "${ROOT}/config/config.json" ORIGINAL_CONFIG)
  string(JSON ORIGINAL_COUNT LENGTH "${ORIGINAL_CONFIG}" projects 0 objects)
  math(EXPR ORIGINAL_LAST "${ORIGINAL_COUNT}-1")
  get_target_property(EXISTING_CORE_SOURCES halo_core SOURCES)
  foreach(I RANGE 0 ${ORIGINAL_LAST})
    string(JSON ORIGINAL_SOURCE GET "${ORIGINAL_CONFIG}" projects 0 objects ${I} name)
    string(JSON ORIGINAL_STATUS GET "${ORIGINAL_CONFIG}" projects 0 objects ${I} status)
    if(ORIGINAL_SOURCE MATCHES "\\.c$" AND NOT ORIGINAL_STATUS STREQUAL "MISSING"
        AND NOT ORIGINAL_SOURCE MATCHES "source/interface/(ui_widget|event_manager|ui_widget_event_handler_functions)\\.c$"
        AND NOT ORIGINAL_SOURCE MATCHES "source/cseries/(cseries_windows|stack_walk_windows)\\.c$"
        AND NOT ORIGINAL_SOURCE STREQUAL "source/cache/cache_files_windows.c")
      if(NOT "${ROOT}/${ORIGINAL_SOURCE}" IN_LIST EXISTING_CORE_SOURCES)
        target_sources(halo_core PRIVATE "${ROOT}/${ORIGINAL_SOURCE}")
      endif()
    endif()
  endforeach()
  target_sources(halo_core PRIVATE
    src/vita_original_file_api.c src/halo_cache_windows_vita.c
    ${ROOT}/port/linux/src/bink_null.c
    ${ROOT}/port/linux/src/msvc_crt.c
    ${ROOT}/port/linux/src/xnet.c
    ${ROOT}/port/linux/src/port_config.c
    ${ROOT}/port/linux/game/network_distributed.c
    ${ROOT}/port/linux/game/network_objects.c
    ${ROOT}/port/linux/game/network_damage.c
    ${ROOT}/port/linux/game/render_interpolation.c)
  target_include_directories(halo_core PRIVATE ${ROOT}/port/third_party/tomlc17)
  set_source_files_properties(src/vita_original_file_api.c ${ROOT}/port/linux/src/bink_null.c
    ${ROOT}/port/linux/src/msvc_crt.c ${ROOT}/port/linux/src/xnet.c
    ${ROOT}/port/linux/src/port_config.c
    PROPERTIES COMPILE_DEFINITIONS HALO_LINUX_PLATFORM_LAYER)
  target_compile_definitions(halo_core PRIVATE HALO_VITA_ORIGINAL_RUNTIME=1)
  set_source_files_properties(${ROOT}/source/shell/shell_xbox.c
    PROPERTIES COMPILE_DEFINITIONS main=halo_original_main)
endif()

if(HALO_VITA_MENU_AUDIO)
  target_sources(halo_core PRIVATE
    src/halo_menu_audio.c src/halo_menu_audio_guard.c
    ${ROOT}/source/sound/game_sound.c
    ${ROOT}/source/sound/sound_manager.c
    ${ROOT}/source/sound/sound_definitions.c
    ${ROOT}/source/sound/sound_classes.c
    ${ROOT}/source/sound/sound_preferences.c
    ${ROOT}/source/sound/sound_environment_definitions.c
    ${ROOT}/source/sound/sound_dsound_xbox.c
    ${ROOT}/source/cache/xbox_sound_cache.c
    ${ROOT}/port/linux/src/dsound_sdl.c)
  target_compile_definitions(halo_core PRIVATE HALO_VITA_MENU_AUDIO=1)
  set_source_files_properties(${ROOT}/port/linux/src/dsound_sdl.c
    PROPERTIES COMPILE_DEFINITIONS HALO_LINUX_PLATFORM_LAYER COMPILE_OPTIONS
    "-std=gnu11;-U__STRICT_ANSI__;-Werror=implicit-function-declaration;-Werror=incompatible-pointer-types;-Werror=int-conversion;-Werror=return-type")
  set_source_files_properties(src/halo_menu_audio.c src/halo_menu_audio_guard.c
    PROPERTIES COMPILE_OPTIONS
    "-Werror=implicit-function-declaration;-Werror=incompatible-pointer-types;-Werror=int-conversion;-Werror=return-type")
endif()
set(SEMANTICS "${CMAKE_CURRENT_BINARY_DIR}/halo_msvc_semantics.h")
file(GLOB_RECURSE SEMANTIC_INPUTS CONFIGURE_DEPENDS ${ROOT}/source/*.h ${ROOT}/source/*.c ${ROOT}/port/include/xdk/*.h)
add_custom_command(OUTPUT ${SEMANTICS}
  COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/vita_semantics.py ${SEMANTICS}
  WORKING_DIRECTORY ${ROOT}
  DEPENDS ${ROOT}/tools/vita_semantics.py ${ROOT}/tools/linux_msvc_semantics.py ${SEMANTIC_INPUTS})
add_custom_target(vita_semantics DEPENDS ${SEMANTICS})
add_dependencies(halo_core vita_semantics)
target_compile_options(halo_core PRIVATE -std=gnu89 -fms-extensions -fshort-wchar -fno-short-enums -fsigned-char -fcommon
  -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -ffp-contract=off
  -fno-omit-frame-pointer -ffunction-sections -fdata-sections -O2 -g
  -Wno-multichar -Wno-unknown-pragmas -Wno-attributes
  -Wno-error=implicit-function-declaration -Wno-error=implicit-int
  -Wno-error=int-conversion -Wno-error=incompatible-pointer-types
  "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/halo_vita_prefix.h" "SHELL:-include ${SEMANTICS}"
  -idirafter ${ROOT}/port/include/xdk)
target_compile_definitions(halo_core PRIVATE __STRICT_ANSI__ DEBUG xbox)
# Use the same deterministic musl libm subset as the upstream native ports.
set_source_files_properties(${MUSL_MATH_SOURCES} PROPERTIES COMPILE_OPTIONS
  "-std=gnu11;-w;-I${ROOT}/port/third_party/musl-math/include;-include;${ROOT}/port/third_party/musl-math/include/libm.h")
# Exactly the game include paths used by upstream's halobetacache project.
file(READ "${ROOT}/config/config.json" CONFIG)
string(JSON INCLUDE_COUNT LENGTH "${CONFIG}" projects 0 options include_dirs)
math(EXPR INCLUDE_LAST "${INCLUDE_COUNT}-1")
target_include_directories(halo_core PRIVATE include ${ROOT}/port/vita/include ${ROOT}/port/linux/include ${ROOT}/port/linux/src)
foreach(I RANGE 0 ${INCLUDE_LAST})
  string(JSON DIR GET "${CONFIG}" projects 0 options include_dirs ${I})
  if(NOT DIR STREQUAL "xbox/include")
    target_include_directories(halo_core PRIVATE "${ROOT}/${DIR}")
  endif()
endforeach()
set_source_files_properties(src/halo_services.c src/xinput_vita.c src/xbox_memory_vita.c
  src/halo_ui_kernel.c src/halo_ui_file_search.c ${ROOT}/port/linux/src/xbox_xapi.c
  ${ROOT}/port/linux/src/msvc_wide.c
  ${ROOT}/port/linux/src/nv2a_vsh.c ${ROOT}/port/linux/src/nv2a_psh.c ${ROOT}/port/linux/src/xgpu_text.c
  PROPERTIES COMPILE_DEFINITIONS HALO_LINUX_PLATFORM_LAYER)
# d3d8_gl.c applies the complete original raster state after Vita texture
# lookup/upload. Keep xgpu_texture_get direct; a second blend-only restoration
# inside the lookup would create a competing owner for the same draw state.
# xbox_textures.c owns the decoded bytes and cache policy. On Vita keep its
# first allocation as glTexImage2D, but route later same-shape mutable updates
# through the bridge so vitaGL can copy-on-write with glTexSubImage2D instead
# of forcing a global glFinish + storage replacement for every cached glyph.
set_source_files_properties(${ROOT}/port/linux/src/xbox_textures.c
  PROPERTIES COMPILE_DEFINITIONS
  "glBindTexture=halo_vita_texture_bind;glDeleteTextures=halo_vita_texture_delete;glTexImage2D=halo_vita_texture_image_2d;glFinish=halo_vita_texture_upload_finish")
# Keep the shared decoded BGRA upload path. The 00.30 direct-internal-format
# experiment did not change the hardware rectangles, so do not retain it as a
# purported transparency fix.
set_source_files_properties(src/halo_menu_tags.c PROPERTIES COMPILE_OPTIONS
  "-Werror=implicit-function-declaration;-Werror=incompatible-pointer-types;-Werror=int-conversion;-Werror=return-type")

add_executable(HaloCE.elf src/main.c src/vita_platform.c src/vita_graphics.c src/vita_video_backend.c src/vita_gl_compat.c
  src/vita_renderer_platform.c src/vita_memory.c src/vita_cache_read.c src/vita_menu_relocate.c src/vita_cache_probe.c src/vita_xapi_files.c)
target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_APP_VERSION="${VITA_APP_VERSION}")
if(HALO_VITA_MENU_BRINGUP)
  target_compile_definitions(halo_core PRIVATE HALO_VITA_MENU_BRINGUP=1)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_BRINGUP=1)
endif()
if(HALO_VITA_ORIGINAL_RUNTIME)
  target_sources(HaloCE.elf PRIVATE src/vita_native_net.c
    ${ROOT}/port/linux/src/posix_files.c
    ${ROOT}/port/third_party/tomlc17/tomlc17.c)
  set_source_files_properties(${ROOT}/port/linux/src/posix_files.c
    PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)
  target_include_directories(HaloCE.elf PRIVATE ${ROOT}/port/linux/src ${ROOT}/port/third_party/tomlc17)
  target_link_libraries(HaloCE.elf PRIVATE SceNet_stub SceNetCtl_stub)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_ORIGINAL_RUNTIME=1)
endif()
if(HALO_VITA_MENU_UPDATE_PROBE)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_UPDATE_PROBE=1)
endif()
if(HALO_VITA_MENU_RENDER_PROBE)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_RENDER_PROBE=1)
  target_link_options(HaloCE.elf PRIVATE -Wl,-u,_rasterizer_initialize)
endif()
if(HALO_VITA_MENU_AUDIO)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_AUDIO=1)
endif()
if(HALO_VITA_MENU_AUDIO AND NOT HALO_VITA_ORIGINAL_RUNTIME)
  # These world/debug owners are unavailable in the staged UI-only runtime.
  # Fail explicitly if a spatial/object path escapes the 2D menu contract.
  foreach(OWNER compute_sound_obstruction track_object_impulse_sound
      game_sound_set_mouth_aperture player_effect_continuous_refresh
      observer_get_camera scenario_location_underwater render_debug_sphere
      render_debug_string_at_point render_debug_string sound_render_time sound_idle)
    target_link_options(HaloCE.elf PRIVATE "-Wl,--wrap=${OWNER}")
  endforeach()
endif()
target_include_directories(HaloCE.elf PRIVATE include)
target_compile_options(HaloCE.elf PRIVATE -std=gnu11 -Wall -Wextra -O2 -g -ffunction-sections -fdata-sections -ffp-contract=off)
# vita-elf-create appends SCE module/import metadata to the RX LOAD segment.
# Keep the constructor/destructor arrays beyond the original audio DSP image
# and the enlarged code closure, then start RW data on the following page. This
# preserves the original array contents/order while leaving stable conversion
# headroom as the original renderer grows.
target_link_options(HaloCE.elf PRIVATE
  -Wl,-q -Wl,--gc-sections
  -Wl,--section-start=.init_array=0x81600000
  -Wl,--section-start=.fini_array=0x8160001c
  -Wl,-Tdata=0x81610000
  "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/HaloCE.elf.map")
target_link_libraries(HaloCE.elf PRIVATE halo_core
  -Wl,--start-group vitaGL vitashark SceShaccCgExt SDL3 pthread stdc++ mathneon m z
  SceShaccCg_stub taihen_stub SceDisplay_stub SceCtrl_stub SceGxm_stub ScePower_stub
  SceKernelDmacMgr_stub SceIofilemgr_stub SceSysmem_stub SceProcessmgr_stub SceKernelThreadMgr_stub
  SceAudio_stub SceAudioIn_stub SceSysmodule_stub SceAppMgr_stub SceAppUtil_stub
  SceRtc_stub SceCommonDialog_stub SceTouch_stub SceHid_stub SceMotion_stub SceCamera_stub -Wl,--end-group)
vita_create_self(eboot.bin HaloCE.elf UNSAFE)
set(ASSETS "${CMAKE_CURRENT_BINARY_DIR}/sce_sys")
add_custom_command(OUTPUT ${ASSETS}/icon0.png ${ASSETS}/bg.png ${ASSETS}/startup.png
  COMMAND ${Python3_EXECUTABLE} ${ROOT}/tools/vita_assets.py ${ASSETS}
  DEPENDS ${ROOT}/tools/vita_assets.py)
add_custom_target(vita_assets DEPENDS ${ASSETS}/icon0.png ${ASSETS}/bg.png ${ASSETS}/startup.png)
vita_create_vpk(HaloCE.vpk HCEV00001 eboot.bin VERSION ${VITA_APP_VERSION} NAME "Halo CE Vita"
  FILE ${ASSETS}/icon0.png sce_sys/icon0.png
       ${ASSETS}/bg.png sce_sys/livearea/contents/bg.png
       ${ASSETS}/startup.png sce_sys/livearea/contents/startup.png
       ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml)
add_dependencies(HaloCE.vpk-vpk vita_assets)
add_custom_target(vita DEPENDS eboot.bin-self)
add_custom_target(vita_vpk DEPENDS HaloCE.vpk-vpk)
