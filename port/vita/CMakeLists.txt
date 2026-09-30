cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "Set VITASDK to the existing standard/native SDK (not SoftFP)")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE FILEPATH "Vita toolchain")
project(HaloCEVita C)
set(VITA_APP_VERSION "00.06")
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep libc/Vita APIs separate from the game's 16-bit wchar and MSVC macros.
add_library(halo_core STATIC
  src/halo_core.c src/halo_services.c src/xinput_vita.c src/xbox_memory_vita.c
  ${ROOT}/source/cseries/cseries.c
  ${ROOT}/source/cseries/debug_memory.c
  ${ROOT}/source/cseries/profile.c
  ${ROOT}/source/math/random_math.c
  ${ROOT}/source/memory/data.c
  ${ROOT}/source/memory/memory_pool.c
  ${ROOT}/source/memory/crc.c
  ${ROOT}/source/cache/cache_files.c
  ${ROOT}/source/cache/physical_memory_map.c
  "${ROOT}/source/saved games/game_state_xbox.c"
  ${ROOT}/port/linux/src/nv2a_vsh.c
  ${ROOT}/port/linux/src/nv2a_psh.c
  ${ROOT}/port/linux/src/xgpu_text.c)
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
  ${ROOT}/port/linux/src/nv2a_vsh.c ${ROOT}/port/linux/src/nv2a_psh.c ${ROOT}/port/linux/src/xgpu_text.c
  PROPERTIES COMPILE_DEFINITIONS HALO_LINUX_PLATFORM_LAYER)

add_executable(HaloCE.elf src/main.c src/vita_platform.c src/vita_graphics.c src/vita_gl_compat.c
  src/vita_memory.c src/vita_cache_read.c src/vita_cache_probe.c)
target_compile_definitions(HaloCE.elf PRIVATE HALO_VITA_APP_VERSION="${VITA_APP_VERSION}")
target_include_directories(HaloCE.elf PRIVATE include)
target_compile_options(HaloCE.elf PRIVATE -std=gnu11 -Wall -Wextra -O2 -g -ffunction-sections -fdata-sections -ffp-contract=off)
target_link_options(HaloCE.elf PRIVATE -Wl,-q -Wl,--gc-sections "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/HaloCE.elf.map")
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
