"""检查固件界面用到的每个字在对应字体里都有字形。
用法：python check_glyphs.py <assets.bin> <assets_manifest.json> <firmware/main/fortune>"""
import json, os, re, struct, sys
blob = open(sys.argv[1], 'rb').read(); man = json.load(open(sys.argv[2]))['items']; src = sys.argv[3]
def font(name):
    o = man['font/' + name]['offset']; n = struct.unpack('<H', blob[o + 6:o + 8])[0]
    return {struct.unpack('<I', blob[o + 12 + 13 * i:o + 16 + 13 * i])[0] for i in range(n)}
def txt(name):
    o = man['txt/' + name]['offset']; n = struct.unpack('<H', blob[o + 4:o + 6])[0]
    offs = struct.unpack('<%dI' % n, blob[o + 6:o + 6 + 4 * n])
    return ''.join(blob[o + x:blob.index(b'\0', o + x)].decode() for x in offs)
chars = set()
for root, _, fs in os.walk(src):
    for f in fs:
        if f.endswith('.c'):
            for m in re.finditer(r'"((?:[^"\\\n]|\\.)*)"', open(os.path.join(root, f), encoding='utf-8').read()): chars |= set(m.group(1))
for k in man:
    if k.startswith('txt/'): chars |= set(txt(k[4:]))
chars = {c for c in chars if ord(c) > 32 and c not in '\t\n'}
bad = 0
for fn in ['kai15', 'kai13', 'kai10', 'sans11', 'sans9', 'disp24', 'disp13']:
    miss = sorted(c for c in chars if ord(c) not in font(fn) and not c.isascii() or (c.isascii() and ord(c) not in font(fn)))
    if miss: bad += 1; print(fn, len(miss), ''.join(miss)[:200])
print('chars', len(chars), 'fonts with gaps', bad)
