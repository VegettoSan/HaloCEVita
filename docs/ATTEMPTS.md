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
