"""周易卦爻辞：三源校对 → 定稿 JSON（简体，带标点）"""
import json, re, sys, opencc
SRC = sys.argv[1]
cc = opencc.OpenCC('t2s')
def s2(t): return cc.convert(t.replace('乾', '')).replace('', '乾')
def bare(t): return re.sub(r'[^一-鿿]', '', t)
W = json.load(open(SRC + '/wuying-lines.json', encoding='utf-8'))['hexagrams']
H = json.load(open(SRC + '/iching-64-hexagrams.zh.json', encoding='utf-8'))['hexagrams']
D = json.load(open(SRC + '/sunls2_docs.json', encoding='utf-8'))
docs = {int(k.split('.')[0]): v for k, v in D.items() if k.split('.')[0].isdigit()}
TRI = {'乾': 0, '兑': 1, '离': 2, '震': 3, '巽': 4, '坎': 5, '艮': 6, '坤': 7}
POS = r'(初九|初六|九二|六二|九三|六三|九四|六四|九五|六五|上九|上六|用九|用六)'
out = []; notes = []
for n in range(1, 65):
    w = next(x for x in W if x['number'] == n); h = next(x for x in H if x['number'] == n); d = docs[n]
    up, lo = TRI[h['upper_trigram'][0]], TRI[h['lower_trigram'][0]]
    gm = re.search(r'\*\*([^*。]{1,4})。\*\*\s*([^\n]+)', d)
    gua_trad = gm.group(2).strip(); gua = s2(gua_trad)
    gua_h = re.sub(r'^[^：:]+[：:]', '', h['judgment'])
    if bare(gua) != bare(gua_h): notes.append((n, '卦辞', gua, gua_h))
    lines = []
    found = dict()
    for m in re.finditer(r'\*\*' + POS + r'。\*\*\s*([^\n]+)', d):
        found.setdefault(m.group(1), m.group(2).strip())
    order = [k for k in re.findall(POS, ' '.join(found.keys())) if k not in ('用九', '用六')]
    labels = [l.split('：')[0] for l in w['lines']]
    for i, lab in enumerate(labels):
        lab_s = s2(lab)
        txt = s2(found.get(lab, ''))
        wtxt = s2(w['lines'][i].split('：', 1)[1])
        if bare(txt) != bare(wtxt): notes.append((n, lab_s, txt, wtxt)); txt = txt if txt else wtxt
        lines.append({'pos': lab_s, 'text': txt})
    sp = None
    if w.get('special'):
        lab = w['special']['label']; txt = s2(found.get(lab, '')); wtxt = s2(w['special']['text'])
        if bare(txt) != bare(wtxt): notes.append((n, lab, txt, wtxt))
        sp = {'pos': s2(lab), 'text': txt or wtxt}
    out.append({'kw': n, 'up': up, 'lo': lo, 'name': h['chinese'], 'gua': gua, 'lines': lines, 'special': sp})
json.dump(out, open('zhouyi_draft.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('done', len(out), 'notes', len(notes))
for x in notes: print(x)
