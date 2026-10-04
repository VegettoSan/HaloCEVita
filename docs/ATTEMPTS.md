# Intentos de la nueva base

Los intentos anteriores pertenecen a la implementación descartada y permanecen
en el historial Git hasta 29b967fae5ac9546f7a79cd2bd86adcc1b7d2ce8.
No constituyen contratos ni baselines para esta implementación.

## R001 — Guardar la directiva e iniciar el reinicio (2026-10-03)

- HECHO COMPROBADO: reporte del usuario sobre Build289: persisten cuadros
  blancos, posiciones incorrectas y texto ausente/incorrecto. Sin log nuevo,
  no atribuimos todos los síntomas a una única causa.
- El usuario autoriza descartar todo el código y comenzar desde las fuentes
  originales de cybersecurity/halo-ce-universal y BirchWoodGod/halo-ce-vita.
- AGENTS.md/RESTART.md y documentos de estado sustituyen reglas anteriores.
  Se retira el workflow Vita descartado para evitar publicaciones engañosas.
- Comandos iniciales: git status; git fetch de main y de ambas fuentes;
  inspección de los repositorios originales, README del donante y build.
- Resultado: instrucciones persistentes; importación y build nuevos pendientes.

## R002 — Importación directa limpia del decomp (2026-10-03)

- Se obtuvo main933aac61754eb5de2c8496dbe9b8278f033e4c1f del repositorio original.
- Se retiraron todos los archivos rastreados anteriores, conservando únicamente
  la directiva y documentación nueva R001. Copia directa de1490 archivos,
  incluidos los967 archivos del engine, sin modificar bytes ni layouts.
- Se excluyen gráficos/menús/fonts del port, perfiles PGO y workflows/ports
  ajenos al build Vita (cabeceras ARM de Android sí se conservan). La lista
  explícita y hashes/rutas/blobs/modos quedan en tools/origins.json.
- Nuevo verificador registra código de fuentes autorizadas y código de proyecto
  nuevo; rechaza módulos descartados y código fuera del manifiesto.
- Comprobación: python3 tools/verify_origins.py --reference ../reference-upstream.
  No se conserva el stack de tests antiguo como evidencia de esta base.
- El nuevo workflow solo comprueba procedencia; aún no publica VPK.
  Adaptaciones Vita, ABI, build y traducción vitaGL siguen en trabajo.
- La importación conserva whitespace original (git diff --check lo reporta);
  no se reformatea el decomp para mantener la comparación de bytes. Se excluye
  el ejecutable auxiliar libtiff/mkg3states: no es código fuente ni dependencia
  del engine. Quedan1490 importaciones,967 archivos source/, todos verificables.

## R003 — Importar módulos Vita independientes (2026-10-03)

- Procedencia verificada: 1502 importaciones y 5 archivos nuevos registrados.
- Incluye el walker de reubicación por layouts del donante; su invocación por
  cache_files y el arranque del host aún requieren integración.
- Se importa la prueba de red del donante. Su primera ejecución local no pudo
  compilar porque no había cabeceras VitaSDK instaladas; no se registra PASS.
- Se prepara una toolchain nueva; no se reutilizan binarios del port descartado.

## R004 — Compilación ARM de la base limpia y hooks de caché (2026-10-04 UTC)

- Adaptado generador del donante al port.json actual: 466 unidades engine,
  6 plataforma, 6 host y 2 sondas ABI. Los 480 objetos compilan.
- Toolchain obtenida directamente del snapshot oficial y digest comprobado.
- Fallos corregidos: macros limits de newlib contra enum engine; guard wint_t;
  terminal va_list x86; cambio de firma preferred_port en UPnP; dependencia
  oficial vitaShaRK de vitaGL.h. No se cambiaron geometría ni datos de widgets.
- Se conserva -fmax-type-align=1 como el donante. Interlocked emite advertencias
  de alineación que requieren validación de consumidores reales (R-KI003).
- No se afirma LINKS/BOOTS/RENDERS: el host completo y renderer no están ligados.

## R005 — Pruebas de datos reales y contratos (2026-10-04 UTC)

- Walker del donante ejecutado como i386 bajo QEMU 8.2.2 sobre copias
  descomprimidas de los dos mapas adjuntos. Resultados y límites en STATUS.
- ui.map: 983 tags, 485 widgets, 195 bitmaps, 862 textos; 6448 relocaciones.
- bloodgulch.map: 1806 tags, 30 widgets, 368 bitmaps, 181 textos; 17319 relocaciones.
- Ambos BSP cargan/recargan con resultados idénticos; solo cambian words que
  cumplen el desplazamiento de una dirección en la ventana Xbox.
- La prueba SceNet del donante pasó 51/51 comprobaciones de errores, TCP/UDP,
  lobby, actualizaciones y sockets conectados bajo su mock. No prueba Wi-Fi real.
- Auditoría GL oficial: 89/104 exports; 15 ausentes. Reporta BLOCKED y no
  genera stubs. Tampoco valida las semánticas de los 89 presentes.
- Nuevo workflow prepara solo estas comprobaciones; no publica un VPK.
- Dos intentos de push fueron rechazados por revisión automática: publicar
  código en main del repo público sin autorización considerada confiable.
  Se verificó el destino exacto y permisos, pero persistió el rechazo. No
  se elude el bloqueo. Los commits nuevos permanecen locales.
