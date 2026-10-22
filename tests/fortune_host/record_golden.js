// 录制对拍标准答案：node tests/fortune_host/record_golden.js
// 依赖（只在开发机上）：lunar-javascript@1.7.7、iztro@2.6.1、kinqimen，以及 prototype/fortune-core.js
const fs = require('fs'), path = require('path');
const ROOT = path.resolve(__dirname, '../..');
const { Solar, LunarMonth } = require('lunar-javascript');
const { astro } = require('iztro');
require(path.join(ROOT, 'prototype/fortune-core.js'));
const F = globalThis.FortuneCore;
const OUT = path.join(__dirname, 'golden');
fs.mkdirSync(OUT, { recursive: true });
const GAN = F.GAN, ZHI = F.ZHI, gz = F.gzIdx;
const JQ = ['小寒','大寒','立春','雨水','惊蛰','春分','清明','谷雨','立夏','小满','芒种','夏至','小暑','大暑','立秋','处暑','白露','秋分','寒露','霜降','立冬','小雪','大雪','冬至'];
let s = 20261022; const rnd = () => { s = (Math.imul(s, 1103515245) + 12345) >>> 0; return s / 4294967296; };
const ri = (a, b) => a + Math.floor(rnd() * (b - a + 1));
const dim = (y, m) => new Date(y, m, 0).getDate();
const randDT = (y0, y1) => { const y = ri(y0, y1), m = ri(1, 12); return [y, m, ri(1, dim(y, m)), ri(0, 23), ri(0, 59)]; };

/* ---------- 历法 ---------- */
const cal = [];
const pushCal = (y, m, d, h, mi) => {
  const l = Solar.fromYmdHms(y, m, d, h, mi, 0).getLunar(), e = l.getEightChar();
  cal.push({ in: [y, m, d, h, mi], ly: l.getYear(), lm: Math.abs(l.getMonth()), leap: l.getMonth() < 0, ld: l.getDay(),
    mdays: LunarMonth.fromYm(l.getYear(), l.getMonth()).getDayCount(), ygz: gz(l.getYearInGanZhi()), dgz: gz(l.getDayInGanZhi()),
    hgz: gz(l.getTimeInGanZhi()), jq: JQ.indexOf(l.getPrevJieQi(false).getName()),
    bazi: [gz(e.getYear()), gz(e.getMonth()), gz(e.getDay()), gz(e.getTime())] });
};
for (let i = 0; i < 6000; i++) pushCal(...randDT(1901, 2100));
for (const yy of [1985, 2023, 2026, 2033]) for (let m = 1; m <= 12; m++) for (let d = 1; d <= dim(yy, m); d++) pushCal(yy, m, d, 12, 0);
// 节气交接前后一分钟
for (let i = 0; i < 400; i++) {
  const y = ri(1902, 2099), t = Solar.fromYmd(y, ri(1, 12), 15).getLunar().getPrevJieQi(false).getSolar();
  const base = new Date(Date.UTC(t.getYear(), t.getMonth() - 1, t.getDay(), t.getHour(), t.getMinute()));
  for (const dm of [-1, 0, 1, 2]) { const x = new Date(base.getTime() + dm * 60000); pushCal(x.getUTCFullYear(), x.getUTCMonth() + 1, x.getUTCDate(), x.getUTCHours(), x.getUTCMinutes()); }
}
fs.writeFileSync(path.join(OUT, 'calendar.json'), JSON.stringify(cal));

/* ---------- 称骨 ---------- */
const bone = [];
for (let i = 0; i < 3000; i++) {
  const y = ri(1901, 2099), m = ri(1, 12), d = ri(1, dim(y, m)), hb = ri(0, 11);
  const l = Solar.fromYmdHms(y, m, d, hb === 0 ? 0 : hb * 2, 30, 0).getLunar(), b = F.bone(l);
  bone.push({ in: [y, m, d, hb], w: b.parts.map(p => Math.round(p[2] * 10)), total: Math.round(b.total * 10), band: F.BONE_BAND.findIndex(x => x[2] === b.type) });
}
fs.writeFileSync(path.join(OUT, 'bone.json'), JSON.stringify(bone));

