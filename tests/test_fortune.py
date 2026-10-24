"""NABO 赛博算命机 · 纯逻辑对拍测试（宿主机，不碰设备）

用 ctypes 编译 firmware/main/fortune/*.c，逐条重放 tests/fortune_host/golden/*.json。
  python tests/test_fortune.py            # 全部对拍
  python tests/test_fortune.py --mutate   # 变异测试：故意改坏规则，确认每一处都会被抓到
"""
import ctypes as C, json, os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'firmware', 'main', 'fortune')
GOLD = os.path.join(ROOT, 'tests', 'fortune_host', 'golden')
FILES = ['fortune_tables.c', 'fortune_calendar.c', 'fortune_misc.c', 'fortune_ssq.c', 'fortune_qimen.c', 'fortune_ziwei.c']

i8, u8, i16, u32, bl = C.c_int8, C.c_uint8, C.c_int16, C.c_uint32, C.c_bool
class Lunar(C.Structure): _fields_ = [('year', i16), ('month', i8), ('leap', bl), ('day', i8), ('month_days', i8)]
class Bazi(C.Structure): _fields_ = [('y', i8), ('m', i8), ('d', i8), ('h', i8)]
class Bone(C.Structure): _fields_ = [('wy', u8), ('wm', u8), ('wd', u8), ('wh', u8), ('total', u8)]
class Rng(C.Structure): _fields_ = [('a', u32)]
class YaoItem(C.Structure): _fields_ = [('kind', i8), ('hex', i8), ('pos', i8)]
class YaoRead(C.Structure): _fields_ = [('ben', i8), ('bian', i8), ('moving', i8), ('n', i8), ('main', i8), ('it', YaoItem * 2)]
class Ticket(C.Structure): _fields_ = [('sign', i8), ('sign_stars', i8), ('tarot', i8), ('tarot_rev', i8), ('lot_level', i8), ('hex_up', i8), ('hex_lo', i8),
                                       ('lot_no', i16), ('yi', i8 * 3), ('ji', i8 * 2), ('serial', i16)]
class Qimen(C.Structure): _fields_ = [('jieqi', i8), ('yang', bl), ('yuan', i8), ('ju', i8), ('day_gz', i8), ('hour_gz', i8), ('earth', i8 * 10), ('heaven', i8 * 10),
                                      ('extra', i8), ('star', i8 * 10), ('door', i8 * 10), ('god', i8 * 10), ('zhifu_src', i8), ('zhifu_at', i8), ('zhishi_at', i8),
                                      ('ring_shift', i8), ('door_shift', i8)]
class QRead(C.Structure): _fields_ = [('palace', i8), ('score', i8), ('verdict', i8)]
class Ziwei(C.Structure): _fields_ = [('soul', i8), ('body', i8), ('five', i8), ('ys', i8), ('yb', i8), ('stem', i8 * 12), ('name', i8 * 12), ('major', i8 * 14),
                                      ('minor', i8 * 14), ('bright', i8 * 14), ('mmut', i8 * 14), ('nmut', i8 * 14), ('soul_star', i8), ('body_star', i8)]


def build(src_dir, out):
    cc = os.environ.get('CC', 'gcc')
    subprocess.check_call([cc, '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', '-o', out] + [os.path.join(src_dir, f) for f in FILES])
    return C.CDLL(out)


def load(name):
    with open(os.path.join(GOLD, name), encoding='utf-8') as f:
        return json.load(f)


