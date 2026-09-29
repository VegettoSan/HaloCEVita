# Entrega de la sesión — 2026-09-28

**Corrección posterior A012:** el VPK 00.01 descrito abajo fue rechazado por VitaShell con 0x8010113D según el usuario. Su SHA/tamaño son evidencia histórica. La entrega actual es **00.02**, con LiveArea PNG-8 indexado/opaque, SHA-256 `a3801dd4e0e39b14d732e6ec51a87f5ad37f09986136e0c57b9987b7a93320c8`; ELF/map/eboot no cambiaron. Ver BUILD.md y ATTEMPTS.md A012 para el detalle. La reinstalación y el boot aún requieren confirmación; estado LINKS.

Estado máximo demostrado: **LINKS**. Se generaron ELF ARM32 hard-float, SELF/eboot y VPK nativos. No se ejecutaron en Vita/Vita3K; BOOTS, RENDERS, PLAYABLE y STABLE no están demostrados. El menú y la campaña no están integrados.

## Resultado concreto

- Compilan 466/466 unidades C configuradas del juego original. La agregación relocatable conserva 584 imports de CRT/plataforma pendientes; no constituye el enlace del juego completo.
- El ejecutable sí enlaza y llama código real de Halo: cseries, heap de depuración, profiler, data arrays/datum/iteración, pool compactable, CRC, verificador de cabeceras Xbox y traductores NV2A.
- Capa Vita: logging temprano, filesystem/mapas, timer, prueba pthread, puente XInput, audio SDL3 pausado, diagnóstico vitaGL, shaders generados y prueba GPU framebuffer/blit.
- Ruta estable ux0:data/HaloCE/; log debug.txt. TITLE_ID HCEV00001, versión 00.01. Sin mapas, saves, assets retail ni libshacccg en el paquete.
- Se seleccionó VitaSDK hard-float existente; ningún SDK fue sobrescrito/actualizado. Sin loader Android, eliminación de subsistemas, commits ni reescritura de historia.

## Artefactos y evidencia

| Archivo | Resultado |
|---|---|
| build/vita/eboot.bin | Existe: 1.323.482 bytes; formato SELF verificado |
| build/vita/HaloCE.vpk | Existe: 1.318.349 bytes; CRC ZIP, SFO, eboot y seis entradas verificados |
| build/vita/HaloCE.elf / HaloCE.elf.map / HaloCE.elf.velf | Conservados para analizar crashes |
| build/vita/artifacts.json / SHA256SUMS | Tamaños, hashes, ABI y símbolos reales incluidos |
| build/vita/audit-abi/report.json | 466/466 COMPILES; comandos y errores previos conservados |
| build/vita/audit-abi/HaloGame.audit.o / game-imports.txt | Agregación ARM32 y 584 imports pendientes |
| build/vita/logs/ | Logs de builds/auditorías, ABI, imports y mapas |

SHA-256 del VPK entregado:

```text
6a323854be1c7eae452379769d31926eb5522610a1a51652f2d24d94a8595498
```

## Build reproducido

En Ubuntu WSL:

```bash
cd /home/vegettosandev/HaloCEVita
export VITASDK=/usr/local/vitasdk-hardfp
export PATH="$VITASDK/bin:$PWD/build/vita/tools/ninja/usr/bin:$PATH"
python3 configure.py --release
ninja vita
ninja vita_vpk
```

Ninja se extrajo localmente en build/vita/tools porque faltaba en este equipo. BUILD.md conserva el comando de preparación, alternativa CMake y auditorías. El build mantiene símbolos y assertions del juego. La verificación final del paquete y py_compile pasan; git diff --check no encuentra errores. La recompilación puede cambiar timestamps/hash del VPK.

## Errores y experimentos registrados

ATTEMPTS.md conserva A001–A011 y los fallos anteriores A000/A000.1. A002: conflictos newlib/MSVC y wchar; A003: inline GCC/XDK y wint_t; A004: agrupación incorrecta de -include por CMake, weak static, literales ui64, falso va_list y libtiff; A005: namespace Winsock/newlib y API vglEnd inexistente; A006: nombre incorrecto del stub thread manager e imports halo_linux_fopen/fprintf; A008/A009: prototipos static/extern, enums y error(short,const char*,...). Cada corrección está limitada a Vita y los logs iniciales permanecen.

El linker conserva advertencias por wchar/enum distintos entre juego y SDK; el puente enlazado usa argumentos estrechos/escalars y no prueba todos los ABI del juego. No se ocultaron imports del juego con bypass de enlace.

