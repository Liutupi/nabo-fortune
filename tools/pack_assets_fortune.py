"""NABO 赛博算命机 · 素材总包 assets.bin

python tools/pack_assets_fortune.py

收录：
  spr/<片段>/<尺寸>   NABO 动画（FNBO）        sprites/<ID>/<尺寸>/frames.bin
  prop/<名称>          静态道具与塔罗（FNBR）    sprites/props/*.bin
  font/<名称>          点阵字体（FNFT）          sprites/fonts/*.fnt
  txt/<表名>           文本表（FNTX）            由签文库、周易库生成
FNAS v1：头 '4s FNAS | H 版本 | H 条目数'，目录每条 40 字节（32 字节名称，NUL 补齐 | I 偏移 | I 长度），数据 4 字节对齐。
FNTX v1：'4s FNTX | H 条数'，随后 I 偏移 × 条数（相对表头），字符串 UTF-8 以 NUL 结尾。
输出 firmware/assets/fortune_assets.bin 与 assets_manifest.json。
"""
import glob, hashlib, json, os, struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SP = os.path.join(ROOT, 'game-art', 'fortune', 'sprites')
TX = os.path.join(ROOT, 'game-art', 'fortune', 'text')
OUT = os.path.join(ROOT, 'firmware', 'assets')


def fntx(strings):
    head = struct.pack('<4sH', b'FNTX', len(strings))
    base = len(head) + 4 * len(strings)
    offs = bytearray(); body = bytearray()
    for s in strings:
        offs += struct.pack('<I', base + len(body)); body += s.encode('utf-8') + b'\0'
    return head + bytes(offs) + bytes(body)


def texts():
    lots = json.load(open(os.path.join(TX, 'guanyin', 'guanyin_lots.json'), encoding='utf-8'))
    zy = json.load(open(os.path.join(TX, 'zhouyi', 'zhouyi.json'), encoding='utf-8'))
    T = {
        'lot_story': [x['story'] for x in lots],
        'lot_poem': ['\n'.join(x['poem']) for x in lots],
        'lot_jie': [' '.join(x['jie']) for x in lots],
        'lot_bh': [x['bh'] for x in lots],
        'lot_xiang': [x['xiang'] for x in lots],
    }
    by = [None] * 64
    for h in zy: by[h['up'] * 8 + h['lo']] = h
    T['zy_gua'] = [h['gua'] for h in by]
    T['zy_bh'] = [h['bh'] for h in by]
    T['zy_line'] = [by[i]['lines'][p]['pos'] + '\t' + by[i]['lines'][p]['text'] for i in range(64) for p in range(6)]
    T['zy_special'] = [(by[i]['special']['pos'] + '\t' + by[i]['special']['text']) if by[i]['special'] else '' for i in range(64)]
    return {k: fntx(v) for k, v in T.items()}


def main():
    items = []
    for p in sorted(glob.glob(os.path.join(SP, 'N-*', '*', 'frames.bin'))):
        cid, size = p.split(os.sep)[-3:-1]
        items.append(('spr/%s/%s' % (cid, size), open(p, 'rb').read()))
    for p in sorted(glob.glob(os.path.join(SP, 'props', '*.bin'))):
        items.append(('prop/' + os.path.basename(p)[:-4], open(p, 'rb').read()))
    for p in sorted(glob.glob(os.path.join(SP, 'fonts', '*.fnt'))):
        items.append(('font/' + os.path.basename(p)[:-4], open(p, 'rb').read()))
    for k, v in texts().items():
        items.append(('txt/' + k, v))
    voice = os.path.join(ROOT, 'game-art', 'fortune', 'audio', 'boot-welcome.wav')
    if os.path.isfile(voice):
        import wave
        with wave.open(voice, 'rb') as w:
            assert (w.getframerate(), w.getnchannels(), w.getsampwidth()) == (16000, 1, 2)
            items.append(('snd/boot-welcome', w.readframes(w.getnframes())))
    for n, _ in items:
        assert len(n.encode()) < 32, n
    head = struct.pack('<4sHH', b'FNAS', 1, len(items))
    pos = len(head) + 40 * len(items)
    pos = (pos + 3) & ~3
    dirb = bytearray(); data = bytearray(); man = {}
    for n, b in items:
        off = pos + len(data)
        dirb += n.encode().ljust(32, b'\0') + struct.pack('<II', off, len(b))
        data += b; data += b'\0' * ((-len(data)) % 4)
        man[n] = dict(offset=off, size=len(b))
    blob = head + bytes(dirb)
    blob += b'\0' * (((len(head) + len(dirb) + 3) & ~3) - len(blob))
    blob += bytes(data)
    os.makedirs(OUT, exist_ok=True)
    open(os.path.join(OUT, 'fortune_assets.bin'), 'wb').write(blob)
    sha = hashlib.sha256(blob).hexdigest()
    json.dump(dict(bytes=len(blob), sha256=sha, count=len(items), items=man), open(os.path.join(OUT, 'assets_manifest.json'), 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    print('条目', len(items), '大小', len(blob) // 1024, 'KB', '分区占用 %.0f%%' % (len(blob) * 100 / (6 * 1024 * 1024)), sha[:12])


if __name__ == '__main__':
    main()
