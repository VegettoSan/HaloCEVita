#!/usr/bin/env python3
"""Actual native reader: one validated inflate, then seek-only resources."""
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
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fPIC','-shared','-O2','-I'+str(root/'port/vita/include'),str(root/'port/vita/src/vita_cache_read.c'),str(wrapper),'-Wl,--wrap=inflate','-Wl,--wrap=fwrite','-lz','-o',str(lib)],check=True)
r=C.CDLL(str(lib));r.vita_cache_resource_bind.argtypes=[C.c_char_p,C.c_uint32]
r.vita_cache_resource_read.argtypes=[C.c_uint32,C.c_void_p,C.c_size_t,C.c_char_p,C.c_size_t]
r.vita_cache_resource_error.restype=C.c_char_p
inflates=C.c_uint.in_dll(r,'test_inflates');writes=C.c_uint.in_dll(r,'test_writes');failure=C.c_int.in_dll(r,'test_write_failure')
spec=importlib.util.spec_from_file_location('fixture',root/'tools/vita_cache_regression.py');fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
_,_,small=fixture.fixture()
logical=bytearray(small+bytes((i*37+11)&255 for i in range(2*1024*1024)))
struct.pack_into('<I',logical,8,len(logical));logical=bytes(logical)
encoded=logical[:2048]+zlib.compress(logical[2048:])
err=C.create_string_buffer(160)
with tempfile.TemporaryDirectory() as tmp:
 path=Path(tmp)/'ui.map';cache=Path(tmp)/'cache0.vita-logical.tmp'
 for source in (encoded,logical):
  path.write_bytes(source);before=hashlib.sha256(source).hexdigest();writes.value=inflates.value=0
  assert r.vita_cache_resource_bind(str(path).encode(),len(logical))
  assert bool(cache.exists())==(source is encoded)
  if source is encoded:assert cache.read_bytes()==logical and inflates.value>1
  else:assert not inflates.value
  startup_inflates=inflates.value
  for i in range(128):
   offset=2048+(i*7919)%(len(logical)-2048-1234);buf=C.create_string_buffer(1234)
   assert r.vita_cache_resource_read(offset,buf,1234,err,len(err)),err.value
   assert buf.raw==logical[offset:offset+1234]
  assert inflates.value==startup_inflates
  assert hashlib.sha256(path.read_bytes()).hexdigest()==before
  assert not r.vita_cache_resource_read(len(logical)-10,C.create_string_buffer(11),11,err,len(err))
  r.vita_cache_resource_unbind();assert not cache.exists()
  r.vita_cache_resource_unbind()
  assert not r.vita_cache_resource_read(2048,C.create_string_buffer(1),1,err,len(err))
 # A damaged far-away checksum must fail even though a requested first byte is fine.
 bad=bytearray(encoded);bad[-1]^=1;path.write_bytes(bad)
 assert not r.vita_cache_resource_bind(str(path).encode(),len(logical));assert not cache.exists()
 path.write_bytes(encoded)
 for fail_at in (0,1,8):
  failure.value=fail_at;writes.value=0
  assert not r.vita_cache_resource_bind(str(path).encode(),len(logical));assert not cache.exists()
  assert b'write failed' in r.vita_cache_resource_error()
  assert path.read_bytes()==encoded
 failure.value=-1
 assert r.vita_cache_resource_bind(str(path).encode(),len(logical));r.vita_cache_resource_unbind()
 assert not r.vita_cache_resource_bind(str(path).encode(),len(logical)+1)
 assert b'logical size changed' in r.vita_cache_resource_error()
print('PASS actual seekable resources: full-stream/checksum validation,256 exact reads/no live inflation, unchanged source, owned cleanup,3 write failures and invalid ranges')
