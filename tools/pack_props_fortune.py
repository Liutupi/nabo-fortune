"""NABO 赛博算命机 · Codex 静态素材 → 固件打包（FNBO v1，与 NABO 精灵帧同格式）

python tools/pack_props_fortune.py

- 签筒：以底部中心为轴，-16°…+16° 共 11 个倾角帧（脚本生成，Codex 只画了正面一张）
- 铜钱：字面 → 侧边 → 背面 16 帧横向缩放翻转
- 塔罗：22 张牌面 + 大小牌背，各自一套调色板
- 其余道具：单帧
输出 game-art/fortune/sprites/props/<名称>.bin，清单 props_manifest.json，预览 qa/props_packed.png
"""
import json, math, os, struct, sys
import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import clip_to_sprites_fortune as C   # 复用量化、打包、回解

ROOT = C.ROOT
PROPS = os.path.join(C.ART, 'props')
TAROT = os.path.join(C.ART, 'tarot')
OUT = os.path.join(C.OUT, 'props')


def load(path):
    a = np.asarray(Image.open(path).convert('RGBA'), np.float32)
    return a[..., :3], a[..., 3] / 255


def pack_frames(name, frames, w, h, loop=False):
    cols, als = zip(*frames)
    pal, idx = C.quantize(cols, als)
    # 非正方形：pack 按行处理，size 参数只用于行数和宽度一致的情况，这里逐帧自己打
    blobs = []
    for k, f in enumerate(idx):
        rows = [C.rle_row(r) if k == 0 else C.delta_row(idx[k - 1][y], r) for y, r in enumerate(f)]
        hdr = bytearray(); body = bytearray()
        for r in rows:
            hdr += struct.pack('<H', 2 * h + len(body)); body += r
        blobs.append(bytes(hdr + body))
    prev = None; ok = True
    for k, bl in enumerate(blobs):
        img = decode_rect(bl, prev, w, h, k > 0)
        ok &= bool(np.array_equal(img, idx[k])); prev = img
    head = struct.pack('<4sHHHHB', b'FNBR', 1, w, h, len(blobs), 1 if loop else 0)
    palb = b''.join(struct.pack('<H', C.rgb565(p)) for p in pal)
    offs = bytearray(); body = bytearray(); base = len(head) + len(palb) + 4 * len(blobs)
    for bl in blobs:
        offs += struct.pack('<I', base + len(body)); body += bl
    data = head + palb + bytes(offs) + bytes(body)
    os.makedirs(OUT, exist_ok=True)
    open(os.path.join(OUT, name + '.bin'), 'wb').write(data)
    err = []
    for k in range(len(idx)):
        m = idx[k] > 0
        mix = cols[k] * als[k][..., None] + C.BG * (1 - als[k][..., None])
        if m.any(): err.append(float(np.abs(pal[idx[k][m] - 1].astype(np.float32) - mix[m]).mean()))
    prev_img = []
    for f in idx:
        rgba = np.zeros((h, w, 4), np.uint8); m = f > 0
        rgba[m, :3] = pal[f[m] - 1]; rgba[m, 3] = 255; prev_img.append(Image.fromarray(rgba))
    return dict(w=w, h=h, frames=len(blobs), bytes=len(data), roundtrip_ok=ok, mean_color_err=round(float(np.mean(err)) if err else 0, 2)), prev_img


def decode_rect(data, prev, w, h, delta):
    img = prev.copy() if delta else np.zeros((h, w), np.uint8)
    offs = struct.unpack_from('<%dH' % h, data, 0)
    for y in range(h):
        p = offs[y]; x = 0
        while x < w:
            c = data[p]; p += 1; L = (c & 0x7F) + 1
            if c & 0x80:
                if not delta: img[y, x:x + L] = 0
                x += L
            else:
                img[y, x:x + L] = np.frombuffer(bytes(data[p:p + L]), np.uint8); p += L; x += L
    return img


