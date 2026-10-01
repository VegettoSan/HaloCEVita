cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "Set VITASDK to the existing standard/native SDK (not SoftFP)")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE FILEPATH "Vita toolchain")
project(HaloCEVita C)
set(VITA_APP_VERSION "00.18")
option(HALO_VITA_MENU_BRINGUP "Start original Main Menu root after ui.map mount" ON)
option(HALO_VITA_MENU_UPDATE_PROBE "Link original process_ui_widgets closure (research only)" OFF)
option(HALO_VITA_MENU_RENDER_PROBE "Link original render_ui_widgets closure and rasterizer" ON)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(GLOB MUSL_MATH_SOURCES CONFIGURE_DEPENDS ${ROOT}/port/third_party/musl-math/src/*.c)

# Keep libc/Vita APIs separate from the game's 16-bit wchar and MSVC macros.
add_library(halo_core STATIC
  src/halo_core.c src/halo_menu_tags.c src/halo_services.c src/vita_cache_bridge.c src/xinput_vita.c src/xbox_memory_vita.c
  src/vita_memory_watch.c
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
  ${ROOT}/source/interface/ui_widget.c
  ${ROOT}/source/interface/ui_widget_event_handler_functions.c
  ${ROOT}/source/interface/player_ui.c
  ${ROOT}/source/networking/network_game_globals.c
  ${ROOT}/source/main/main.c
  ${ROOT}/source/game/game.c
  ${ROOT}/source/game/game_engine.c
  "${ROOT}/source/saved games/player_profile.c"
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
  ${ROOT}/port/linux/src/msvc_wide.c
  ${ROOT}/port/linux/src/nv2a_vsh.c ${ROOT}/port/linux/src/nv2a_psh.c ${ROOT}/port/linux/src/xgpu_text.c
  PROPERTIES COMPILE_DEFINITIONS HALO_LINUX_PLATFORM_LAYER)
set_source_files_properties(src/halo_menu_tags.c PROPERTIES COMPILE_OPTIONS
  "-Werror=implicit-function-declaration;-Werror=incompatible-pointer-types;-Werror=int-conversion;-Werror=return-type")

add_executable(HaloCE.elf src/main.c src/vita_platform.c src/vita_graphics.c src/vita_video_backend.c src/vita_gl_compat.c
  src/vita_renderer_platform.c src/vita_memory.c src/vita_cache_read.c src/vita_menu_relocate.c src/vita_cache_probe.c src/vita_xapi_files.c)
target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_APP_VERSION="${VITA_APP_VERSION}")
if(HALO_VITA_MENU_BRINGUP)
  target_compile_definitions(halo_core PRIVATE HALO_VITA_MENU_BRINGUP=1)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_BRINGUP=1)
endif()
if(HALO_VITA_MENU_UPDATE_PROBE)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_UPDATE_PROBE=1)
endif()
if(HALO_VITA_MENU_RENDER_PROBE)
  target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_MENU_RENDER_PROBE=1)
  target_link_options(HaloCE.elf PRIVATE -Wl,-u,_rasterizer_initialize)
endif()
target_include_directories(HaloCE.elf PRIVATE include)
target_compile_options(HaloCE.elf PRIVATE -std=gnu11 -Wall -Wextra -O2 -g -ffunction-sections -fdata-sections -ffp-contract=off)
# vita-elf-create appends SCE module/import metadata to the RX LOAD segment.
# Keep the constructor/destructor arrays one full 64-KiB page after the former
# 0x810e0000 boundary, then start normal RW data on the following page. This
# preserves the original array contents/order while leaving stable conversion
# headroom as the original renderer grows.
target_link_options(HaloCE.elf PRIVATE
  -Wl,-q -Wl,--gc-sections
  -Wl,--section-start=.init_array=0x810f0000
  -Wl,--section-start=.fini_array=0x810f001c
  -Wl,-Tdata=0x81100000
  "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/HaloCE.elf.map")
target_link_libraries(HaloCE.elf PRIVATE halo_core
  -Wl,--start-group vitaGL vitashark SceShaccCgExt SDL3 pthread stdc++ mathneon m z
  SceShaccCg_stub taihen_stub SceDisplay_stub SceCtrl_stub SceGxm_stub ScePower_stub
  SceKernelDmacMgr_stub SceIofilemgr_stub SceSysmem_stub SceProcessmgr_stub SceKernelThreadMgr_stub
  SceAudio_stub SceAudioIn_stub SceSysmodule_stub SceAppMgr_stub SceAppUtil_stub
  SceCommonDialog_stub SceTouch_stub SceHid_stub SceMotion_stub SceCamera_stub -Wl,--end-group)
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