A011 inspeccionó metadatos de mapas existentes en F: sin copiar/modificar pixels/mapas. ui contiene cinco bitmaps 3D; c10/c20 seis, incluido ruido del escudo Elite. Esto prueba presencia en contenido de campaña, aún no su muestreo durante ejecución. a10 comprimido fue omitido por esta herramienta.

## Prueba y retorno de logs

1. Con HENkaku/VitaShell y homebrew inseguro habilitado, instalar HaloCE.vpk. Para la prueba GLSL, libshacccg.suprx debe estar instalado en ur0:data/libshacccg.suprx o su fallback ur0:data/external/libshacccg.suprx; no va en el VPK.
2. Lanzar primero sin mapas. Se espera una pantalla de diagnóstico HALO CE VITA / NATIVE HALO CORE PROBE y estado CORE/MAPS/GLSL; triángulo sintético si compilan los shaders. No es el menú del juego.
3. Copiar ui.map/a10.map Xbox v5 propios a ux0:data/HaloCE/maps/. Cross vuelve a comprobar cabeceras; sticks/botones registran el mapeo Xbox; Start sale. Los mapas PC/Custom Edition v7/609 no son equivalentes.
4. Revisar ux0:data/HaloCE/debug.txt. Último milestone esperado:012, tras011. 009 solo aparece si ambos mapas verifican;010 se omite deliberadamente. Buscar resultados de core/pthread/audio/copia GPU y compilación de shaders.
5. Dejar correr 30 segundos y relanzar varias veces; informar imagen visible, errores y salida con Start.
6. Ante crash devolver debug.txt completo, vertex_probe.glsl/fragment_probe.glsl, código de error, PC/LR/SP o dump con base/offset del módulo, captura, firmware/configuración y SHA del VPK instalado. No hacen falta mapas retail. Conservar el ELF/map coincidente; BUILD.md contiene addr2line.

## Bloqueos ordenados

1. Memoria Xbox: arena con direcciones colocadas o rebasing coherente de tags/recursos; malloc simple no preserva sus punteros y máscaras físicas.
2. Plataforma completa: 584 imports pendientes en la agregación, filesystem/XAPI/handles/APCs y servicios de memoria. El core enlazado tiene solo ocho weak hooks opcionales de SDK/CRT.
3. Renderer completo: integrar d3d8_gl, formatos/swizzling, GL faltante, streaming, NORMPACKED3, volúmenes 3D y shaders dinámicos aceptados por VitaShaRK. Los 108 APIs están inventariados con evidencia del header/archive instalado.
4. ABI reconstruido entre unidades/punteros de función y determinismo ARM: la compilación/layout no prueban todos los contratos.
5. Prueba real de boot/render, presupuestos de memoria medidos, audio real y estabilidad/30 FPS. Sin hardware o Vita3K disponible no hay evidencia de ejecución.

## Archivos creados

- `docs/ABI_VITA.md`
- `docs/SESSION_REPORT.md`
- `port/vita/CMakeLists.txt`
- `port/vita/include/halo_vita_graphics.h`
- `port/vita/include/halo_vita_prefix.h`
- `port/vita/include/vita_runtime.h`
- `port/vita/sce_sys/livearea/contents/template.xml`
- `port/vita/src/halo_core.c`
- `port/vita/src/halo_services.c`
- `port/vita/src/main.c`
- `port/vita/src/vita_gl_compat.c`
- `port/vita/src/vita_graphics.c`
- `port/vita/src/vita_platform.c`
- `port/vita/src/xinput_vita.c`
- `tools/vita_assets.py`
- `tools/vita_build.py`
- `tools/vita_compile_audit.py`
- `tools/vita_gl_audit.py`
- `tools/vita_link_audit.py`
- `tools/vita_map_audit.py`
- `tools/vita_semantics.py`
- `tools/vita_verify.py`

## Archivos modificados

- `.gitignore`
- `README.md`
- `docs/ATTEMPTS.md`
- `docs/BUILD.md`
- `docs/DECISIONS.md`
- `docs/GRAPHICS_COMPATIBILITY.md`
- `docs/KNOWN_ISSUES.md`
- `docs/STATUS.md`
- `docs/UPSTREAM.md`
- `port/linux/include/halo_linux_prefix.h`
- `port/linux/src/d3d8_gl.c`
- `port/linux/src/nv2a_psh.c`
- `port/linux/src/nv2a_vsh.c`
- `source/ai/ai_debug.c`
- `source/bitmaps/libtiff/tiffcompat.h`
- `source/cseries/debug_memory.c`
- `source/hs/hs.c`
- `source/interface/terminal.c`
- `source/interface/ui_widget_event_handler_functions.c`
- `source/memory/byte_swapping.c`
- `tools/project_x86.py`
