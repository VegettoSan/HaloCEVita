# HaloCEVita

Port nativo ARM32 de Halo: Combat Evolved para PlayStation Vita con VitaSDK y
vitaGL. **Reinicio completo autorizado el 2026-10-03.**

La implementación anterior está descartada. La nueva base se obtiene
directamente de [halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal)
y utiliza adaptaciones de plataforma obtenidas de
[halo-ce-vita](https://github.com/BirchWoodGod/halo-ce-vita). La salida gráfica
final debe ser vitaGL y el menú debe proceder del engine y sus tags originales.

Leer [la directiva de reinicio](docs/RESTART.md), [AGENTS.md](AGENTS.md),
[estado](docs/STATUS.md) y [procedencia](docs/UPSTREAM.md).

Las VPK 00.43/Build289 y anteriores pertenecen a la implementación descartada.
Las pruebas de esas versiones no validan la nueva base. El reinicio aún no tiene
una VPK nueva aceptada. Los intentos anteriores permanecen en el historial Git.

No se incluyen mapas ni assets retail. ui.map y otros datos son suministrados
por el usuario y deben permanecer fuera del repositorio y del paquete.
