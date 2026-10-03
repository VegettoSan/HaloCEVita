# Decisiones de la nueva base

## R-D001 — Reiniciar desde las dos fuentes originales (2026-10-03)

Instrucción explícita del usuario: descartar completamente el código anterior,
obtener el decomp del repositorio original y las adaptaciones Vita del port
donante; el backend gráfico final será vitaGL. Solo documentación anterior de
implementación de vitaGL útil y verificable puede consultarse.

Conservar el historial Git sin force-push. Las decisiones previas son históricas
y no restringen esta nueva base. Prohibido usar código anterior como recuperación
oculta. Las importaciones deben tener procedencia y licencias verificables.

## R-D002 — Detener el pipeline descartado antes de importar

Retirar el workflow anterior durante el reinicio: un push de documentación no
debe producir/publicar una VPK de la implementación descartada. El nuevo pipeline
compilará y verificará la nueva base y entregará solo artefactos del SHA final.
