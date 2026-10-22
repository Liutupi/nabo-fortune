"""Record build outputs and their saved source hashes without rebuilding."""
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FW=ROOT/'firmware'

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

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
result={'schema_version':1,'date':'2026-10-22','version':'0.1.2','target':'esp32c3',
        'idf_version':'5.5.3','idf_commit':'2c211b236707889e8400c4dc5644dd5c4ee071e0',
        'compiler':'riscv32-esp-elf GCC 14.2.0_20251107',
        'images':images,'saved_source_snapshot_sha256':snapshot,'saved_sources':sources,
        'app_partition_bytes':0x1f0000,'assets_partition_bytes':0x600000,
        'note':'Hashes bind saved build inputs and outputs. Physical validation status is in M4-device.md; this record is not publication approval.'}
(ROOT/'validation/m4-build.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'images':images,'saved_source_snapshot_sha256':snapshot},indent=2))