/* ---------- 六爻 ---------- */
const ly = [];
for (let c = 0; c < 4096; c++) { const v = [0,1,2,3,4,5].map(i => 6 + ((c >> (2 * i)) & 3)); const r = F.liuyao(v), rd = F.liuyaoRead(v); ly.push({ v, ben: r.ben, bian: r.bian, moving: r.moving, read: { n: rd.moving, items: rd.items.map(x => [{ gua: 0, line: 1, special: 2 }[x[0]], x[1], x.length > 2 ? x[2] : -1]), main: rd.main } }); }
const coins = []; for (let c = 0; c < 8; c++) { const b = [c & 1, (c >> 1) & 1, (c >> 2) & 1]; coins.push({ b, v: F.coinLine(b) }); }
fs.writeFileSync(path.join(OUT, 'liuyao.json'), JSON.stringify({ ly, coins }));

/* ---------- 随机 / 小票 / 塔罗 ---------- */
const rngs = []; for (let i = 0; i < 64; i++) { const seed = ri(0, 0xffffffff) >>> 0, r = F.rng(seed), u = [], n = []; for (let k = 0; k < 16; k++) u.push(r.u32()); const r2 = F.rng(seed); for (let k = 0; k < 16; k++) n.push(r2.int(1 + k * 7)); rngs.push({ seed, u, n }); }
const tickets = []; for (let i = 0; i < 800; i++) { const [y, m, d] = randDT(2026, 2040), py = ri(1930, 2025), pm = ri(1, 12), pd = ri(1, 28), ph = ri(0, 11), pg = ri(0, 1);
  const seed = F.dailySeed(y, m, d, py, pm, pd, ph, pg); tickets.push({ in: [y, m, d, py, pm, pd, ph, pg], seed, sign: ri(0, 11) }); tickets[i].t = F.dailyTicket(seed, tickets[i].sign); }
const hands = []; for (let i = 0; i < 300; i++) { const seed = ri(0, 0xffffffff) >>> 0; hands.push({ seed, ...F.tarotHand(F.rng(seed)) }); }
const lots = []; for (let n = 1; n <= 100; n++) lots.push(F.lotLevel(n));
fs.writeFileSync(path.join(OUT, 'random.json'), JSON.stringify({ rngs, tickets, hands, lots }));

/* ---------- 奇门 ---------- */
const STAR_NO = { 天蓬: 1, 天芮: 2, 天冲: 3, 天辅: 4, 天禽: 5, 天心: 6, 天柱: 7, 天任: 8, 天英: 9 };
const DOOR_NO = { 休门: 1, 死门: 2, 伤门: 3, 杜门: 4, 开门: 6, 惊门: 7, 生门: 8, 景门: 9 };
const GODS = ['值符','螣蛇','太阴','六合','白虎','玄武','九地','九天'];
const qm = [];
const qmIn = []; for (let i = 0; i < 4000; i++) qmIn.push(randDT(1902, 2099));
for (let h = 0; h < 24; h++) qmIn.push([2026, 10, 22, h, 30]);
for (const [y, m, d, h, mi] of qmIn) {
  const q = F.qimen(Solar.fromYmdHms(y, m, d, h, mi, 0).getLunar());
  const rec = { in: [y, m, d, h, mi], jq: JQ.indexOf(q.jq), yang: q.yang, yuan: '上中下'.indexOf(q.yuan[0]), ju: q.ju, day: gz(q.dayGZ), hour: gz(q.shiGZ),
    earth: [], heaven: [], star: [], door: [], god: [], extra: -1, zhifuAt: q.zhifuAt, zhishiAt: q.zhishiAt, reads: [] };
  for (let p = 1; p <= 9; p++) { rec.earth[p] = GAN.indexOf(q.earth[p]); if (p !== 5) { rec.heaven[p] = GAN.indexOf(q.heaven[p][0]); if (q.heaven[p].length > 1) rec.extra = GAN.indexOf(q.heaven[p][1]); rec.star[p] = STAR_NO[q.star[p]]; rec.door[p] = DOOR_NO[q.door[p]]; rec.god[p] = GODS.indexOf(q.god[p]); } }
  for (let a = 0; a < 5; a++) { const r = F.qimenRead(q, a); rec.reads.push([r.palace, r.score, r.vc]); }
  qm.push(rec);
}
fs.writeFileSync(path.join(OUT, 'qimen.json'), JSON.stringify(qm));

