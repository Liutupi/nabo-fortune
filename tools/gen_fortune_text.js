// 从原型 prototype/index.html 抽出界面和解读文案，生成 firmware/main/fortune/fortune_text.c
// 用法：node tools/gen_fortune_text.js prototype/index.html firmware/main/fortune/fortune_text.c
const fs = require('fs');
const [src, out] = process.argv.slice(2);
const html = fs.readFileSync(src, 'utf8');
const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]);
const core = scripts.find(s => s.includes('root.FortuneCore='));
const ui = scripts.find(s => s.includes('const TAROT='));
const g = {};
new Function('root', core.replace('root.FortuneCore=', 'root.__X={BONE_POEM,BONE_BAND,ZW,ZW_FIELD,ZW_BRIGHT,ZW_MUT,QM_DOOR,QM_STAR,QM_GOD,QM_ASK,YI,JI,DIR,GUA};root.FortuneCore=').replace(/\}\)\(typeof window[^;]*;\s*$/, '})(root);').replace(/^\s*\(function\(root\)\{/, '(function(root){'))(g);
const X = g.__X;
const grab = name => { const m = ui.match(new RegExp('const ' + name + '=(\\[[\\s\\S]*?\\]);\\r?\\n')); if (!m) throw new Error('Missing prototype array: ' + name); return eval(m[1]); };
const TAROT = grab('TAROT'), SIGNS = grab('SIGNS'), HEX = grab('HEX');
const q = s => JSON.stringify(s);
const arr = (type, name, a) => `const char *const ${name}[${a.length}] = {${a.map(q).join(', ')}};\n`;
let c = `/* 由 tools/gen_fortune_text.js 从 prototype/index.html 生成，请勿手改 */\n#include "fortune_text.h"\n\n`;
// 称骨
const poems = []; for (let i = 21; i <= 72; i++) poems.push(X.BONE_POEM[(i / 10).toFixed(1)] || '');
c += arr('', 'FT_BONE_POEM', poems);
c += `const uint8_t FT_BONE_BAND_LO[9] = {${X.BONE_BAND.map(b => Math.round(b[0] * 10)).join(',')}};\n`;
c += `const uint8_t FT_BONE_BAND_HI[9] = {${X.BONE_BAND.map(b => Math.round(b[1] * 10)).join(',')}};\n`;
c += arr('', 'FT_BONE_TYPE', X.BONE_BAND.map(b => b[2])) + arr('', 'FT_BONE_TEXT', X.BONE_BAND.map(b => b[3]));
// 塔罗、星座、卦名、宜忌
c += arr('', 'FT_TAROT_NUM', TAROT.map(t => t[0])) + arr('', 'FT_TAROT_NAME', TAROT.map(t => t[1])) + arr('', 'FT_TAROT_KEY', TAROT.map(t => t[2]));
c += arr('', 'FT_SIGN_NAME', SIGNS.map(s => s[0])) + arr('', 'FT_SIGN_DATE', SIGNS.map(s => s[1]));
c += arr('', 'FT_HEX_NAME', HEX.flat());
c += arr('', 'FT_YI', X.YI) + arr('', 'FT_JI', X.JI);
// 紫微
const MAJ = ['紫微', '天机', '太阳', '武曲', '天同', '廉贞', '天府', '太阴', '贪狼', '巨门', '天相', '天梁', '七杀', '破军'];
c += arr('', 'FT_ZW_TEXT', MAJ.flatMap(n => X.ZW[n]));
const PAL = ['命宫', '父母', '福德', '田宅', '官禄', '仆役', '迁移', '疾厄', '财帛', '子女', '夫妻', '兄弟'];
c += `const int8_t FT_ZW_FIELD_IDX[12] = {${PAL.map(p => (X.ZW_FIELD[p] || [0])[0]).join(',')}};\n`;
c += arr('', 'FT_ZW_FIELD_NAME', PAL.map(p => (X.ZW_FIELD[p] || [0, '整体'])[1]));
c += arr('', 'FT_ZW_BRIGHT', ['', ...'庙旺得利平不陷'.split('').map(k => X.ZW_BRIGHT[k])]);
c += arr('', 'FT_ZW_MUT', '禄权科忌'.split('').map(k => X.ZW_MUT[k]));
// 奇门：门、星按原宫号（0 与 5 空），神按 0..7
const DOOR0 = {1: '休门', 8: '生门', 3: '伤门', 4: '杜门', 9: '景门', 2: '死门', 7: '惊门', 6: '开门'};
const STAR0 = {1: '天蓬', 8: '天任', 3: '天冲', 4: '天辅', 9: '天英', 2: '天芮', 7: '天柱', 6: '天心', 5: '天禽'};
const GODS = ['值符', '螣蛇', '太阴', '六合', '白虎', '玄武', '九地', '九天'];
const rng10 = f => [...Array(10).keys()].map(f);
c += arr('', 'FT_QM_DOOR_TEXT', rng10(i => DOOR0[i] ? X.QM_DOOR[DOOR0[i]][1] : ''));
c += arr('', 'FT_QM_STAR_TEXT', rng10(i => STAR0[i] ? X.QM_STAR[STAR0[i]][1] : ''));
c += arr('', 'FT_QM_GOD_TEXT', GODS.map(n => X.QM_GOD[n][1]));
c += arr('', 'FT_QM_ASK', X.QM_ASK.map(a => a[0])) + arr('', 'FT_QM_KEY', X.QM_ASK.map(a => a[1]));
c += arr('', 'FT_QM_DIR', rng10(i => X.DIR[i] || ''));
fs.writeFileSync(out, c);
console.log('ok', out, c.length);
