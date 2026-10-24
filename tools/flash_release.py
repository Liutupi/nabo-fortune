"""Guarded ESP32-C3 install/upgrade from an extracted release ZIP.

Requires esptool 4.x and an explicit MAC. Upgrade preserves NVS and checks
the exact layout. A full-flash backup is optional; no mode erases the chip.
"""
import argparse,datetime,hashlib,json,re,subprocess,sys,tempfile
from pathlib import Path

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode',choices=('inspect','install','upgrade'),required=True)
    p.add_argument('--port',required=True);p.add_argument('--expected-mac',required=True)
    p.add_argument('--release-dir',type=Path,default=Path(__file__).resolve().parent)
    p.add_argument('--backup-dir',type=Path,default=None,help='Optional, explicitly requested local full-Flash backup')
    p.add_argument('--confirm-full-install',action='store_true',help='Allow replacing firmware/partition layout; no whole-chip erase is performed')
    a=p.parse_args();mac=a.expected_mac.lower()
    if not re.fullmatch(r'(?:[0-9a-f]{2}:){5}[0-9a-f]{2}',mac):p.error('Expected MAC must be xx:xx:xx:xx:xx:xx')
    if a.mode=='install' and not a.confirm_full_install:p.error('Install replaces firmware/layout; --confirm-full-install is required')
    folder=a.release_dir.resolve();m=json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
    assert m['chip']=='esp32c3' and m['flash_bytes']==0x800000
    for v in m['images'].values():
        file=(folder/v['file']).resolve();assert file.is_relative_to(folder)
        assert file.stat().st_size==v['bytes'] and sha(file)==v['sha256'],file.name
    def run(*args,reset='no_reset'):
        cmd=[sys.executable,'-m','esptool','--chip','esp32c3','--port',a.port,'--baud','460800','--after',reset,*map(str,args)]
        r=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,errors='replace')
        print(r.stdout)
        if r.returncode:raise RuntimeError('esptool failed')
        if not re.search(r'MAC:\s*'+re.escape(mac),r.stdout,re.I):raise RuntimeError('Live MAC differs from expected MAC')
        return r.stdout
    info=run('flash_id',reset='hard_reset' if a.mode=='inspect' else 'no_reset')
    if '8MB' not in info:raise RuntimeError('8 MB Flash was not confirmed')
    if a.mode=='inspect':return
    security=run('get_security_info')
    if 'Secure Boot: Disabled' not in security or 'Flash Encryption: Disabled' not in security:
        raise RuntimeError('Unencrypted, non-secure-boot device required')
    backup=None
    if a.backup_dir is not None:
        backup=a.backup_dir.resolve()/(mac.replace(':','')+'-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
        backup.mkdir(parents=True,exist_ok=False)
        run('read_flash','0','0x800000',backup/'before.bin')
        run('verify_flash','0',backup/'before.bin')
        (backup/'backup.json').write_text(json.dumps({'mac':mac,'mode':a.mode,'sha256':sha(backup/'before.bin'),'private':True},indent=2)+'\n')
    if a.mode=='upgrade':
        table=m['images']['partition_table'];start=int(table['offset'],0)
        with tempfile.TemporaryDirectory(prefix='nabo-layout-') as temporary:
            current=Path(temporary)/'partition.bin'
            run('read_flash',hex(start),'0x1000',current)
            if current.read_bytes()[:table['bytes']]!=(folder/table['file']).read_bytes():
                raise RuntimeError('Partition layout differs; refusing upgrade')
        keys=('app','assets')
    else:
        keys=('bootloader','partition_table','app','assets')
    images=[]
    for k in keys:images += [m['images'][k]['offset'],folder/m['images'][k]['file']]
    run('write_flash','--flash_mode','dio','--flash_freq','80m','--flash_size','8MB',*images)
    run('verify_flash',*images,reset='hard_reset')
    print('Verified. NVS preserved by compatible upgrade.' if a.mode=='upgrade' else 'Verified installation; no whole-chip erase performed.')
    if backup is not None:print('Optional personal backup remains only at:',backup)

if __name__=='__main__':main()