/* ---------- 紫微（iztro） ---------- */
const MAJ = ['紫微','天机','太阳','武曲','天同','廉贞','天府','太阴','贪狼','巨门','天相','天梁','七杀','破军'];
const MIN = ['左辅','右弼','文昌','文曲','天魁','天钺','禄存','天马','地空','地劫','火星','铃星','擎羊','陀罗'];
const PAL = ['命宫','父母','福德','田宅','官禄','仆役','迁移','疾厄','财帛','子女','夫妻','兄弟'];
const BR = ['', '庙', '旺', '得', '利', '平', '不', '陷'], MU = ['禄', '权', '科', '忌'];
const FIVE = { 水二局: 2, 木三局: 3, 金四局: 4, 土五局: 5, 火六局: 6 };
const starCode = n => MAJ.includes(n) ? MAJ.indexOf(n) : 100 + MIN.indexOf(n);
const zw = [];
for (let i = 0; i < 3000; i++) {
  const y = ri(1930, 2025), m = ri(1, 12), d = ri(1, dim(y, m)), t = ri(0, 11), g = rnd() < .5 ? '男' : '女';
  const a = astro.bySolar(`${y}-${m}-${d}`, t, g, true, 'zh-CN');
  const rec = { in: [y, m, d, t], soul: a.palaces.findIndex(p => p.name === '命宫'), body: a.palaces.findIndex(p => p.isBodyPalace), five: FIVE[a.fiveElementsClass],
    stem: a.palaces.map(p => GAN.indexOf(p.heavenlyStem)), name: a.palaces.map(p => PAL.indexOf(p.name)),
    major: Array(14).fill(-1), bright: Array(14).fill(0), mmut: Array(14).fill(-1), minor: Array(14).fill(-1), nmut: Array(14).fill(-1),
    soulStar: starCode(a.soul), bodyStar: starCode(a.body) };
  a.palaces.forEach((p, pi) => {
    p.majorStars.forEach(s => { const k = MAJ.indexOf(s.name); if (k < 0) return; rec.major[k] = pi; rec.bright[k] = Math.max(0, BR.indexOf(s.brightness || '')); rec.mmut[k] = s.mutagen ? MU.indexOf(s.mutagen) : -1; });
    p.minorStars.forEach(s => { const k = MIN.indexOf(s.name); if (k < 0) return; rec.minor[k] = pi; rec.nmut[k] = s.mutagen ? MU.indexOf(s.mutagen) : -1; });
  });
  zw.push(rec);
}
fs.writeFileSync(path.join(OUT, 'ziwei.json'), JSON.stringify(zw));

/* ---------- 原型核心 vs kinqimen（第二标准） ---------- */
(async () => {
let kinBad = 0, kinN = 0, kin = null;
try { kin = await import('kinqimen'); } catch (e) {}
if (kin) {
  const G = { 坎: 1, 坤: 2, 震: 3, 巽: 4, 中: 5, 乾: 6, 兑: 7, 艮: 8, 离: 9 };
  for (const r of qm) { const [y, m, d, h, mi] = r.in; const p = new kin.Qimen({ year: y, month: m, day: d, hour: h, minute: mi }).pan(); kinN++;
    const ju = (r.yang ? '阳' : '阴') + '一二三四五六七八九'[r.ju - 1] + '局' + '上中下'[r.yuan];
    let ok = p.ju === ju;
    for (const [k, v] of Object.entries(p.doorPan)) if (r.door[G[k]] !== DOOR_NO[v + '门']) ok = false;
    for (const [k, v] of Object.entries(p.earthPan)) if (r.earth[G[k]] !== GAN.indexOf(v)) ok = false;
    if (!ok) kinBad++; }
}
console.log(JSON.stringify({ calendar: cal.length, bone: bone.length, liuyao: ly.length, tickets: tickets.length, qimen: qm.length, ziwei: zw.length, kinqimen_checked: kinN, kinqimen_mismatch: kinBad }));
})();
