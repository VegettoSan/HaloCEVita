# HaloCEVita — instrucciones vigentes para agentes

## Directiva prioritaria del usuario: reinicio completo (2026-10-03)

Leer primero [docs/RESTART.md](docs/RESTART.md). Esta directiva sustituye todas
las decisiones, contratos, baselines y planes anteriores de HaloCEVita.

El usuario probó 00.43/Build289 y reportó nuevamente cuadros blancos,
UI desplazada y texto incorrecto/invisible. Ordenó descartar la implementación
completa y empezar desde las dos fuentes originales. No diagnosticamos una
causa única a partir de ese reporte.

## Fuentes permitidas para la nueva implementación

1. Engine y comportamiento: https://github.com/cybersecurity/halo-ce-universal
   Obtener el código directamente del repositorio original, fijando un SHA.
2. Adaptaciones de plataforma Vita:
   https://github.com/BirchWoodGod/halo-ce-vita
   Obtener código directamente de ese repositorio, fijando un SHA, conservando
   atribución/licencias y estudiando las dependencias con su renderer.
3. Backend gráfico: vitaGL. Estudiar código/documentación oficial del backend.
4. Del trabajo descartado solo puede consultarse documentación de implementación
   de vitaGL que resulte útil y verificable. No copiar su código, tests,
   bibliotecas generadas, shaders modificados, recursos de packaging ni glue.

El historial de Git conserva el trabajo descartado como archivo. No utilizar
una revisión de HaloCEVita como fuente de la nueva implementación, aunque un
archivo parezca funcionar. Si una adaptación del donante coincide con algo
anterior, importarla de la fuente donante y registrar su procedencia nueva.

## Arquitectura obligatoria

- ARM32 nativo, VitaSDK, vitaGL; conservar la ABI/layout del engine.
- Preservar el flujo original completo de inicialización y main loop.
- Usar las adaptaciones Vita del donante para bootstrap, memoria, files,
  cache/tags, threads, timers, audio, input, red, config y lifecycle donde sean
  necesarias. Conservar sistemas originales que ya realicen esas funciones.
- Mantener la separación ABI engine/host del donante cuando sea necesaria.
- Adaptar el renderer original a vitaGL con evidencia de cada contrato.
- UI original: ui.map -> inicialización/gamestate -> Main Menu -> widgets ->
  draw calls originales -> renderer adaptado -> vitaGL.
- Las posiciones, colores, alpha, fuentes y navegación proceden del engine
  y de los tags; no crear un menú propio ni modificar tags para ocultar fallos.
- No mezclar ejecutables, VPK, mapas o assets de builds anteriores o del donante.

## Trabajo y verificación

Antes de editar, leer README.md, docs/STATUS.md, docs/ATTEMPTS.md,
docs/KNOWN_ISSUES.md, docs/DECISIONS.md y las fuentes originales involucradas.
Registrar cada importación/adaptación en docs/UPSTREAM.md y en el manifiesto
nuevo de procedencia. Las primeras comprobaciones deben demostrar que la base
es de las fuentes permitidas y que ningún módulo descartado participa del build.

Distinguir HECHO COMPROBADO, HIPÓTESIS y PROPUESTA. Documentar fallos y límites;
no presentar compilación o link como prueba del menú en una Vita real. Usar
COMPILES, LINKS, BOOTS, RENDERS, PLAYABLE y STABLE con evidencia apropiada.
No resolver símbolos con éxito ficticio ni omitir sistemas originales para
producir una VPK aparentemente válida. No repetir pruebas sintéticas de menú
como sustituto de la integración del engine.

## Autorización y seguridad del repositorio

El usuario autoriza commits pequeños y push directo a main, y reafirma el
reinicio completo en esta conversación. Guardar progreso frecuentemente.
Preservar historial y trabajo ajeno; no force-push ni eliminar el repositorio.
No incluir mapas/retail data, SDK, secretos, build artifacts o libshacccg.suprx.
Las fuentes ui.map/bloodgulch.map del usuario se leen en modo de solo lectura.

Cada VPK entregada debe ser compilada del commit final de main, tener manifiesto
con el SHA exacto y conservar su ELF/map. Nunca entregar un paquete descartado
bajo el nombre de la nueva base. Si el reinicio todavía no compila, comunicar
el bloqueo exacto sin publicar una build anterior como resultado nuevo.
