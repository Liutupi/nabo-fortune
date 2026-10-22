"""NABO 赛博算命机 · Grok 视频 → 精灵帧（抽帧、抠图、选段、缩放、64 色、RLE+差分、逐帧回解）

python tools/clip_to_sprites_fortune.py            # 处理全部片段
python tools/clip_to_sprites_fortune.py N-SHELL    # 只处理一段

输入：game-art/fortune/grok-clips/selected-clips.json 指定的视频（只读主视频流 0:v:0）
输出：game-art/fortune/sprites/<ID>/<尺寸>/  sheet.png（透明精灵表）、frames.bin（打包数据）
      game-art/fortune/sprites/manifest.json（帧数、时长、锚点、字节数、校验结果）
      game-art/fortune/sprites/qa/<ID>.png（暗底预览）
"""
import hashlib, json, os, struct, subprocess, sys
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, 'game-art', 'fortune')
CLIPS = os.path.join(ART, 'grok-clips')
OUT = os.path.join(ART, 'sprites')
BG = np.array([0x0D, 0x08, 0x20], np.float32)     # 屏幕底色，半透明边缘预先混合到这个颜色
SRC_FPS, OUT_FPS = 24, 12
HOLD = 30   # 颜色差（RGB 三通道绝对差之和）不超过此值的像素沿用上一帧

# 每段的用法：loop=循环（自动找切点，长度范围单位为源帧），once=一次性动作（按动作量裁剪）
SPEC = {
    'N-IDLE':       dict(kind='loop', span=(60, 96), sizes=[120, 76]),
    'N-WELCOME':    dict(kind='once', sizes=[120]),
    'N-SHELL':      dict(kind='loop', span=(24, 40), sizes=[120]),
    'N-SHELL-TOSS': dict(kind='once', sizes=[120], anchor='shell'),
    'N-TUBE':       dict(kind='loop', span=(30, 60), sizes=[76]),
    'N-WOW':        dict(kind='once', sizes=[76]),
    'N-NOD':        dict(kind='once', sizes=[76, 56]),
    'N-SIGH':       dict(kind='once', sizes=[76]),
    'N-CALC':       dict(kind='loop', span=(36, 60), sizes=[100, 56]),
    'N-PRINT':      dict(kind='once', sizes=[120]),
}


