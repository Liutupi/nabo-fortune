"""NABO 赛博算命机 · 点阵字体子集（4 bpp），供固件直绘文字使用

python tools/gen_fonts_fortune.py

字符集从 prototype/index.html（含签文、称骨歌、解读文案、界面文字）自动统计，不手写。
输出 game-art/fortune/sprites/fonts/<名称>.fnt 和 fonts.json（源字体 SHA256、字号、字数、字节数）。

FNFT v1 格式（小端）：
  头   : 4s 'FNFT' | H 版本 | H 字形数 | B 行高 | B 基线(ascent) | B 空格宽 | B 保留
  索引 : 每个字形 13 字节，按码位升序：I 码位 | I 数据偏移(相对文件头) | B 宽 | B 高 | b x偏移 | b y偏移(相对行顶) | B 前进宽度
         （前进宽度 = 宽 + x偏移 + 1，CJK 统一为字号宽度，见 adv 规则）
  数据 : 每字形按行存 4 bpp（高 4 位在前），每行字节数 = (宽+1)//2
"""
import hashlib, json, os, re, struct
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, 'tools', 'fonts')
OUT = os.path.join(ROOT, 'game-art', 'fortune', 'sprites', 'fonts')

FACES = {   # 名称: (文件, 像素字号)
    # Stable resource IDs; physical-device feedback requires readable 14px minimum.
    'kai15': ('LXGWWenKai-Regular.ttf', 20),
    'kai13': ('LXGWWenKai-Regular.ttf', 17),
    'kai10': ('LXGWWenKai-Regular.ttf', 14),
    'sans11': ('SourceHanSansCN-Medium.otf', 16),
    'sans9': ('SourceHanSansCN-Medium.otf', 14),
    'disp24': ('ZCOOLQingKeHuangYou-Regular.ttf', 26),
    'disp13': ('ZCOOLQingKeHuangYou-Regular.ttf', 18),
    'mono10': ('SpaceMono-Bold.ttf', 14),
}


SYMS = '·—–…“”‘’★☆○×→↑↓▲▼'


def charset():
    """原型 HTML（签文、卦爻辞、解读、界面字）+ 固件 C 源码里的字符串（名称表、界面字）+ 打进总包的签文/周易文本"""
    s = open(os.path.join(ROOT, 'prototype', 'index.html'), encoding='utf-8').read()
    s = re.sub(r'<script src=[^>]*></script>', '', s)
    for d, _, fs in os.walk(os.path.join(ROOT, 'firmware', 'main', 'fortune')):
        for f in fs:
            if f.endswith('.c') or f.endswith('.h'):
                s += ''.join(re.findall(r'"((?:[^"\\\n]|\\.)*)"', open(os.path.join(d, f), encoding='utf-8').read()))
    for j in ('guanyin/guanyin_lots.json', 'zhouyi/zhouyi.json'):    # 打进总包的文本表
        s += open(os.path.join(ROOT, 'game-art', 'fortune', 'text', j), encoding='utf-8').read()
    cs = set(ch for ch in s if ('\u3000' <= ch <= '\u9fff') or ('\uff00' <= ch <= '\uffef') or ord(ch) >= 0x20000 or ch in SYMS)
    cs |= set(chr(c) for c in range(0x20, 0x7f))
    return sorted(cs)


FALLBACK = 'SourceHanSansCN-Medium.otf'


def cmap(path):
    from fontTools.ttLib import TTFont
    return TTFont(path, fontNumber=0).getBestCmap()


def build(name, file, px, chars):
    path = os.path.join(FONTS, file)
    font = ImageFont.truetype(path, px)
    fb = ImageFont.truetype(os.path.join(FONTS, FALLBACK), px)
    have = cmap(path); fb_used = []
    asc, desc = font.getmetrics()
    line_h = asc + desc
    glyphs = []
    for ch in chars:
        if ch == ' ':
            continue
        f = font
        if ord(ch) not in have:
            f = fb; fb_used.append(ch)   # 主字体缺字：用思源黑体同字号
        bbox = f.getbbox(ch)  # 相对基线原点的包围盒（y 以行顶为 0）
        if not bbox or bbox[2] <= bbox[0]:
            continue
        x0, y0, x1, y1 = bbox
        w, h = x1 - x0, y1 - y0
        im = Image.new('L', (w, h), 0)
        ImageDraw.Draw(im).text((-x0, -y0), ch, fill=255, font=f)
        px4 = [(v + 8) // 17 for v in im.tobytes()]  # 0..15
        rowb = (w + 1) // 2
        data = bytearray()
        for y in range(h):
            row = px4[y * w:(y + 1) * w] + [0]
            for x in range(0, w, 2):
                data.append((row[x] << 4) | row[x + 1])
        adv = int(round(f.getlength(ch)))
        glyphs.append((ord(ch), w, h, x0, y0, adv, bytes(data)))
    head = struct.pack('<4sHHBBBB', b'FNFT', 1, len(glyphs), line_h, asc, int(round(font.getlength(' '))), 0)
    idx = bytearray(); body = bytearray(); base = len(head) + 13 * len(glyphs)
    for cp, w, h, x0, y0, adv, data in glyphs:
        idx += struct.pack('<IIBBbbB', cp, base + len(body), w, h, x0, y0, adv)
        body += data
    blob = head + bytes(idx) + bytes(body)
    os.makedirs(OUT, exist_ok=True)
    open(os.path.join(OUT, name + '.fnt'), 'wb').write(blob)
    return dict(source=file, source_sha256=hashlib.sha256(open(path, 'rb').read()).hexdigest(), px=px,
                glyphs=len(glyphs), line_h=line_h, ascent=asc, bytes=len(blob), fallback=''.join(fb_used))


def main():
    chars = charset()
    info = {}
    for name, (f, px) in FACES.items():
        # 等宽和标题字体只放用得到的字：mono 只要 ASCII
        cs = [c for c in chars if ord(c) < 0x80] if name.startswith('mono') else chars
        info[name] = build(name, f, px, cs)
        print(name, info[name]['glyphs'], '字', info[name]['bytes'] // 1024, 'KB', flush=True)
    info['_charset_size'] = len(chars)
    json.dump(info, open(os.path.join(OUT, 'fonts.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    print('合计', sum(v['bytes'] for k, v in info.items() if not k.startswith('_')) // 1024, 'KB')


if __name__ == '__main__':
    main()
