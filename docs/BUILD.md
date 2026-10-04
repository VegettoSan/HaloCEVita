# Compilar la etapa inicial del reinicio

## Dependencias fijadas

VitaSDK snapshot sdk-snapshot-20260926.790.1 (GCC 15.2.0 hard-float), con URL
y SHA-256 en tools/origins.json. Clang 18.1.3 y Ninja 1.11.1 se utilizaron
localmente. vitaGL 1ae86f65718675b797d3cd8ffc021ad2527096cb y vitaShaRK
df24065e65098b2d1ac533760109ad4367573f28 proceden de checkouts oficiales.
No se reutiliza ninguna biblioteca/ELF/VPK del port descartado.

```sh
python3 tools/verify_origins.py --reference /ruta/halo-ce-universal --vita-reference /ruta/halo-ce-vita
python3 -m tools.vita_build --sdk /ruta/vitasdk --cc clang-18 --vitagl /ruta/vitaGL --vitashark /ruta/vitaShaRK
ninja -f build/vita/build.ninja vita-objects
python3 tools/vitagl_contract.py --vitagl /ruta/vitaGL --output build/vita/renderer-contract.json
VITASDK=/ruta/vitasdk sh port/vita/tests/run_vita_net_test.sh
```

El generador es independiente del configure desktop y consume port.json del
decomp actual. No ejecuta la generación de assets de HUD/fonts/menús PC.
Hay targets separados vita-engine-objects, vita-platform-objects,
vita-host-objects y vita-abi-objects. No hay regla de enlace o VPK todavía.

## Prueba sobre mapas propios

Los datos retail no se guardan en el repo ni se suben a CI. Para un mapa Xbox
comprimido versión 5, conservar los primeros 2048 bytes y descomprimir con zlib
el stream que comienza después de esa cabecera, en una copia temporal.
La longitud final debe coincidir con el campo de longitud descomprimida de
la cabecera. El test abre esa copia solo para lectura.

```sh
gcc -m32 -std=gnu11 -O1 -g -iquote port/linux/include \
  port/vita/tests/tag_relocation_map.c port/linux/src/tag_relocate.c \
  -o build/vita/tag_relocation_map
build/vita/tag_relocation_map /ruta/copia-ui.descomprimida.map
build/vita/tag_relocation_map /ruta/copia-bloodgulch.descomprimida.map
```

Requiere libc/GCC multilib. Si el host no ejecuta i386, usar qemu-i386 y
VITA_NET_TEST_RUNNER=/ruta/qemu-i386 para la prueba de red. El test comprueba
identidades/nombres, campos de widgets anteriores a los punteros, bytes UTF-16,
que las mutaciones son desplazamientos de direcciones y que recargar un BSP
produce los mismos bytes reubicados. No prueba semánticas gráficas ni gameplay.