def decode(path):
    cmd = ['ffmpeg', '-v', 'error', '-i', path, '-map', '0:v:0', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-']
    raw = subprocess.check_output(cmd)
    probe = json.loads(subprocess.check_output(['ffprobe', '-v', 'error', '-select_streams', 'v:0', '-show_entries',
                                                'stream=width,height', '-of', 'json', path]))
    w, h = probe['streams'][0]['width'], probe['streams'][0]['height']
    return np.frombuffer(raw, np.uint8).reshape(-1, h, w, 3).astype(np.float32)


def key_green(frames):
    """估计实际绿底颜色（取四边像素中位数），按色距生成软遮罩，再去绿边。"""
    edge = np.concatenate([frames[:, :6].reshape(-1, 3), frames[:, -6:].reshape(-1, 3),
                           frames[:, :, :6].reshape(-1, 3), frames[:, :, -6:].reshape(-1, 3)])
    g = np.median(edge, axis=0)
    d = np.sqrt(((frames - g) ** 2).sum(-1))
    # 绿色优势：G 明显高于 R、B 的像素更可能是背景
    dom = frames[..., 1] - np.maximum(frames[..., 0], frames[..., 2])
    alpha = np.clip((d - 60) / 70, 0, 1)
    alpha = np.where(dom > 70, np.minimum(alpha, 0.0), alpha)
    # 去绿边：半透明和靠近背景的像素，把 G 压到 R、B 的较大值
    spill = (alpha < 1) | (dom > 18)
    rgb = frames.copy()
    lim = np.maximum(rgb[..., 0], rgb[..., 2]) + 8
    rgb[..., 1] = np.where(spill, np.minimum(rgb[..., 1], lim), rgb[..., 1])
    return rgb, alpha, g


def pick_loop(rgb, alpha, lo, hi):
    """在 [lo,hi] 长度范围内找首尾最接近的循环段（比较缩小图）。"""
    small = np.stack([np.asarray(Image.fromarray(np.uint8(f)).resize((60, 60), Image.BOX), np.float32) for f in rgb])
    best = None
    n = len(small)
    for i in range(0, n - lo):
        for L in range(lo, min(hi, n - 1 - i) + 1, 2):
            e = np.abs(small[i] - small[i + L]).mean()
            if best is None or e < best[0]:
                best = (e, i, L)
    e, i, L = best
    return i, i + L, float(e)  # 末帧不含（循环回到首帧）


def pick_once(rgb):
    small = np.stack([np.asarray(Image.fromarray(np.uint8(f)).resize((60, 60), Image.BOX), np.float32) for f in rgb])
    energy = np.abs(small - small[0]).mean(axis=(1, 2, 3))
    thr = max(2.0, energy.max() * 0.18)
    act = np.where(energy > thr)[0]
    a, b = int(max(0, act[0] - 4)), int(min(len(rgb) - 1, act[-1] + 6))
    return a, b + 1, energy.tolist()


def resize(img_rgb, a, size):
    """预乘 alpha 后面积缩放，避免边缘发绿。"""
    pre = np.dstack([img_rgb * a[..., None], a * 255])
    im = Image.fromarray(np.uint8(np.clip(pre, 0, 255)), 'RGBA').resize((size, size), Image.BOX)
    arr = np.asarray(im, np.float32)
    al = arr[..., 3] / 255
    col = np.where(al[..., None] > 0, arr[..., :3] / np.maximum(al[..., None], 1e-6), 0)
    return col, al


def quantize(frames_rgb, frames_a):
    """把整段动作统一量化到 63 色 + 透明。半透明边缘预先混合到屏幕底色。"""
    solid = []
    for col, al in zip(frames_rgb, frames_a):
        mixed = col * al[..., None] + BG * (1 - al[..., None])
        solid.append((np.uint8(np.clip(mixed, 0, 255)), al >= 0.35))
    stack = np.concatenate([s[0][s[1]] for s in solid]).reshape(-1, 1, 3)
    pal_img = Image.fromarray(stack).quantize(colors=63, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    raw = pal_img.getpalette()[:63 * 3]; raw = raw + [0] * (63 * 3 - len(raw))   # 颜色少于 63 时补零
    pal = np.array(raw, np.uint8).reshape(63, 3)
    out = []
    for rgb, m in solid:
        im = Image.fromarray(rgb).quantize(palette=pal_img, dither=Image.Dither.NONE)
        idx = np.asarray(im, np.uint8) + 1
        idx[~m] = 0
        out.append(idx)
    # 时间去噪：颜色变化很小的像素沿用上一帧的索引，去掉视频闪烁、提高差分压缩率
    palf = pal.astype(np.int32)
    for k in range(1, len(out)):
        a, b = out[k - 1], out[k]
        both = (a > 0) & (b > 0) & (a != b)
        d = np.abs(palf[a[both] - 1] - palf[b[both] - 1]).sum(-1)
        hold = np.zeros_like(both); hold[both] = d <= HOLD
        b[hold] = a[hold]
    return pal, out


def rle_row(row):
    """单行 RLE：每段 [长度-1 | 0x80 表示透明跳过]；不透明段后跟像素索引（字面量）。"""
    out = bytearray(); i = 0; n = len(row)
    while i < n:
        j = i
        if row[i] == 0:
            while j < n and row[j] == 0 and j - i < 128: j += 1
            out.append(0x80 | (j - i - 1))
        else:
            while j < n and row[j] != 0 and j - i < 128: j += 1
            out.append(j - i - 1); out += bytes(row[i:j])
        i = j
    return out


def delta_row(prev, cur):
    """差分行：与上一帧相同的像素记为跳过（0x80 段），其余为字面量（0 也作为字面量表示变透明）。"""
    out = bytearray(); i = 0; n = len(cur)
    while i < n:
        j = i
        if cur[i] == prev[i]:
            while j < n and cur[j] == prev[j] and j - i < 128: j += 1
            out.append(0x80 | (j - i - 1))
        else:
            while j < n and cur[j] != prev[j] and j - i < 128: j += 1
            out.append(j - i - 1); out += bytes(cur[i:j])
        i = j
    return out


def decode_rows(data, prev, size, delta):
    img = prev.copy() if delta else np.zeros((size, size), np.uint8)
    p = 0
    offs = struct.unpack_from('<%dH' % size, data, 0)
    for y in range(size):
        p = offs[y]; x = 0
        while x < size:
            c = data[p]; p += 1; L = (c & 0x7F) + 1
            if c & 0x80:
                if not delta: img[y, x:x + L] = 0
                x += L
            else:
                img[y, x:x + L] = np.frombuffer(bytes(data[p:p + L]), np.uint8); p += L; x += L
    return img


def pack(idx_frames, size):
    """FNBO 格式：头 + 调色板(RGB565) + 帧偏移表 + 每帧(行偏移表 + 行数据)。第 0 帧关键帧，其余差分。"""
    blobs = []
    for k, f in enumerate(idx_frames):
        rows = [rle_row(r) if k == 0 else delta_row(idx_frames[k - 1][y], r) for y, r in enumerate(f)]
        hdr = bytearray(); base = 2 * size; body = bytearray()
        for r in rows:
            hdr += struct.pack('<H', base + len(body)); body += r
        blobs.append(bytes(hdr + body))
    return blobs


def rgb565(p):
    return ((int(p[0]) >> 3) << 11) | ((int(p[1]) >> 2) << 5) | (int(p[2]) >> 3)


def shell_anchor(rgb, alpha, fi):
    """龟甲口：在指定帧里找龟甲金色像素的包围盒，口取上缘中心（坐标按 480 源图）。"""
    f = rgb[fi]; a = alpha[fi]
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    gold = (a > 0.9) & (r > 150) & (g > 100) & (g < 200) & (b < 110) & (r - b > 90)
    ys, xs = np.nonzero(gold)
    ys_sorted = np.sort(ys)
    top = int(np.percentile(ys, 2)); cx = float(np.median(xs[ys < top + 30]))
    return [cx, float(top)]


def run(cid):
    spec = SPEC[cid]
    sel = {x['id']: x for x in json.load(open(os.path.join(CLIPS, 'selected-clips.json'), encoding='utf-8'))['items']}
    src = os.path.join(CLIPS, sel[cid]['file'])
    sha = hashlib.sha256(open(src, 'rb').read()).hexdigest()
    assert sha == sel[cid]['sha256'], (cid, 'source hash changed')
    frames = decode(src)
    rgb, alpha, green = key_green(frames)
    info = dict(id=cid, file=sel[cid]['file'], sha256=sha, green=[round(float(v), 1) for v in green])
    if spec['kind'] == 'loop':
        a, b, err = pick_loop(rgb, alpha, *spec['span'])
        info.update(loop=True, src_range=[a, b], seam_error=round(err, 2))
    else:
        a, b, energy = pick_once(rgb)
        info.update(loop=False, src_range=[a, b])
    picks = list(range(a, b, SRC_FPS // OUT_FPS))
    info['frames'] = len(picks); info['ms'] = round(len(picks) * 1000 / OUT_FPS)
    if spec.get('anchor') == 'shell':
        # 抛出事件帧：龟甲最高点（金色像素上缘最小）
        tops = []
        for fi in picks:
            f = rgb[fi]; al = alpha[fi]
            gold = (al > 0.9) & (f[..., 0] > 150) & (f[..., 1] > 100) & (f[..., 1] < 200) & (f[..., 2] < 110) & (f[..., 0] - f[..., 2] > 90)
            ys = np.nonzero(gold)[0]
            tops.append(np.percentile(ys, 2) if len(ys) else 999)
        k = int(np.argmin(tops)); info['release_frame'] = k
        info['anchor_src480'] = shell_anchor(rgb, alpha, picks[k])
    info['sizes'] = {}
    os.makedirs(os.path.join(OUT, 'qa'), exist_ok=True)
    for size in spec['sizes']:
        cols, als = zip(*[resize(rgb[i], alpha[i], size) for i in picks])
        pal, idx = quantize(cols, als)
        blobs = pack(idx, size)
        # 逐帧回解比对
        prev = None; ok = True
        for k, bl in enumerate(blobs):
            img = decode_rows(bl, prev, size, k > 0)
            if not np.array_equal(img, idx[k]): ok = False
            prev = img
        # 颜色误差：量化结果 vs 预混合源
        err = []
        for k in range(len(idx)):
            m = idx[k] > 0
            src_mix = cols[k] * als[k][..., None] + BG * (1 - als[k][..., None])
            if m.any(): err.append(float(np.abs(pal[idx[k][m] - 1].astype(np.float32) - src_mix[m]).mean()))
        d = os.path.join(OUT, cid, str(size)); os.makedirs(d, exist_ok=True)
        # 打包文件
        head = struct.pack('<4sHHHB', b'FNBO', 1, size, len(blobs), 1 if info['loop'] else 0)
        palb = b''.join(struct.pack('<H', rgb565(p)) for p in pal)
        offs = bytearray(); body = bytearray(); base = len(head) + len(palb) + 4 * len(blobs)
        for bl in blobs:
            offs += struct.pack('<I', base + len(body)); body += bl
        data = head + palb + bytes(offs) + bytes(body)
        open(os.path.join(d, 'frames.bin'), 'wb').write(data)
        # 透明精灵表（原型用）
        cols_n = min(len(idx), 12); rows_n = (len(idx) + cols_n - 1) // cols_n
        sheet = np.zeros((rows_n * size, cols_n * size, 4), np.uint8)
        for k, f in enumerate(idx):
            y0, x0 = (k // cols_n) * size, (k % cols_n) * size
            rgba = np.zeros((size, size, 4), np.uint8)
            m = f > 0
            rgba[m, :3] = pal[f[m] - 1]; rgba[m, 3] = 255
            sheet[y0:y0 + size, x0:x0 + size] = rgba
        Image.fromarray(sheet, 'RGBA').save(os.path.join(d, 'sheet.png'), optimize=True)
        info['sizes'][str(size)] = dict(bytes=len(data), cols=cols_n, roundtrip_ok=ok,
                                        mean_color_err=round(float(np.mean(err)), 2), max_frame_err=round(float(np.max(err)), 2))
        if spec.get('anchor') == 'shell':
            info['sizes'][str(size)]['anchor'] = [round(v * size / 480, 1) for v in info['anchor_src480']]
    # QA 预览：最大尺寸，暗底，每帧一格
    size = max(spec['sizes'])
    sh = Image.open(os.path.join(OUT, cid, str(size), 'sheet.png'))
    bg = Image.new('RGBA', sh.size, (13, 8, 32, 255)); bg.alpha_composite(sh); bg.convert('RGB').save(os.path.join(OUT, 'qa', cid + '.png'))
    return info


def main():
    ids = sys.argv[1:] or list(SPEC)
    mf = os.path.join(OUT, 'manifest.json')
    man = json.load(open(mf, encoding='utf-8')) if os.path.exists(mf) else {}
    for cid in ids:
        info = run(cid); man[cid] = info
        s = ', '.join(f"{k}px {v['bytes']//1024}KB 回解{'✓' if v['roundtrip_ok'] else '✗'} 色差{v['mean_color_err']}" for k, v in info['sizes'].items())
        print(f"{cid}: 源帧 {info['src_range']} → {info['frames']} 帧 {info['ms']}ms {'循环 切点误差 ' + str(info.get('seam_error')) if info['loop'] else '一次'} | {s}", flush=True)
    man['_note'] = '由 tools/clip_to_sprites_fortune.py 生成；frames.bin 为 FNBO v1：头、RGB565 调色板(63)、帧偏移、每帧行偏移+RLE（第0帧关键帧，其余差分）'
    json.dump(man, open(mf, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    tot = sum(v['bytes'] for k, i in man.items() if not k.startswith('_') for v in i['sizes'].values())
    print('总计', tot // 1024, 'KB')


if __name__ == '__main__':
    main()
