# Reinicio completo autorizado — 2026-10-03

## Instrucción del usuario

El usuario descarta toda la implementación previa de HaloCEVita tras probar
Build289 y volver a observar cuadros blancos, UI desplazada y problemas de texto.
Su instrucción es empezar desde el decomp original y las adaptaciones Vita del
otro port. Esta es la fuente de verdad para todas las sesiones siguientes.

## Alcance

- Sustituir la base de código actual por una importación limpia obtenida de
  cybersecurity/halo-ce-universal, no por nuestra copia modificada del decomp.
- Importar las adaptaciones de PS Vita directamente de BirchWoodGod/halo-ce-vita.
- Conservar los sistemas y el main loop del engine; adaptar su renderer a vitaGL.
- Descartar los glue, caché, punteros, bootstrap, UI, renderer, shaders, tests,
  recursos y build previos de HaloCEVita como fuente de implementación.
- Consultar exclusivamente documentación anterior de implementación de vitaGL
  cuando sea útil y comprobable; cualquier código nuevo debe provenir de las
  fuentes originales o ser una adaptación nueva claramente documentada.
- El historial conserva los intentos anteriores. Su conservación no convierte
  la implementación antigua en un fallback permitido.

## Fuentes

| Responsabilidad | Fuente autorizada |
| --- | --- |
| Comportamiento/estructuras/engine | https://github.com/cybersecurity/halo-ce-universal |
| Plataforma PS Vita | https://github.com/BirchWoodGod/halo-ce-vita |
| Backend gráfico final | vitaGL, con documentación/código oficial |

Registrar SHAs, archivos importados, cambios y licencias en docs/UPSTREAM.md y
un manifiesto de procedencia verificado. Importar solo código, no ejecutables
ni datos de juegos. El renderer GXM del donante puede estudiarse para comprender
sus contratos; el backend final debe usar vitaGL.

## Resultado esperado

ui.map -> bootstrap original adaptado -> gamestate -> Main Menu -> widgets
originales -> draw calls originales -> vitaGL -> menú visual y navegación.
Primero lograr el menú correctamente. Campaign, escenario y gameplay vienen
cuando las dependencias anteriores estén verificadas. No escribir widgets o
bitmaps manualmente para simular el resultado.

## Estado inicial del reinicio

La directiva está registrada; la importación limpia y el build nuevo son los
próximos pasos. Build289 y anteriores están descartadas como base y resultado.
Sus milestones no son evidencia de funcionamiento de la nueva implementación.
El workflow antiguo se retira antes de sustituir el código para impedir que
publique otra VPK descartada al guardar documentación.

## Disciplina

Trabajar directamente en main con commits pequeños y push frecuente. No
reescribir historial. Comprobar desde las fuentes originales cada frontera ABI,
init/lifecycle, memoria, lectura/relocación y llamada gráfica que se adapte.
Distinguir hechos, hipótesis y propuestas. Documentar cada bloqueo; no afirmar
que el menú funciona si no hay prueba de hardware. Una VPK nueva debe pertenecer
exactamente al SHA final de main y tener símbolos correspondientes.
