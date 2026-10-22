"""宿主机跑固件 UI：按脚本驱动按键与时间，逐帧校验 增量合成 == 整屏合成，并导出 PNG。
用法：python run_ui.py <assets.bin> <out_dir>"""
import ctypes, os, sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
L = ctypes.CDLL(os.path.join(HERE, 'libfchost.so'))
L.host_fb.restype = ctypes.POINTER(ctypes.c_uint16)
L.host_scene.restype = ctypes.c_char_p
UP, DOWN, OK = 0, 1, 2

class UI:
    def __init__(s, blob, seed=12345):
        r = L.host_init(blob.encode(), seed); assert r == 0, r
        s.t = 0; s.bad = 0; s.frames = 0; s.strips = 0; s.maxops = 0
    def run(s, ms, dt=16, shots=None, out=None, tag=''):
        """推进 ms 毫秒；shots: 相对时间列表，到点导出"""
        shots = sorted(shots or []); el = 0
        while el < ms:
            L.host_step(dt); el += dt; s.t += dt
            mm = ctypes.c_int(); ops = ctypes.c_int()
            n = L.host_frame(0, ctypes.byref(mm), ctypes.byref(ops))
            s.frames += 1; s.strips += n; s.maxops = max(s.maxops, ops.value & 0xFFFF)
            assert not (ops.value >> 30), 'display list overflow'
            if mm.value: s.bad += 1; print('MISMATCH', tag, el, mm.value)
            while shots and el >= shots[0]:
                s.png(os.path.join(out, f'{tag}_{shots.pop(0):05d}.png'))
    def key(s, k, down): L.host_key(k, 1 if down else 0)
    def tap(s, k, hold=60):
        s.key(k, 1); s.run(hold); s.key(k, 0)
    def go(s, sid): L.host_go(sid.encode())
    def png(s, path):
        a = np.ctypeslib.as_array(L.host_fb(), shape=(320 * 240,)).astype(np.uint32).reshape(320, 240)
        rgb = np.stack([((a >> 11) & 31) * 255 // 31, ((a >> 5) & 63) * 255 // 63, (a & 31) * 255 // 31], -1).astype(np.uint8)
        Image.fromarray(rgb).save(path)

def sheet(paths, out, cols=6, scale=1):
    ims = [Image.open(p) for p in paths]; w, h = ims[0].size; rows = (len(ims) + cols - 1) // cols
    S = Image.new('RGB', (cols * (w + 4) + 4, rows * (h + 4) + 4), (40, 40, 40))
    for i, im in enumerate(ims): S.paste(im, (4 + (i % cols) * (w + 4), 4 + (i // cols) * (h + 4)))
    if scale != 1: S = S.resize((S.width * scale, S.height * scale), Image.NEAREST)
    S.save(out)

if __name__ == '__main__':
    blob, out = sys.argv[1], sys.argv[2]; os.makedirs(out, exist_ok=True)
    u = UI(blob)
    u.run(3200, shots=[200, 350, 520, 700, 900, 1200, 1500, 1800, 2400, 3200], out=out, tag='boot')
    u.tap(OK); u.run(800, shots=[800], out=out, tag='menu0')
    for i in range(3): u.tap(DOWN); u.run(400, shots=[100, 400], out=out, tag=f'menu{i+1}')
    u.go('lot'); L.host_set_lot(int(os.environ.get('LOT', '1')))
    u.go('lot'); u.run(300, shots=[300], out=out, tag='lot_ready')
    u.key(OK, 1); u.run(1800, shots=[400, 1000, 1800], out=out, tag='lot_hold'); u.key(OK, 0)
    u.run(4600, shots=[150, 400, 700, 1000, 1300, 1600, 1900, 2200, 2500, 2800, 3300, 4600], out=out, tag='lot_after')
    print(f'frames={u.frames} mismatchFrames={u.bad} avgStrips={u.strips / u.frames:.1f}/40 maxOps={u.maxops} cache={L.host_cache_bytes()}')
    import glob
    sheet(sorted(glob.glob(os.path.join(out, 'boot_*.png'))) + sorted(glob.glob(os.path.join(out, 'menu*.png'))), os.path.join(out, 'sheet_boot_menu.png'), cols=8)
    sheet(sorted(glob.glob(os.path.join(out, 'lot_*.png'))), os.path.join(out, 'sheet_lot.png'), cols=8)
