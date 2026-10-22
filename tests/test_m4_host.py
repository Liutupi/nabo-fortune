"""M4 host checks: synthesized PCM bounds and rejection of unsafe flash bundles."""
import contextlib
import ctypes
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import struct

ROOT = Path(__file__).resolve().parents[1]
FW = ROOT / 'firmware'


def sound_check(tmp):
    lib_path = tmp / ('sound.dll' if os.name == 'nt' else 'sound.so')
    subprocess.run([os.environ.get('CC', 'gcc'), '-std=c11', '-O2', '-Wall', '-Wextra',
                    '-Werror', '-shared', '-fPIC', str(FW/'main/fc_sound.c'),
                    str(FW/'main/fortune/gfx/fc_gfx.c'), str(FW/'main/fortune/gfx/fc_res.c'),
                    '-o', str(lib_path)], check=True)
    lib = ctypes.CDLL(str(lib_path))
    buf = (ctypes.c_int16 * 16000)()
    peaks = []
    for cue in range(11):
        ctypes.memset(buf, 0, ctypes.sizeof(buf))
        lib.fc_sfx_play(cue)
        lib.fc_sound_fill(buf, len(buf))
        peak = max(abs(v) for v in buf)
        assert 0 < peak < 32767, (cue, peak)
        assert all(v == 0 for v in buf[-1600:]), cue
        peaks.append(peak)
    # A burst of every cue must remain finite and die away without a stuck voice.
    for cue in range(11):
        lib.fc_sfx_play(cue)
    ctypes.memset(buf, 0, ctypes.sizeof(buf))
    lib.fc_sound_fill(buf, len(buf))
    assert max(abs(v) for v in buf) < 32767
    assert all(v == 0 for v in buf[-1600:])
    raw=(FW/'assets/fortune_assets.bin').read_bytes()
    storage=ctypes.create_string_buffer(raw)
    lib.fc_res_init.argtypes=[ctypes.c_void_p,ctypes.c_uint32]
    lib.fc_res_init(storage,len(raw))
    voice=None
    for i in range(struct.unpack_from('<H',raw,6)[0]):
        name,off,size=struct.unpack_from('<32sII',raw,8+40*i)
        if name.split(b'\0')[0]==b'snd/boot-welcome':voice=raw[off:off+size]
    assert voice
    samples=struct.unpack('<'+'h'*(len(voice)//2),voice)
    count=22400+len(samples)+1600
    welcome=(ctypes.c_int16*count)()
    lib.fc_sfx_welcome();lib.fc_sound_fill(welcome,count)
    assert list(welcome[22400:22400+len(samples)])==list(samples)
    assert all(v==0 for v in welcome[4800:20800])
    assert all(v==0 for v in welcome[-1600:])
    voice_peak=max(abs(v) for v in welcome)
    assert voice_peak < 32767
    # LoadLibrary holds the DLL open on Windows; release before deleting the temp directory.
    if os.name == 'nt':
        import _ctypes
        _ctypes.FreeLibrary(lib._handle)
    return {'cues': 11, 'peaks': peaks, 'all_cues_nonzero_unclipped_and_end_silent': True,
            'voice_sample_rate':16000,'voice_seconds':len(samples)/16000,'voice_peak':voice_peak,
            'boot_voice_matches_packed_pcm_after_1400ms':True}


def bundle_check(tmp):
    spec = importlib.util.spec_from_file_location('fortune_verify', FW/'tools/verify_firmware.py')
    verify = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = verify
    spec.loader.exec_module(verify)
    build = tmp/'firmware/build'
    build.mkdir(parents=True)
    args = (FW/'build/flash_args').read_text()
    offsets = verify.parse_flash_args(args)
    for name in offsets:
        dst = (build/name).resolve()
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(FW/'build'/name, dst)
    merged_name = 'nabo-fortune-merged.bin'
    shutil.copyfile(FW/'build'/merged_name, build/merged_name)
    (build/'flash_args').write_text(args)
    def run():
        previous = sys.argv
        sys.argv = ['verify_firmware.py', str(build)]
        try:
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                return verify.main()
        finally:
            sys.argv = previous
    assert run() == 0
    rejected = []
    for tag, changed in (
        ('missing_assets', '\n'.join(l for l in args.splitlines() if 'fortune_assets.bin' not in l)),
        ('wrong_assets_offset', args.replace('0x200000', '0x210000')),
    ):
        (build/'flash_args').write_text(changed)
        assert run() == 1, tag
        rejected.append(tag)
    (build/'flash_args').write_text(args)
    asset = next((build/n).resolve() for n in offsets if n.endswith('fortune_assets.bin'))
    raw = asset.read_bytes()
    asset.write_bytes(b'BAD!' + raw[4:])
    assert run() == 1
    rejected.append('broken_assets_header')
    asset.write_bytes(raw)
    merged = (build/merged_name).read_bytes()
    corrupt = bytearray(merged)
    corrupt[0x10000+100] ^= 1
    (build/merged_name).write_bytes(corrupt)
    assert run() == 1
    rejected.append('merged_app_differs_from_build')
    (build/merged_name).write_bytes(merged[:0x10100])
    assert run() == 1
    rejected.append('truncated_merged_image')
    return {'valid_bundle_passed': True, 'unsafe_bundles_rejected': rejected}


with tempfile.TemporaryDirectory(prefix='fortune-m4-') as directory:
    tmp = Path(directory)
    result = {'sound': sound_check(tmp), 'bundle': bundle_check(tmp)}
dest = ROOT/'validation/m4-host/host-checks.json'
dest.parent.mkdir(parents=True, exist_ok=True)
dest.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
