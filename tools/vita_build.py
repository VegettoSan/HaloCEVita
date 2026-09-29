"""First-class Ninja entry points; native compiler/build lives in port/vita."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

def vita_configure_inputs():
    return [Path(__file__), Path('port/vita/CMakeLists.txt'), Path('tools/vita_semantics.py')]

def generate_vita_build(n, sln):
    if not Path('port/vita/CMakeLists.txt').is_file():
        return
    n.comment('Native ARM32 Vita core bring-up (not the complete Halo main yet)')
    n.rule('vita_native', command='$python tools/vita_build.py --target $vita_target $vita_options',
           description='VITA NATIVE $vita_target', pool='console')
    n.build('vita_always', 'phony')
    options = '--release' if getattr(sln, 'port_release', False) else ''
    n.build('build/vita/eboot.bin', 'vita_native', inputs=['vita_always'],
            variables={'vita_target': 'vita', 'vita_options': options})
    n.build('build/vita/HaloCE.vpk', 'vita_native', inputs=['build/vita/eboot.bin', 'vita_always'],
            variables={'vita_target': 'vita_vpk', 'vita_options': options})
    n.build('vita', 'phony', inputs=['build/vita/eboot.bin'])
    n.build('vita_vpk', 'phony', inputs=['build/vita/HaloCE.vpk'])
    n.newline()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', choices=['vita', 'vita_vpk'], default='vita_vpk')
    parser.add_argument('--release', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    os.chdir(root)
    sdk = Path(os.environ.get('VITASDK', '/usr/local/vitasdk-hardfp'))
    compiler = sdk / 'bin/arm-vita-eabi-gcc'
    if not compiler.is_file():
        sys.exit('Set VITASDK to an existing native/hard-float VitaSDK.')
    # Directory names alone are insufficient: reject the SoftFP toolchain.
    macros = subprocess.run([str(compiler), '-dM', '-E', '-x', 'c', '-'], input='',
                            text=True, capture_output=True, check=True).stdout
    if '__ARM_PCS_VFP' not in macros or '#define __SIZEOF_POINTER__ 4' not in macros:
        sys.exit('Selected compiler is not ARM32 hard-float. Select the standard SDK; no SDK was changed.')
    env = dict(os.environ, VITASDK=str(sdk), PATH=str(sdk / 'bin') + os.pathsep + os.environ.get('PATH', ''))
    ninja = shutil.which('ninja')
    if not ninja:
        candidate = root / 'build/vita/tools/ninja/usr/bin/ninja'
        if candidate.is_file(): ninja = str(candidate)
    if not ninja: sys.exit('Ninja is required. Install/provide ninja or use the documented local extraction.')
    subprocess.run(['cmake', '-S', 'port/vita', '-B', 'build/vita', '-G', 'Ninja',
                    '-DCMAKE_MAKE_PROGRAM=' + ninja,
                    '-DCMAKE_BUILD_TYPE=' + ('RelWithDebInfo' if args.release else 'Debug')], env=env, check=True)
    subprocess.run(['cmake', '--build', 'build/vita', '--target', args.target, '-j', '6'], env=env, check=True)
    if args.target == 'vita_vpk':
        subprocess.run([sys.executable, 'tools/vita_verify.py'], env=env, check=True)

if __name__ == '__main__':
    main()
