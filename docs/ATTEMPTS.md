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
