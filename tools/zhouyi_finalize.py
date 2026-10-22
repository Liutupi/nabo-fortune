"""以武英殿本爻辞为准，标点取 sunls2 本，异体字规范为通行简体"""
import json, re, sys, opencc
SRC = sys.argv[1]
cc = opencc.OpenCC('t2s')
def s2(t): return cc.convert(t.replace('乾', '')).replace('', '乾')
VAR = {'彚': '汇', '闚': '窥', '咷': '啕', '遯': '遁', '朶': '朵'}
def norm(t): return ''.join(VAR.get(c, c) for c in t)
isc = lambda c: '一' <= c <= '鿿' or c in '𬙊'
def transfer(punct_src, chars):
    """把 punct_src 的标点按汉字位置移到 chars 上（两者汉字数相同）"""
    out = []; k = 0
    for c in punct_src:
        if isc(c): out.append(chars[k]); k += 1
        else: out.append(c)
    assert k == len(chars)
    return ''.join(out)
d = json.load(open('zhouyi_draft.json', encoding='utf-8'))
W = {x['number']: x for x in json.load(open(SRC + '/wuying-lines.json', encoding='utf-8'))['hexagrams']}
log = []
for h in d:
    w = W[h['kw']]
    for i, ln in enumerate(h['lines']):
        wt = norm(s2(w['lines'][i].split('：', 1)[1]))
        wt_chars = [c for c in wt if isc(c)]
        cur = ln['text']; cur_chars = [c for c in cur if isc(c)]
        if cur_chars != wt_chars:
            if len(cur_chars) == len(wt_chars):
                new = transfer(cur, wt_chars)
            else:
                new = ''.join(wt_chars) + '。'
            log.append({'kw': h['kw'], 'pos': ln['pos'], 'from': cur, 'to': new})
            ln['text'] = new
    if w.get('special'):
        t = norm(s2(w['special']['text']))
        h['special'] = {'pos': s2(w['special']['label']), 'text': {'见群龙无首吉': '见群龙无首，吉。', '利永贞': '利永贞。'}[t]}
# 卦辞个别处理
for h in d:
    if h['kw'] == 49: log.append({'kw': 49, 'pos': '卦辞', 'from': h['gua'], 'to': h['gua'], 'why': '“己日/巳日”两说，保留通行本“己日”'})
json.dump(d, open('zhouyi.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
json.dump(log, open('zhouyi_edits.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print(len(log), 'edits')
for x in log: print(x['kw'], x['pos'], x['from'], '→', x['to'])
