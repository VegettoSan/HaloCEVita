#!/usr/bin/env python3
"""Exercise actual D3D8 vblank worker and Present scheduling with real pthreads.

The old staged path blocks at frame three without the main-time callback.
This checks CPU scheduling and failure behavior, not Vita display/GPU output.
"""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "9a14de843863e3fbdd26f57359b93d9b52058523"
PREFIX = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
typedef int BOOL;
typedef void (*D3DCALLBACK)(unsigned long);
#define TRUE 1
#define FALSE 0
#define WINAPI
static int fail_create, create_count;
static atomic_int callback_count;
static int test_create(pthread_t *t, const pthread_attr_t *a,
    void *(*f)(void *), void *v) {
    create_count++;
    return fail_create ? 11 : pthread_create(t, a, f, v);
}
#define pthread_create test_create
static void platform_log(const char *f, ...) {(void)f;}
static void vita_log(const char *f, ...) {(void)f;}
static _Noreturn void vita_fatal(const char *f) {(void)f;exit(23);}
static void platform_pump_events(void) {}
static int interpolate;
static int halo_interpolation_enabled(void) {return interpolate;}
static int halo_vita_wait_monotonic_deadline(uint64_t ns) {
    struct timespec deadline={(time_t)(ns/1000000000ULL),(long)(ns%1000000000ULL)};
    return clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&deadline,NULL)==0;
}
'''
SUFFIX = r'''
static void callback(unsigned long value) {
    assert(value==0);
    atomic_fetch_add(&callback_count, 1);
}
int main(int argc, char **argv) {
    assert(argc==2);
    (void)vita_log;(void)vita_fatal;(void)d3d_find_flipcount;
    (void)halo_vita_wait_monotonic_deadline;
    if (!strcmp(argv[1], "failure")) {
        fail_create=1;
        present_schedule();
        assert(!"worker creation must fail before enqueuing");
    }
    if (!strcmp(argv[1], "callback")) {
        D3DDevice_SetVerticalBlankCallback(callback);
        for (int i=0;i<3;i++) D3DDevice_BlockUntilVerticalBlank();
        assert(atomic_load(&callback_count)>=2);
    }
    if (!strcmp(argv[1], "interpolation")) interpolate=1;
    for (int i=0;i<120;i++) {
        present_schedule();
        if (i==1) {puts("completed frame two");fflush(stdout);}
        pthread_mutex_lock(&vertical_blank_lock);
        assert(pending_flips<=2);
        pthread_mutex_unlock(&vertical_blank_lock);
    }
    for (int i=0;i<3;i++) D3DDevice_BlockUntilVerticalBlank();
    pthread_mutex_lock(&vertical_blank_lock);
    assert(pending_flips==0 && flip_count==120);
    assert(vertical_blank_thread_started && create_count==1);
    pthread_mutex_unlock(&vertical_blank_lock);
    puts("PASS actual scheduling:120 frames, bounded queue,120 flips,one worker");
}
'''

DEADLINE_PREFIX = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
static uint64_t now_ns;
static unsigned waits, last_delay;
static int fail_clock, fail_delay, early;
static int test_gettime(clockid_t clock, struct timespec *t) {
    assert(clock==CLOCK_MONOTONIC);
    if (fail_clock) return -1;
    t->tv_sec=(time_t)(now_ns/1000000000ULL);
    t->tv_nsec=(long)(now_ns%1000000000ULL);
    return 0;
}
#define clock_gettime test_gettime
static int sceKernelDelayThread(unsigned us) {
    assert(us>0 && us<=1000000);
    waits++;last_delay=us;
    if (fail_delay) return -1;
    if (early) {early=0;now_ns+=1000;} else now_ns+=(uint64_t)us*1000;
    return 0;
}
'''
DEADLINE_SUFFIX = r'''
int main(void) {
    now_ns=1234000000000ULL;
    assert(halo_vita_wait_monotonic_deadline(now_ns));
    assert(halo_vita_wait_monotonic_deadline(now_ns-1));
    assert(waits==0);
    uint64_t deadline=now_ns+16666666ULL;
    assert(halo_vita_wait_monotonic_deadline(deadline));
    assert(now_ns>=deadline && last_delay==16667 && waits==1);
    deadline=now_ns+9000000;early=1;unsigned before=waits;
    assert(halo_vita_wait_monotonic_deadline(deadline));
    assert(now_ns>=deadline && waits==before+2);
    deadline=now_ns+2500000000ULL;before=waits;
    assert(halo_vita_wait_monotonic_deadline(deadline));
    assert(now_ns==deadline && waits==before+3 && last_delay==500000);
    fail_clock=1;before=waits;
    assert(!halo_vita_wait_monotonic_deadline(now_ns+1) && waits==before);
    fail_clock=0;fail_delay=1;
    assert(!halo_vita_wait_monotonic_deadline(now_ns+1));
    puts("PASS actual native deadline:past,due,ceil,early wakeup,64-bit time,bounded delay,clock/kernel failures");
}
'''


def extract(source):
    start = source.index('/* ---------- vertical blank emulation */')
    end = source.index('/* ---------- GL helpers */', start)
    present = source.index('void WINAPI D3DDevice_Present(')
    tail = source.index('\tplatform_pump_events();', present)
    end_present = source.index('\nHRESULT WINAPI D3DDevice_PersistDisplay', tail)
    return source[start:end] + '\nstatic void present_schedule(void)\n{\n' + source[tail:end_present]


def main():
    source = (ROOT / 'port/linux/src/d3d8_gl.c').read_text()
    old = subprocess.check_output(['git', 'show', BASELINE + ':port/linux/src/d3d8_gl.c'], cwd=ROOT, text=True)
    out = ROOT / 'build/vita/tests/present'
    out.mkdir(parents=True, exist_ok=True)
    cc = os.environ.get('HOST_CC', 'cc')
    native = (ROOT / 'port/vita/src/vita_platform.c').read_text()
    begin = native.index('int halo_vita_wait_monotonic_deadline(')
    end = native.index('void vita_log(', begin)
    path = out / 'deadline.c'
    path.write_text(DEADLINE_PREFIX + native[begin:end] + DEADLINE_SUFFIX)
    exe = out / 'deadline'
    subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], timeout=2, check=True)
    # Vita guards leave other native backends' scheduling tokens unchanged.
    for defines in [[], ['-DHALO_ANDROID']]:
        tokens = [subprocess.check_output([cc, '-E', '-P', '-x', 'c', *defines, '-'], input=extract(s), text=True) for s in [old, source]]
        assert tokens[0] == tokens[1]
    print('PASS desktop/Android vblank scheduling unchanged')
    for label, code in [('historical', extract(old)), ('fixed', extract(source))]:
        path = out / (label + '.c')
        path.write_text(PREFIX + code + SUFFIX)
        exe = out / label
        subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-pthread', '-DHALO_VITA', str(path), '-o', str(exe)], check=True)
        if label == 'historical':
            try:
                subprocess.run([str(exe), 'plain'], capture_output=True, text=True, timeout=1, check=True)
                raise AssertionError('historical third frame should block')
            except subprocess.TimeoutExpired as error:
                assert b'completed frame two' in error.stdout
                print('PASS historical actual Present tail blocks at frame three')
        else:
            for mode in ['plain', 'callback', 'interpolation']:
                subprocess.run([str(exe), mode], timeout=5, check=True)
            result = subprocess.run([str(exe), 'failure'], timeout=2, check=False)
            assert result.returncode == 23
            print('PASS worker startup failure stops before queueing; callback and interpolation preserved')


if __name__ == '__main__':
    main()
