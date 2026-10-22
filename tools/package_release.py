"""Package exact, validated build outputs; never read device backups."""
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--version',required=True);a=p.parse_args()
    b=json.loads((ROOT/'validation/m4-build.json').read_text(encoding='utf-8'))
    v=json.loads((ROOT/'validation/M4-device.json').read_text(encoding='utf-8'))
    boot=json.loads((ROOT/f'validation/RELEASE-v{a.version}.json').read_text(encoding='utf-8'))
    assert b['version']==v['version']==boot['version']==a.version
    assert v['acceptance']=='complete_local_device_validation'
    assert boot['all_four_build_images_digest_verified_on_device']
    subprocess.run([sys.executable,str(ROOT/'firmware/tools/verify_firmware.py'),str(ROOT/'firmware/build')],check=True)
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    assert not subprocess.check_output(['git','status','--porcelain'],cwd=ROOT).strip(),'Commit source changes before packaging'
    for name,expected in b['saved_sources'].items():assert sha(ROOT/name)==expected,name
    dest=ROOT/'dist'/('nabo-fortune-v'+a.version);dest.mkdir(parents=True,exist_ok=False)
    files={'bootloader':'bootloader.bin','partition_table':'partition-table.bin','app':'nabo-fortune-passport.bin','assets':'fortune_assets.bin'}
    manifest={'format':'nabo-fortune-release-v1','version':a.version,'chip':'esp32c3','flash_bytes':8388608,'source_commit':commit,'saved_source_snapshot_sha256':b['saved_source_snapshot_sha256'],'images':{}}
    for key,name in files.items():
        entry=b['images'][key];src=ROOT/entry['path'];assert sha(src)==entry['sha256']
        assert boot['images'][key]==entry['sha256']
        shutil.copyfile(src,dest/name);manifest['images'][key]={'file':name,**{k:entry[k] for k in ('offset','bytes','sha256')}}
    full='nabo-fortune-v'+a.version+'-full.bin';src=ROOT/b['images']['merged']['path']
    assert sha(src)==b['images']['merged']['sha256'];shutil.copyfile(src,dest/full)
    manifest['full_image']={'file':full,'offset':'0x0','bytes':src.stat().st_size,'sha256':sha(src),'install_replaces_device_data':True}
    (dest/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    for name in ('LICENSE','NOTICE.md','THIRD_PARTY_NOTICES.md'):shutil.copyfile(ROOT/name,dest/name)
    shutil.copytree(ROOT/'licenses',dest/'licenses')
    shutil.copyfile(ROOT/'docs/FLASHING.md',dest/'FLASHING.md')
    shutil.copyfile(ROOT/'tools/flash_release.py',dest/'flash_release.py')
    paths=sorted(p for p in dest.rglob('*') if p.is_file())
    (dest/'bundle-SHA256SUMS.txt').write_text(''.join(sha(p)+'  '+p.relative_to(dest).as_posix()+'\n' for p in paths),encoding='ascii')
    archive=dest/('nabo-fortune-v'+a.version+'.zip')
    with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for file in sorted(p for p in dest.rglob('*') if p.is_file() and p!=archive):
            name=file.relative_to(dest).as_posix();info=zipfile.ZipInfo(name,(2026,10,22,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,file.read_bytes())
    assets=[dest/x for x in files.values()]+[dest/full,archive,dest/'manifest.json']
    (dest/'SHA256SUMS.txt').write_text(''.join(sha(p)+'  '+p.name+'\n' for p in assets),encoding='ascii')
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for line in z.read('bundle-SHA256SUMS.txt').decode('ascii').splitlines():
            expected,name=line.split('  ',1);assert hashlib.sha256(z.read(name)).hexdigest()==expected
        assert not any(n.startswith(('private/','..','/')) for n in z.namelist())
    print(json.dumps({'directory':str(dest),'source_commit':commit,'release_assets':[p.name for p in assets]+['SHA256SUMS.txt'],'zip_bytes':archive.stat().st_size},indent=2))
if __name__=='__main__':main()
