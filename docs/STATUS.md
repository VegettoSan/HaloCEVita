# Estado del reinicio

Actualizado: 2026-10-03.

**RESET AUTHORIZED / IMPORT PENDING.** La nueva directiva de docs/RESTART.md
sustituye la implementación y los baselines anteriores.

HECHO COMPROBADO: el usuario informa que la prueba de 00.43/Build289 vuelve a
mostrar cuadros blancos, UI desplazada y letras incorrectas/invisibles. Ese
resultado no establece una causa única. La implementación completa queda
fuera de la nueva base por instrucción explícita del usuario.

La nueva base debe proceder directamente del decomp original y las adaptaciones
Vita del donante. El renderer final debe ser vitaGL. La primera tarea es importar
esas fuentes con SHAs y licencias verificables; después integrar el build nativo
sin componentes descartados. Aún no hay evidencia BOOTS/RENDERS de la nueva base.

El workflow antiguo se retira. No publicar Build289 ni anteriores como resultado
del reinicio. Conservarlas únicamente como archivo en el historial y releases.
