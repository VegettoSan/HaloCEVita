#!/usr/bin/env python3
"""Actual native cache: committed precache once, then seek-only resources."""
from pathlib import Path
import ctypes as C
import hashlib
import importlib.util
import struct
import subprocess
import tempfile
import zlib

root=Path(__file__).resolve().parents[1]
out=root/'build/vita/tests/resource-seek';out.mkdir(parents=True,exist_ok=True)
wrapper=out/'count.c'
wrapper.write_text('''#include <zlib.h>\n#include <stdio.h>\nunsigned test_inflates,test_writes;int test_write_failure=-1;\nint __real_inflate(z_streamp,int);\nint __wrap_inflate(z_streamp s,int f){test_inflates++;return __real_inflate(s,f);}\nsize_t __real_fwrite(const void*,size_t,size_t,FILE*);\nsize_t __wrap_fwrite(const void*p,size_t n,size_t c,FILE*f){if((int)test_writes++==test_write_failure)return 0;return __real_fwrite(p,n,c,f);}\n''')
lib=out/'seek.so'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fPIC','-shared','-O2',
 '-I'+str(root/'port/vita/include'),str(root/'port/vita/src/vita_cache_read.c'),str(wrapper),
 '-Wl,--wrap=inflate','-Wl,--wrap=fwrite','-lz','-o',str(lib)],check=True)
r=C.CDLL(str(lib))
progress_type=C.CFUNCTYPE(C.c_int,C.c_uint32,C.c_void_p)
r.vita_cache_prepare_slot.argtypes=[C.c_char_p,C.c_uint,C.c_char_p,C.c_size_t,C.POINTER(C.c_int),progress_type,C.c_void_p,C.c_char_p,C.c_size_t]
r.vita_cache_prepare_slot.restype=C.c_int
r.vita_cache_slot_valid.argtypes=[C.c_char_p,C.c_uint,C.c_char_p,C.c_size_t,C.c_char_p,C.c_size_t]
r.vita_cache_slot_valid.restype=C.c_int
r.vita_cache_resource_bind.argtypes=[C.c_char_p,C.c_uint32];r.vita_cache_resource_bind.restype=C.c_int
r.vita_cache_resource_read.argtypes=[C.c_uint32,C.c_void_p,C.c_size_t,C.c_char_p,C.c_size_t]
r.vita_cache_resource_read.restype=C.c_int
r.vita_cache_resource_error.restype=C.c_char_p
inflates=C.c_uint.in_dll(r,'test_inflates');writes=C.c_uint.in_dll(r,'test_writes');failure=C.c_int.in_dll(r,'test_write_failure')
spec=importlib.util.spec_from_file_location('fixture',root/'tools/vita_cache_regression.py');fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
_,_,small=fixture.fixture()
logical=bytearray(small+bytes((i*37+11)&255 for i in range(2*1024*1024)))
struct.pack_into('<I',logical,8,len(logical));logical=bytes(logical)
encoded=logical[:2048]+zlib.compress(logical[2048:])
err=C.create_string_buffer(160)
noop=progress_type(lambda _position,_context:1)

def prepare(path,slot=2):
 out_path=C.create_string_buffer(320);prep_err=C.create_string_buffer(160);reused=C.c_int(-1)
 ok=r.vita_cache_prepare_slot(str(path).encode(),slot,out_path,len(out_path),C.byref(reused),noop,None,prep_err,len(prep_err))
 return bool(ok),Path(out_path.value.decode()),reused.value,prep_err.value

with tempfile.TemporaryDirectory() as tmp:
 root_tmp=Path(tmp);maps=root_tmp/'maps';maps.mkdir();path=maps/'ui.map';cache=root_tmp/'cache002.map'

 # Compressed source: exactly one full verified precache, then no live inflation.
 path.write_bytes(encoded);before=hashlib.sha256(encoded).hexdigest();writes.value=inflates.value=0;failure.value=-1
 ok,built,reused,prep_error=prepare(path)
 assert ok,prep_error
 assert built==cache and cache.read_bytes()==logical and reused==0
 assert inflates.value>1 and writes.value>2
 successful_write_count=writes.value
 startup_inflates=inflates.value
 assert hashlib.sha256(path.read_bytes()).hexdigest()==before
 assert r.vita_cache_resource_bind(str(cache).encode(),len(logical)),r.vita_cache_resource_error()
 for i in range(128):
  offset=2048+(i*7919)%(len(logical)-2048-1234);buf=C.create_string_buffer(1234)
  assert r.vita_cache_resource_read(offset,buf,1234,err,len(err)),err.value
  assert buf.raw==logical[offset:offset+1234]
 assert inflates.value==startup_inflates
 assert not r.vita_cache_resource_read(len(logical)-10,C.create_string_buffer(11),11,err,len(err))
 r.vita_cache_resource_unbind();assert cache.exists()
 r.vita_cache_resource_unbind();assert cache.exists()
 assert not r.vita_cache_resource_read(2048,C.create_string_buffer(1),1,err,len(err))
 # The compressed source itself is no longer legal resource backing.
 assert not r.vita_cache_resource_bind(str(path).encode(),len(logical))
 assert b'precache' in r.vita_cache_resource_error()

 # Reuse is persistent: exact source header/logical size means no inflate/write.
 writes.value=inflates.value=0
 ok,built,reused,prep_error=prepare(path)
 assert ok and built==cache and reused==1,prep_error
 assert inflates.value==0 and writes.value==0

 # A damaged far-away zlib checksum must fail and never publish a valid slot.
 cache.unlink();bad=bytearray(encoded);bad[-1]^=1;path.write_bytes(bad);writes.value=inflates.value=0
 ok,_,_,prep_error=prepare(path)
 assert not ok and not cache.exists()
 assert b'zlib' in prep_error or b'length' in prep_error
 path.write_bytes(encoded)

 # Failure at blank header, payload, and final-header commit all leave no valid cache.
 final_write=successful_write_count-1
 for fail_at in (0,1,8,final_write):
  cache.unlink(missing_ok=True);failure.value=fail_at;writes.value=inflates.value=0
  ok,_,_,prep_error=prepare(path)
  assert not ok and not cache.exists(),(fail_at,prep_error)
  assert b'write' in prep_error or b'header commit' in prep_error
  assert path.read_bytes()==encoded
 failure.value=-1

 # A valid slot is tied to the exact source header/checksum identity.
 ok,_,_,prep_error=prepare(path);assert ok,prep_error
 changed=bytearray(encoded);changed[0x64]^=1;path.write_bytes(changed)
 slot_path=C.create_string_buffer(320);slot_err=C.create_string_buffer(160)
 assert not r.vita_cache_slot_valid(str(path).encode(),2,slot_path,len(slot_path),slot_err,len(slot_err))
 path.write_bytes(encoded)

 # Uncompressed input still uses the same persistent slot/commit protocol, but no zlib.
 cache.unlink();path.write_bytes(logical);writes.value=inflates.value=0
 ok,built,reused,prep_error=prepare(path);assert ok,prep_error
 assert built==cache and cache.read_bytes()==logical and not reused and inflates.value==0 and writes.value>2
 assert r.vita_cache_resource_bind(str(cache).encode(),len(logical));r.vita_cache_resource_unbind();assert cache.exists()
 assert not r.vita_cache_resource_bind(str(cache).encode(),len(logical)+1)
 assert b'logical size changed' in r.vita_cache_resource_error()

print('PASS upstream-style cache slots: exact logical bytes, header-last commit, persistent reuse, no live inflate, checksum/identity and write-failure rejection')
