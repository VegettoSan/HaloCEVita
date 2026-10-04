# Bloqueos de la nueva base

## R-KI001 — Integración y enlace completos pendientes

El engine original compila como 466 objetos ARM32. Eso no valida un ejecutable.
Los seis módulos plataforma y seis host incluidos se detallan en tools/vita_build.py.
El main loop original está conservado; aún no está conectado a un host completo
y un renderer aceptado. vita_main/vita_settings están importados, pero su
dependencia GXM del panel no se ha sustituido todavía. No compilar/publicar
una VPK anterior como supuesto resultado de esta nueva base.

## R-KI002 — Contrato gráfico original todavía incompatible con vitaGL

15 de 104 nombres GL de la rama desktop original no se exportan en el vitaGL
fijado. Ver docs/RENDERER.md. No sustituirlos por funciones de éxito ficticio.
Faltan validación de semánticas, traducción de shaders, estados, formatos de
vértices/texturas y presentación. La prueba de datos de UI no prueba sus draws.

## R-KI003 — Advertencias ABI y servicios de host por cerrar

El donante usa __sync en los wrappers Interlocked; Clang advierte que esos
accesos deben estar naturalmente alineados. Se conserva el código del donante,
pero hay que verificar alineación y concurrencia de sus consumidores reales.
El guard de wint_t y los nombres de límites de newlib fueron adaptados al SDK
fijado. Las sondas ABI pasan. La escritura explícita de timestamps del donante
no está implementada: ahora devuelve ENOSYS en vez de afirmar éxito.
UPnP/Discord/registro de URL del donante devuelven indisponibilidad real.

## R-KI004 — Push bloqueado por revisión automática

El push fast-forward a VegettoSan/HaloCEVita main fue rechazado por considerar
que faltaba autorización confiable para publicar código en ese destino público,
a pesar de la instrucción previa registrada en AGENTS.md y la comprobación del
repo/destino/permisos. Se conservan commits locales y no se intenta eludir el
bloqueo mediante otro transporte. Se necesita autorización explícita nueva.
