"""Record build outputs and their saved source hashes without rebuilding."""
import argparse, datetime, struct, subprocess
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/'firmware'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out',type=Path,default=ROOT/'private/build-record.json')
args=parser.parse_args()
app=(FW/'build/nabo-fortune-passport.bin').read_bytes()
assert struct.unpack_from('<I',app,32)[0]==0xabcd5432
version=app[48:80].split(b'\0')[0].decode()
elf_sha=sha(FW/'build/nabo-fortune-passport.elf');assert app[0xb0:0xd0].hex()==elf_sha
names={
    'bootloader':('build/bootloader/bootloader.bin',0),
    'partition_table':('build/partition_table/partition-table.bin',0x8000),
    'app':('build/nabo-fortune-passport.bin',0x10000),
    'assets':('assets/fortune_assets.bin',0x200000),
    'merged':('build/nabo-fortune-merged.bin',0),
}
images={k:{'path':'firmware/'+rel,'offset':hex(off),'bytes':(FW/rel).stat().st_size,'sha256':sha(FW/rel)}
        for k,(rel,off) in names.items()}
files=[]
for folder in ('main','components/bsp'):
    files += [p for p in (FW/folder).rglob('*') if p.is_file() and p.suffix in ('.c','.h','.txt','.yml')]
files += [FW/'CMakeLists.txt',FW/'sdkconfig.defaults',FW/'partitions.csv',FW/'assets/assets_manifest.json',FW/'build/config/sdkconfig.h']
sources={p.relative_to(ROOT).as_posix():sha(p) for p in sorted(set(files))}
snapshot=hashlib.sha256(json.dumps(sources,sort_keys=True).encode()).hexdigest()
result={'schema_version':1,'date':datetime.date.today().isoformat(),'version':version,'app_elf_sha256':elf_sha,'target':'esp32c3',
        'idf_version':'5.5.3','idf_commit':'2c211b236707889e8400c4dc5644dd5c4ee071e0',
        'compiler':'riscv32-esp-elf GCC 14.2.0_20251107',
        'images':images,'saved_source_snapshot_sha256':snapshot,'saved_sources':sources,
        'app_partition_bytes':0x1f0000,'assets_partition_bytes':0x600000,
        'note':'Hashes bind saved build inputs and outputs. Physical validation status is in M4-device.md; this record is not publication approval.'}
args.out.parent.mkdir(parents=True,exist_ok=True)
args.out.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'images':images,'saved_source_snapshot_sha256':snapshot},indent=2))
