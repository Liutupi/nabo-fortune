"""Guarded installer regression: no device access; all esptool calls are mocked."""
import contextlib,hashlib,importlib.util,io,json,sys,tempfile,unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('installer',ROOT/'tools/flash_release.py')
installer=importlib.util.module_from_spec(spec);spec.loader.exec_module(installer)
MAC='02:00:00:00:00:01'
class ReleaseInstaller(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.folder=Path(self.tmp.name);self.calls=[];self.device_mac=MAC;self.bad_layout=False
        self.images={};self.table=b'partitions'+b'\xff'*3062
        for key,off,raw in [('bootloader','0x0',b'boot'),('partition_table','0x8000',self.table),('app','0x10000',b'app'),('assets','0x200000',b'assets')]:
            name=key+'.bin';(self.folder/name).write_bytes(raw);self.images[key]={'file':name,'offset':off,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
        (self.folder/'manifest.json').write_text(json.dumps({'chip':'esp32c3','flash_bytes':0x800000,'images':self.images}))
    def tearDown(self):self.tmp.cleanup()
    def run_esptool(self,cmd,**kwargs):
        op=cmd[cmd.index('--after')+2:];self.calls.append(op)
        if op[0]=='read_flash':
            self.assertEqual(op[1:3],['0x8000','0x1000'],'Unexpected full-Flash backup')
            Path(op[3]).write_bytes((b'bad'+self.table[3:] if self.bad_layout else self.table).ljust(4096,b'\xff'))
        return SimpleNamespace(returncode=0,stdout=f'MAC: {self.device_mac}\nDetected flash size: 8MB\nSecure Boot: Disabled\nFlash Encryption: Disabled\n')
    def invoke(self,mode='upgrade',extra=()):
        args=['flash_release.py','--mode',mode,'--port','test-port','--expected-mac',MAC,'--release-dir',str(self.folder),*extra]
        with patch.object(sys,'argv',args),patch.object(installer.subprocess,'run',side_effect=self.run_esptool),contextlib.redirect_stdout(io.StringIO()),contextlib.redirect_stderr(io.StringIO()):installer.main()
    def test_upgrade_preserves_nvs_without_backup_or_erase(self):
        self.invoke();ops=[c[0] for c in self.calls];self.assertNotIn('erase_flash',ops)
        write=next(c for c in self.calls if c[0]=='write_flash');self.assertIn('0x10000',write);self.assertIn('0x200000',write);self.assertNotIn('0x0',write);self.assertNotIn('0x9000',write)
        self.assertFalse((self.folder/'private').exists())
    def test_install_uses_segments_without_whole_chip_erase(self):
        self.invoke('install',['--confirm-full-install']);self.assertNotIn('erase_flash',[c[0] for c in self.calls]);write=next(c for c in self.calls if c[0]=='write_flash');self.assertTrue(all(x in write for x in ['0x0','0x8000','0x10000','0x200000']));self.assertNotIn('--erase-all',write)
    def test_install_requires_explicit_choice(self):
        with self.assertRaises(SystemExit):self.invoke('install')
        self.assertFalse(self.calls)
    def test_wrong_mac_rejects_before_write(self):
        self.device_mac='02:00:00:00:00:02'
        with self.assertRaises(RuntimeError):self.invoke()
        self.assertNotIn('write_flash',[c[0] for c in self.calls])
    def test_wrong_layout_rejects_before_write(self):
        self.bad_layout=True
        with self.assertRaises(RuntimeError):self.invoke()
        self.assertNotIn('write_flash',[c[0] for c in self.calls])
    def test_corrupt_artifact_rejects_before_device_access(self):
        (self.folder/'app.bin').write_bytes(b'corrupt')
        with self.assertRaises(AssertionError):self.invoke()
        self.assertFalse(self.calls)
if __name__=='__main__':unittest.main()