def run(lib, verbose=True):
    fails = {}

    def bad(cat, msg):
        fails.setdefault(cat, []).append(msg)

    # ---- 历法 ----
    for r in load('calendar.json'):
        y, m, d, h, mi = r['in']; l = Lunar()
        if not lib.fc_solar_to_lunar(y, m, d, C.byref(l)):
            bad('lunar', (r['in'], 'fail')); continue
        if (l.year, l.month, l.leap, l.day, l.month_days) != (r['ly'], r['lm'], r['leap'], r['ld'], r['mdays']):
            bad('lunar', (r['in'], (l.year, l.month, l.leap, l.day, l.month_days), r))
        if lib.fc_lunar_year_gz(l.year) != r['ygz']: bad('year_gz', r['in'])
        if lib.fc_day_gz(y, m, d) != r['dgz']: bad('day_gz', r['in'])
        if lib.fc_hour_gz(y, m, d, h) != r['hgz']: bad('hour_gz', r['in'])
        if lib.fc_prev_jieqi(y, m, d, h, mi) != r['jq']: bad('jieqi', (r['in'], lib.fc_prev_jieqi(y, m, d, h, mi), r['jq']))
        b = Bazi(); lib.fc_bazi(y, m, d, h, mi, C.byref(b))
        if [b.y, b.m, b.d, b.h] != r['bazi']: bad('bazi', (r['in'], [b.y, b.m, b.d, b.h], r['bazi']))
    # ---- 称骨 ----
    for r in load('bone.json'):
        y, m, d, hb = r['in']; l = Lunar(); lib.fc_solar_to_lunar(y, m, d, C.byref(l)); o = Bone()
        lib.fc_bone(C.byref(l), hb, C.byref(o))
        if [o.wy, o.wm, o.wd, o.wh] != r['w'] or o.total != r['total']: bad('bone', (r['in'], [o.wy, o.wm, o.wd, o.wh, o.total], r))
        if lib.fc_bone_band(o.total) != r['band']: bad('bone_band', r['in'])
    # ---- 六爻 ----
    g = load('liuyao.json')
    ben, bian, mv = C.c_int(), C.c_int(), C.c_bool()
    for r in g['ly']:
        lib.fc_liuyao((u8 * 6)(*r['v']), C.byref(ben), C.byref(bian), C.byref(mv))
        if (ben.value, bian.value, mv.value) != (r['ben'], r['bian'], r['moving']): bad('liuyao', r)
        rd = YaoRead(); lib.fc_liuyao_read((u8 * 6)(*r['v']), C.byref(rd)); e = r['read']
        got = (rd.moving, rd.main, [[rd.it[j].kind, rd.it[j].hex, rd.it[j].pos] for j in range(rd.n)])
        if got != (e['n'], e['main'], e['items']): bad('liuyao_read', (r['v'], got, e))
    for r in g['coins']:
        if lib.fc_coin_line((u8 * 3)(*r['b'])) != r['v']: bad('coin', r)
    # ---- 随机 ----
    g = load('random.json')
    lib.fc_rng_u32.restype = u32; lib.fc_daily_seed.restype = u32
    for r in g['rngs']:
        st = Rng(r['seed']); u = [lib.fc_rng_u32(C.byref(st)) for _ in range(16)]
        st = Rng(r['seed']); n = [lib.fc_rng_int(C.byref(st), 1 + k * 7) for k in range(16)]
        if u != r['u'] or n != r['n']: bad('rng', r['seed'])
    for r in g['tickets']:
        if lib.fc_daily_seed(*r['in']) != r['seed']: bad('daily_seed', r['in']); continue
        t = Ticket(); lib.fc_daily_ticket(u32(r['seed']), r['sign'], C.byref(t)); e = r['t']
        got = [t.sign, t.sign_stars, t.tarot, bool(t.tarot_rev), t.lot_no, t.lot_level, t.hex_up, t.hex_lo, list(t.yi), list(t.ji), t.serial]
        exp = [e['sign'], e['signStars'], e['tarot'], e['tarotRev'], e['lotNo'], e['lotLevel'], e['hexUp'], e['hexLo'], e['yi'], e['ji'], e['serial']]
        if got != exp: bad('ticket', (r['in'], got, exp))
    for n, lv in enumerate(g['lots'], 1):
        if lib.fc_lot_level(n) != lv: bad('lot_level', n)
    for r in g['hands']:
        st = Rng(r['seed']); h = (i8 * 7)(); rv = C.c_bool(); lib.fc_tarot_hand(C.byref(st), h, C.byref(rv))
        if list(h) != r['hand'] or rv.value != r['rev']: bad('tarot', r['seed'])
    # ---- 奇门 ----
    for r in load('qimen.json'):
        q = Qimen(); lib.fc_qimen(*r['in'], C.byref(q))
        head = (q.jieqi, q.yang, q.yuan, q.ju, q.day_gz, q.hour_gz, q.zhifu_at, q.zhishi_at)
        exp = (r['jq'], r['yang'], r['yuan'], r['ju'], r['day'], r['hour'], r['zhifuAt'], r['zhishiAt'])
        if head != exp: bad('qimen_head', (r['in'], head, exp)); continue
        for p in range(1, 10):
            if q.earth[p] != r['earth'][p]: bad('qimen_earth', r['in']); break
            if p == 5: continue
            if (q.heaven[p], q.star[p], q.door[p], q.god[p]) != (r['heaven'][p], r['star'][p], r['door'][p], r['god'][p]):
                bad('qimen_pan', (r['in'], p)); break
        if q.extra != r['extra']: bad('qimen_extra', r['in'])
        for a in range(5):
            o = QRead(); lib.fc_qimen_read(C.byref(q), a, C.byref(o))
            if [o.palace, o.score, o.verdict] != r['reads'][a]: bad('qimen_read', (r['in'], a)); break
    # ---- 紫微 ----
    for r in load('ziwei.json'):
        z = Ziwei()
        if not lib.fc_ziwei(*r['in'], C.byref(z)): bad('ziwei', (r['in'], 'fail')); continue
        for key, got in [('soul', z.soul), ('body', z.body), ('five', z.five), ('stem', list(z.stem)), ('name', list(z.name)), ('major', list(z.major)),
                         ('bright', list(z.bright)), ('mmut', list(z.mmut)), ('minor', list(z.minor)), ('nmut', list(z.nmut)),
                         ('soulStar', z.soul_star), ('bodyStar', z.body_star)]:
            if got != r[key]: bad('ziwei_' + key, (r['in'], got, r[key])); break
    if verbose:
        for k, v in fails.items():
            print(f'  ✗ {k}: {len(v)} 条不一致，例如 {v[:2]}')
    return fails


