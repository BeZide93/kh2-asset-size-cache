#!/usr/bin/env python3
import pathlib,subprocess,sys,struct,json,hashlib
import pefile
R=pathlib.Path(__file__).resolve().parent;B=R/'build';B.mkdir(exist_ok=True)
def run(*a):return subprocess.check_output([str(x) for x in a],text=True)
flags=['-target','x86_64-windows-gnu','-O2','-g0','-ffreestanding','-fno-builtin','-fno-jump-tables','-fno-stack-protector','-mno-stack-arg-probe','-funwind-tables']
for src,obj in [('fix.c','fix.o'),('hde/hde64.c','hde64.o')]:run(sys.executable,'-m','ziglang','cc',*flags,'-c',R/'src'/src,'-o',B/obj)
run(sys.executable,'-m','ziglang','cc','-target','x86_64-windows-gnu','-shared','-nostdlib','-Wl,--entry,DllMain','-Wl,--strip-debug',B/'fix.o',B/'hde64.o','-lkernel32','-o',R/'KH2AssetSizeCache.dll')
f=R/'KH2AssetSizeCache.dll'
data=bytearray(f.read_bytes());p=pefile.PE(data=bytes(data))
# Zig's PE linker inserts wall-clock timestamps in COFF/debug metadata.
# Normalize only these build timestamps before computing the PE checksum.
struct.pack_into('<I',data,p.FILE_HEADER.get_field_absolute_offset('TimeDateStamp'),0)
for debug in getattr(p,'DIRECTORY_ENTRY_DEBUG',[]):
    struct.pack_into('<I',data,debug.struct.get_field_absolute_offset('TimeDateStamp'),0)
if hasattr(p,'DIRECTORY_ENTRY_EXPORT'):
    struct.pack_into('<I',data,p.DIRECTORY_ENTRY_EXPORT.struct.get_field_absolute_offset('TimeDateStamp'),0)
struct.pack_into('<I',data,p.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'),0)
struct.pack_into('<I',data,p.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'),pefile.PE(data=bytes(data)).generate_checksum())
f.write_bytes(data);p=pefile.PE(data=bytes(data))
assert p.FILE_HEADER.Machine==0x8664 and p.OPTIONAL_HEADER.Magic==0x20b
assert p.FILE_HEADER.Characteristics&0x2000 and not p.get_warnings()
assert p.OPTIONAL_HEADER.CheckSum==p.generate_checksum()
assert [(e.name,e.ordinal) for e in p.DIRECTORY_ENTRY_EXPORT.symbols]==[(b'OnInit',1)]
assert [d.dll.lower() for d in p.DIRECTORY_ENTRY_IMPORT]==[b'kernel32.dll']
assert all(not (s.Characteristics&0x20000000 and s.Characteristics&0x80000000) for s in p.sections)
# Actual helper function/data addresses are recovered from linked COFF relocations.
# No hand-maintained final DLL RVAs in tests.
o=(B/'fix.o').read_bytes();_,ns,_,symptr,nsyms,opt,_=struct.unpack_from('<HHIIIHH',o)
sections=[]
for i in range(ns):
 a=struct.unpack_from('<8sIIIIIIHHI',o,20+opt+i*40);sections.append(a)
strbase=symptr+nsyms*18;symbols={};indices={};i=0
while i<nsyms:
 raw,val,sect,typ,sc,aux=struct.unpack_from('<8sIhHBB',o,symptr+18*i)
 if raw[:4]==b'\0'*4:
  off=struct.unpack_from('<I',raw,4)[0];end=o.index(0,strbase+off);name=o[strbase+off:end].decode()
 else:name=raw.rstrip(b'\0').decode()
 indices[i]=(name,val,sect);symbols[name]=(val,sect);i+=1+aux
entry=next(e.address for e in p.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'OnInit')
codebase=entry-symbols['OnInit'][0]
a=sections[symbols['OnInit'][1]-1];raw=o[a[4]:a[4]+a[3]];linked=p.get_data(codebase,len(raw));mask=bytearray(b'\1'*len(raw));addresses={}
for j in range(a[7]):
 off,idx,kind=struct.unpack_from('<IIH',o,a[5]+10*j);assert 4<=kind<=9
 name,val,sect=indices[idx];addend=struct.unpack_from('<i',raw,off)[0];disp=struct.unpack_from('<i',linked,off)[0]
 addr=codebase+off+4+kind-4+disp-addend
 if name in addresses:assert addresses[name]==addr,(name,addresses[name],addr)
 addresses[name]=addr;mask[off:off+4]=b'\0'*4
assert all(not mask[i] or raw[i]==linked[i] for i in range(len(raw)))
for name,(val,sect) in symbols.items():
 if sect==symbols['OnInit'][1] and not name.startswith('.'):addresses[name]=codebase+val
assert addresses['DllMain']==p.OPTIONAL_HEADER.AddressOfEntryPoint
h='#include <stddef.h>\n#define IMAGE_SIZE '+str(p.OPTIONAL_HEADER.SizeOfImage)+'\n'
h+='struct chunk {unsigned rva,offset,size,flags;};\nstatic const struct chunk chunks[]={'+','.join('{'+','.join(str(x) for x in [s.VirtualAddress,s.PointerToRawData,s.SizeOfRawData,s.Characteristics])+'}' for s in p.sections)+'};\n'
for d in p.DIRECTORY_ENTRY_IMPORT:
 for e in d.imports:h+=f'#define IAT_{e.name.decode()} {e.address-p.OPTIONAL_HEADER.ImageBase}\n'
for n in ['OnInit','DllMain','discover_panacea','cached_size','install_hook','previous_size','info_slot','game_begin','game_end','cache','generation','hits','refreshes','candidates','contention','no_package','invalid_size','evicted','fallback_active','diag_prepare','diag_profile_ok','diag_install','diag_flush','diag_active','diag_direct','diag_attrs_slot','diag_register_slot','diag_info_slot','diag_patch_count','diag_patches','diag_freq','diag_origin','diag_dropped','diag_untracked','diag_lock','diag_stream_lock','diag_rows','diag_page','diag_open_0','diag_read_0','diag_seek_0','diag_tell_0','diag_close_0','diag_attrs_0','diag_transform_0','diag_info_0','diag_register','diag_remaster','diag_size']:h+=f'#define RVA_{n} {addresses[n]}\n'
# Currently all compiled code/data is PC-relative or zero-initialized.
assert not p.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size
# The unused CodeView/PDB identifier otherwise depends on the build directory.
assert not p.OPTIONAL_HEADER.DATA_DIRECTORY[6].Size
(B/'generated.h').write_text(h)
man={'file':f.name,'sha256':hashlib.sha256(data).hexdigest(),'size':len(data),'exports':['OnInit'],'imports':['KERNEL32.dll'],'entry':hex(p.OPTIONAL_HEADER.AddressOfEntryPoint),'symbols':addresses,'runtime_functions':len(p.DIRECTORY_ENTRY_EXCEPTION),'game_tested':False}
(R/'BUILD-MANIFEST.json').write_text(json.dumps(man,indent=2)+'\n')
dump=run('objdump','-p',f);(R/'PE-EXPORTS.txt').write_text(dump)
print('PASS: AMD64 DLL, OnInit export, KERNEL32-only imports, valid checksum/unwind tables, RX/RW separation, no PE parser warnings.')
print(man['sha256'],len(data),'bytes')
