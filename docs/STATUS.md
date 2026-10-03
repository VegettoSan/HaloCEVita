# Estado del reinicio

Actualizado: 2026-10-03.

**CLEAN DECOMP IMPORTED / VITA INTEGRATION IN PROGRESS.**

La directiva de docs/RESTART.md sustituye todos los baselines anteriores.
1490 archivos proceden directamente del decomp original933aac6; los967 archivos
del engine son copias sin cambios. El árbol anterior y su pipeline se retiraron.
Procedencia/hash/ausencia de código no registrado se comprueban mediante
`python3 tools/verify_origins.py --reference /ruta/al/checkout-original`.

La plataforma Vita se está integrando desde el donante309b9de. Se preservará
su frontera Clang(engine)/GCC(host); el backend final debe ser vitaGL.
El generador del donante utiliza la estructura antigua de port.json y necesita
adaptarse al decomp actual. Aún no hay build ni evidencia BOOTS/RENDERS del
reinicio. El workflow nuevo comprueba procedencia; todavía no publica VPK.

Build289 y anteriores siguen descartadas. El reporte de cuadros blancos y UI/
texto incorrectos es evidencia del trabajo archivado, no de esta base nueva.