def main():
    tmp = tempfile.mkdtemp()
    try:
        if '--mutate' not in sys.argv:
            fails = run(build(SRC, os.path.join(tmp, 'f.so')))
            total = sum(len(v) for v in fails.values())
            print('全部一致' if not total else f'不一致共 {total} 条')
            sys.exit(1 if total else 0)
        # 变异测试：每一条都应该让对拍失败
        MUT = [
            ('fortune_misc.c', 'BONE_M[12] = {6,7,18,', 'BONE_M[12] = {16,9,15,', '称骨月表换回旧错表'),
            ('fortune_misc.c', 'BONE_H[12] = {16,6,', 'BONE_H[12] = {16,7,', '称骨时表改一钱'),
            ('fortune_calendar.c', '+ 10, 60); }', '+ 11, 60); }', '日干支偏一天'),
            ('fortune_calendar.c', 'if (hour == 23) dg = (dg + 1) % 60;', '', '23 点时干不进位'),
            ('fortune_calendar.c', 'if (k == ly->leap) { month = ly->leap; leap = true; }', 'if (k == ly->leap) { month = ly->leap; }', '闰月标记丢失'),
            ('fortune_calendar.c', 'if (row[k] <= t)', 'if (row[k] < t)', '节气交接边界'),
            ('fortune_qimen.c', 'return jq <= 10 || jq == 23;', 'return jq <= 11 || jq == 23;', '夏至误判阳遁'),
            ('fortune_qimen.c', '(fz == 2 || fz == 8 || fz == 5 || fz == 11) ? 1 : 2', '(fz == 2 || fz == 8 || fz == 5 || fz == 11) ? 2 : 1', '中下元对调'),
            ('fortune_qimen.c', 'q->yang ? gi + k : gi - k', 'gi + k', '阴遁八神不逆排'),
            ('fortune_qimen.c', 'static const int8_t GOD_SC[8] = {2,0,2,2,0,0,1,2};', 'static const int8_t GOD_SC[8] = {2,0,2,2,0,0,2,2};', '九地分值'),
            ('fortune_ziwei.c', 'zi = (off % 2 == 0) ? zi + off : zi - off;', 'zi = (off % 2 == 0) ? zi - off : zi + off;', '紫微奇偶方向'),
            ('fortune_ziwei.c', '((l.leap && l.day > 15) ? 1 : 0)', '0', '闰月后半月不进位'),
            ('fortune_ziwei.c', 'MA[4] = {0, 9, 6, 3}', 'MA[4] = {0, 9, 3, 6}', '天马'),
            ('fortune_ziwei.c', '{ZW_TIANLIANG, ZW_ZIWEI, 100 + ZM_ZUOFU, ZW_WUQU}', '{ZW_TIANLIANG, ZW_ZIWEI, 100 + ZM_YOUBI, ZW_WUQU}', '壬干化科'),
            ('fortune_misc.c', 'o->tarot_rev = fc_rng_int(&r, 10) < 3;', 'o->tarot_rev = fc_rng_int(&r, 10) < 4;', '小票逆位概率'),
            ('fortune_misc.c', 'c[i] = v[i] == 6 ? 1 : v[i] == 9 ? 0 : y[i];', 'c[i] = y[i];', '变卦不变'),
            ('fortune_misc.c', 'LOT_LEVEL[FC_LOT_COUNT] = {0,2,2,', 'LOT_LEVEL[FC_LOT_COUNT] = {0,1,2,', '第二签签级'),
            ('fortune_misc.c', 'SET(0, 1, ben, mv[1]); SET(1, 1, ben, mv[0]);', 'SET(0, 1, ben, mv[0]); SET(1, 1, ben, mv[1]);', '二爻动以上爻为主'),
            ('fortune_misc.c', 'if (ben == 0 || ben == 63) SET(0, 2, ben, -1);', 'if (ben == 0) SET(0, 2, ben, -1);', '坤卦用六'),
        ]
        caught = 0
        for i, (fn, old, new, label) in enumerate(MUT):
            d = os.path.join(tmp, f'm{i}'); shutil.copytree(SRC, d)
            p = os.path.join(d, fn); s = open(p, encoding='utf-8').read()
            assert s.count(old) == 1, (fn, old)
            open(p, 'w', encoding='utf-8').write(s.replace(old, new))
            f = run(build(d, os.path.join(d, 'f.so')), verbose=False)
            ok = bool(f); caught += ok
            print(f"  {'抓到' if ok else '漏掉'} · {label} → {', '.join(sorted(f)) if f else '无失败'}")
        print(f'变异测试：{caught}/{len(MUT)} 被抓到')
        sys.exit(0 if caught == len(MUT) else 1)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == '__main__':
    main()
