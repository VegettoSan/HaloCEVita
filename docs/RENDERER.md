# Contrato gráfico del reinicio

La fuente es port/linux/src/d3d8_gl.c/gl.h/nv2a_vsh.c/nv2a_psh.c/xbox_textures.c
del decomp original fijado. No se han recuperado shaders ni glue anteriores.
La comparación se reproduce con tools/vitagl_contract.py contra la lookup.c
del vitaGL oficial fijado. 89/104 funciones de la rama desktop se exportan.

| Funciones ausentes | Trabajo necesario antes del enlace/render |
| --- | --- |
| glBindFragDataLocation, glClipControl | Revisar salidas del shader y convención de clip/profundidad; conservar la transformación original. |
| glVertexAttribIPointer, glVertexAttribI4ui | Adaptar lectura/desempaquetado de atributos NV2A sin alterar los valores consumidos por el shader. |
| glTexImage3D, glCompressedTexImage3D | Resolver los tipos de textura reales; no convertirlos silenciosamente en 2D. |
| glGetTexImage, glCopyImageSubData | Implementar lecturas/copias de recursos con formatos, niveles y targets correctos. |
| glTexParameterfv, glSamplerParameterfv, glBlendColor | Conservar border color, sampler y constantes de blending según cada estado D3D usado. |
| glBufferStorage, glMemoryBarrier | Adaptar almacenamiento/sincronización a las operaciones reales que soporta vitaGL. |
| glDrawBuffers | Comprobar el uso de targets del engine y mapear su salida sin afirmar MRT inexistente. |
| glDebugMessageCallback | Sustituir diagnóstico opcional por errores/logs reales, sin simular un callback del driver. |

Esto es una lista de trabajo, no una afirmación de que todos los fallbacks
estén implementados. Además deben comprobarse los exports presentes: sus
semánticas completas pueden diferir de OpenGL 4.5. No activar
FAKE_UNRESOLVED_FUNCS del lookup oficial.

La frontera vita_gl_host crea el contexto oficial y expone resolución y swap
mediante scalars/punteros. Compila con GCC de VitaSDK, separado del engine
Clang. No representa todavía un frame generado por Halo. Las siguientes
fases deben conectar esa frontera al flujo original, compilar shaders NV2A
equivalentes, cerrar los contratos faltantes y validar con hardware.
