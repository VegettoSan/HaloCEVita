# Procedencia de la nueva base

Importación directa del 2026-10-03:

- Decomp: https://github.com/cybersecurity/halo-ce-universal
  SHA `933aac61754eb5de2c8496dbe9b8278f033e4c1f`.
- Donante Vita revisado: https://github.com/BirchWoodGod/halo-ce-vita
  SHA `309b9deeb8f4e5b5155ca1e187c81e4ae207d1f2`; sus primeras adaptaciones ya están importadas
  con atribución y licencia en LICENSES/BirchWoodGod-GPL-3.0.txt.
- vitaGL revisado directamente: https://github.com/Rinnegatamante/vitaGL
  SHA `1ae86f65718675b797d3cd8ffc021ad2527096cb`.

`tools/origins.json` registra los 1490 archivos importados del decomp con ruta,
commit, blob Git, SHA-256 y modo. Los 967 archivos de source/ están inicialmente
intactos. La copia se obtuvo del checkout original, no de HaloCEVita.

Exclusiones explícitas: assets gráficos/fonts/menús del port, PGO, workflow
multiplataforma y ports Windows/Android excepto cabeceras ARM de Android. No
se incluyen datos retail ni paquetes ajenos. Las exclusiones completas constan
en el manifiesto. Se conserva LICENSE.md y licencias de terceros importados.

La ausencia de assets de menús/HUD mantiene esos overrides fuera del paquete;
el nuevo host deberá seleccionar explícitamente los tags Xbox originales.
El decomp reciente incorpora menús PC y cambia el generador de build respecto
al donante: hay que adaptar el generador Vita a su port.json actual.

El código del renderer, helpers y tests antiguos se ha retirado. El nuevo
verificador compara importaciones con el checkout original y rechaza código
sin procedencia registrada, además de los módulos descartados conocidos.

## Primera importación Vita (R003)

Copias directas del donante: host input/net/cpu/movie, vita_host.h, el walker
tag_relocate.c y sus layouts/cabecera, y su prueba de sockets con SceNet simulado.
Estos módulos aún no constituyen un ejecutable integrado. El manifiesto compara
sus hashes con el checkout del donante y conserva los del engine original.

Código nuevo: frontera vita_gl_host que usa exclusivamente las APIs oficiales
vglInitExtended, vglGetProcAddress y vglSwapBuffers; asserts independientes de
ABI para Clang/engine y GCC/SDK. No dibuja widgets ni altera los tags.

## Adaptaciones R004/R005

Memoria/physical address/log/timers y prefix de ARM vienen directamente del
donante. De su cache se trasladan solamente los hooks HALO_RELOCATABLE_TAG_CACHE
y las asignaciones sin dirección fija al decomp actual; se preserva su main
loop y capacidad actuales. La terminal usa va_list como en el donante.
Se aplicó su guard _WINT_T y se quitaron los macros limits de newlib que
colisionaban con enum originales. tools/vita_build.py deriva la frontera ABI
del generador del donante y usa la estructura nueva de port.json.

Los módulos main/settings/overlay/compat/pad/bink/memory-watch del donante se
importan como fuente; main/settings aún no entran en esta etapa de compilación.
Su cabecera vita_gxm.h documenta la dependencia pendiente del panel, pero no
se compila el renderer d3d8_gxm ni se incluye un ejecutable del donante.

Los archivos nuevos de auditoría y prueba real de mapas se registran como
código propio nuevo. SDK y headers oficiales se obtuvieron de nuevo; URLs,
SHAs y digest figuran en el manifiesto. Solo tres archivos de source/ difieren
del original: cache_files.c, physical_memory_map.c y terminal.c.
