#!/usr/bin/env python3
"""Actual typed sound relocation preserves opaque external sample metadata."""
import ctypes as C
import os
from pathlib import Path
import struct
import subprocess
from vita_menu_regression import Stats, BASE, NATIVE, NONE
root=Path(__file__).resolve().parents[1]
out=root/'build/vita/tests/sound-tags';out.mkdir(parents=True,exist_ok=True)
sdk=Path(os.environ.get('VITASDK','/usr/local/vitasdk-hardfp'))
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-O2',
 '-I'+str(root/'port/vita/include'),'-idirafter',str(sdk/'arm-vita-eabi/include'),
 str(root/'port/vita/src/vita_menu_relocate.c'),str(root/'port/vita/src/vita_cache_read.c'),
 '-l:libz.so.1','-o',str(out/'sound.so')],check=True)
lib=C.CDLL(str(out/'sound.so'))
lib.vita_cache_relocate_menu.argtypes=[C.c_void_p,C.c_size_t,C.c_uint32,C.POINTER(Stats),C.c_char_p,C.c_size_t]
lib.vita_cache_relocate_menu.restype=C.c_void_p
lib.vita_cache_restore_menu.argtypes=[C.c_void_p]
tags=bytearray(8192);cursor=36+4*32

def alloc(n):
 global cursor
 p=(cursor+3)&~3;cursor=p+n;return p

def u32(p,*v):struct.pack_into('<'+'I'*len(v),tags,p,*v)
def ref(p,index=NONE,group=NONE,name=0):u32(p,group,name,0,index)
roots=[];names=[];groups=[0x73636e72,0x44654c61,0x736e6421,0x6c736e64]
for i,(name,size) in enumerate(zip([b'levels\\ui\\ui',b'ui\\shell\\main_menu\\main_menu',b'synthetic_sound',b'synthetic_loop'],[1456,1004,164,84])):
 n=alloc(len(name)+1);tags[n:n+len(name)]=name;names.append(n)
 p=alloc(size);roots.append(p);u32(36+32*i,groups[i],NONE,NONE,0xe1740000+i,BASE+n,BASE+p,0,0)
u32(0,BASE+36,0xe1740000,0,4,0,0,0,0,0x74616773)
struct.pack_into('<h',tags,roots[0]+60,2)
for o in [56,236,252,340,356,420]:ref(roots[1]+o)
ref(roots[2]+112)
ran=alloc(72);perm=alloc(124);tracks=alloc(5*160);details=alloc(33*104)
u32(roots[2]+152,1,BASE+ran,0);u32(ran+60,1,BASE+perm,0)
struct.pack_into('<h',tags,ran+44,1)
# Sample address resembles an Xbox tag pointer, but belongs to opaque resource
# metadata. Four MiB sample size deliberately exceeds this metadata image.
u32(perm+44,NONE,0,0xe1740002,0x12345678,0xabcdef01)
u32(perm+64,0x400000,0x1,0x1234000,BASE+512,0)
u32(perm+84,0,0,0,BASE+516,0);u32(perm+104,0,0,0,BASE+520,0)
ref(roots[3]+44);u32(roots[3]+60,1,BASE+tracks,0);u32(roots[3]+72,1,BASE+details,0)
for t in range(5):
 for o in [48,64,80,128,144]:ref(tracks+t*160+o)
ref(tracks+64,0xe1740002,groups[2],BASE+names[2])
for t in range(33):ref(details+t*104)
ref(details,0xe1740002,groups[2])
tags=tags[:cursor]
def run(data,expect=True):
 b=C.create_string_buffer(bytes(data),len(data));err=C.create_string_buffer(256)
 plan=lib.vita_cache_relocate_menu(b,len(data),NATIVE,C.byref(Stats()),err,256)
 assert bool(plan)==expect,err.value
 after=b.raw
 if plan:lib.vita_cache_restore_menu(plan)
 assert b.raw==bytes(data),'transaction/rollback mutated source'
 return after
changed=run(tags)
for slot,dest in [(roots[2]+156,ran),(ran+64,perm),(roots[3]+64,tracks),(roots[3]+76,details),(tracks+68,names[2])]:
 assert struct.unpack_from('<I',changed,slot)[0]==NATIVE+dest
assert changed[perm:perm+124]==tags[perm:perm+124], 'external samples/runtime words were rebased'
for slot,val in [(roots[2]+156,BASE+len(tags)),(ran+64,BASE+perm+1),(ran+60,NONE),
 (tracks+76,0x11110002),(tracks+68,BASE+len(tags)),(roots[3]+60,5),(roots[3]+72,33),
 (roots[2]+152,9),(ran+60,257)]:
 bad=bytearray(tags);struct.pack_into('<I',bad,slot,val);run(bad,False)
print('PASS actual sound/loop typed relocation: nested pointers/references, opaque4MiB sample metadata,9 corruptions and exact rollback')
