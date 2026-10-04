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
