#!/usr/bin/env bash
set -u
printf 'Kernel: '; uname -a || true
printf 'VITASDK=%s\n' "${VITASDK:-<unset>}"
printf 'arm-vita-eabi-gcc: '; command -v arm-vita-eabi-gcc || true
arm-vita-eabi-gcc --version 2>/dev/null | head -n 1 || true
printf 'arm-vita-eabi-g++: '; command -v arm-vita-eabi-g++ || true
printf 'cmake: '; command -v cmake || true
cmake --version 2>/dev/null | head -n 1 || true
printf 'ninja: '; command -v ninja || true
ninja --version 2>/dev/null || true
printf 'python: '; command -v python3 || command -v python || true