def tube_tilts():
    im = Image.open(os.path.join(PROPS, 'tube.png')).convert('RGBA')
    W, H = 96, 116                                    # 倾斜后的外框，轴心在 (48, 112)
    out = []
    for k in range(11):
        deg = -16 + k * 3.2
        canvas = Image.new('RGBA', (W * 4, H * 4), (0, 0, 0, 0))
        big = im.resize((64 * 4, 110 * 4), Image.NEAREST)
        canvas.alpha_composite(big, ((48 - 32) * 4, (112 - 110) * 4))
        rot = canvas.rotate(-deg, resample=Image.BICUBIC, center=(48 * 4, 112 * 4))
        arr = np.asarray(rot, np.float32); a4 = arr[..., 3] / 255; pre = arr[..., :3] * a4[..., None]
        al = a4.reshape(H, 4, W, 4).mean(axis=(1, 3))          # 4×4 面积平均（预乘 alpha，避免边缘发暗）
        pm = pre.reshape(H, 4, W, 4, 3).mean(axis=(1, 3))
        col = np.where(al[..., None] > 0, pm / np.maximum(al[..., None], 1e-6), 0)
        out.append((col, al))
    return out, W, H


def coin_flip():
    fr = Image.open(os.path.join(PROPS, 'coin-front.png')).convert('RGBA')
    bk = Image.open(os.path.join(PROPS, 'coin-back.png')).convert('RGBA')
    out = []
    for k in range(16):
        ang = k / 16 * math.pi
        sx = abs(math.cos(ang)); src = fr if math.cos(ang) >= 0 else bk
        w = max(2, int(round(22 * sx)))
        sq = src.resize((w * 4, 88), Image.BOX).resize((w, 22), Image.BOX)
        canvas = Image.new('RGBA', (22, 22), (0, 0, 0, 0)); canvas.alpha_composite(sq, ((22 - w) // 2, 0))
        if w <= 3:  # 侧边：画一条铜色边
            d = np.asarray(canvas).copy(); d[1:21, 10:12] = [138, 90, 18, 255]; canvas = Image.fromarray(d)
        a = np.asarray(canvas, np.float32); out.append((a[..., :3], a[..., 3] / 255))
    return out


def main():
    man = {}; previews = []
    fr, W, H = tube_tilts()
    man['tube-tilt'], pv = pack_frames('tube-tilt', fr, W, H); man['tube-tilt']['deg'] = [round(-16 + k * 3.2, 1) for k in range(11)]; man['tube-tilt']['pivot'] = [48, 112]; previews += pv
    man['coin-flip'], pv = pack_frames('coin-flip', coin_flip(), 22, 22, loop=True); previews += pv
    singles = ['shell', 'stick', 'scroll', 'seal-shang', 'seal-zhong', 'seal-xia', 'scale-beam', 'scale-pan', 'scale-weight',
               'weight-nian', 'weight-yue', 'weight-ri', 'weight-shi', 'printer', 'card-back-s', 'card-back-l', 'title'] + ['trigram-%d' % i for i in range(8)]
    for n in singles:
        col, al = load(os.path.join(PROPS, n + '.png'))
        man[n], pv = pack_frames(n, [(col, al)], al.shape[1], al.shape[0]); previews += pv
    for f in sorted(os.listdir(TAROT)):
        if not f[:2].isdigit() or not f.endswith('.png'): continue
        col, al = load(os.path.join(TAROT, f)); n = 'tarot-' + f[:-4]
        man[n], pv = pack_frames(n, [(col, al)], al.shape[1], al.shape[0]); previews += pv
    tot = sum(v['bytes'] for v in man.values())
    man['_note'] = 'FNBR v1：头(4s 魔数,H 版本,H 宽,H 高,H 帧数,B 循环)、RGB565 调色板(63)、帧偏移、每帧行偏移+RLE（第0帧关键帧，其余差分）'
    json.dump(man, open(os.path.join(OUT, 'props_manifest.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    # 预览
    Wp = 700; x = y = 4; rh = 0; sheet = Image.new('RGBA', (Wp, 1600), (13, 8, 32, 255))
    for im in previews:
        if x + im.width > Wp - 4: x = 4; y += rh + 4; rh = 0
        sheet.alpha_composite(im, (x, y)); x += im.width + 4; rh = max(rh, im.height)
    sheet.crop((0, 0, Wp, y + rh + 4)).convert('RGB').save(os.path.join(C.OUT, 'qa', 'props_packed.png'))
    bad = [k for k, v in man.items() if not k.startswith('_') and not v['roundtrip_ok']]
    print('素材', len(man) - 1, '项，合计', tot // 1024, 'KB，回解失败', bad or '无')
    for k in ['tube-tilt', 'coin-flip', 'scroll', 'tarot-00-fool']:
        print(' ', k, man[k])


if __name__ == '__main__':
    main()
