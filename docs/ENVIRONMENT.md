# Development environment

Keep this file updated with facts that affect reproducibility.

## Current developer environment assumptions

- Host workflow: Linux/WSL is expected to be available.
- VitaSDK is already used for other Vita projects.
- More than one VitaSDK ABI/toolchain may exist on the machine.
- This project should use the normal native VitaSDK toolchain, not select SoftFP merely because another Android-loader project uses it.

## Agent rule

Before changing build files, print and record:

```bash
uname -a
echo "$VITASDK"
echo "$PATH"
which arm-vita-eabi-gcc || true
arm-vita-eabi-gcc --version || true
cmake --version || true
ninja --version || true
```

Do not install, remove, upgrade or replace the user's SDK unless explicitly requested. If a dependency is missing, document the exact dependency and the least-invasive installation command.
