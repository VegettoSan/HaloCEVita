# Estado del reinicio

Actualizado: 2026-10-04 UTC.

**COMPILES: engine ARM32 + módulos seleccionados. LINKS / BOOTS / RENDERS pendientes.**

Base obtenida directamente del decomp original `933aac61754eb5de2c8496dbe9b8278f033e4c1f`
y del donante Vita `309b9deeb8f4e5b5155ca1e187c81e4ae207d1f2`.
El engine conserva 964 de sus 967 archivos idénticos al original; solo se
adaptaron cache_files.c, physical_memory_map.c y la declaración va_list de la
terminal con los contratos del donante. No se usa código del port descartado.

La etapa local compiló 480 objetos: 466 unidades C originales del engine,
6 módulos de plataforma, 6 módulos host y 2 sondas ABI. La prueba del donante
con SceNet simulado pasó 51/51 comprobaciones en un ejecutable i386 bajo QEMU.

Prueba de reubicación del walker real del donante, usando copias en memoria:

| Mapa | Tags/nombres | Widgets, primeros campos intactos | Bitmaps | Textos UTF-16 intactos | Palabras reubicadas | BSP carga/recarga |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ui.map | 983 | 485 | 195 | 862 | 6448 | 1 |
| bloodgulch.map | 1806 | 30 | 368 | 181 | 17319 | 1 |

Estas pruebas no ejecutan el main loop ni prueban la pantalla. Los mapas
originales se abrieron solo para lectura; no están en el repo ni en un VPK.

La frontera host vitaGL ya compila contra las cabeceras oficiales. La auditoría
del renderer original encuentra 89/104 entry points exportados y 15 ausentes
(ver docs/RENDERER.md). La presencia de exports tampoco prueba sus semánticas.
El renderer, sdl_platform, files/audio y la integración completa del host aún
requieren adaptación y enlace real; main/settings importados no participan de
esta etapa. No existe un VPK del reinicio.

El workflow nuevo añade compilación ARM y pruebas de red; aún no se ha ejecutado
para los commits locales porque la revisión automática bloqueó el push.
No publica VPK. Los commits anteriores hasta ef5529b sí estaban en main al
iniciar esta continuación. Los nuevos checkpoints están en Git local.
