/* NABO 赛博算命机 · 规则核心（原型 = 标准答案；固件 C 版逐项对拍）
   依赖：lunar-javascript（Solar/Lunar）提供农历、节气、干支。 */
(function(root){
const Solar=root.Solar||(typeof require!=='undefined'?require('lunar-javascript').Solar:undefined);
const GAN='甲乙丙丁戊己庚辛壬癸',ZHI='子丑寅卯辰巳午未申酉戌亥';
const gzIdx=s=>{for(let i=0;i<60;i++)if(GAN[i%10]===s[0]&&ZHI[i%12]===s[1])return i;return -1};

/* ---------- 称骨 ---------- */
// 年：按六十甲子顺序（甲子起）。三源校对：两源一致；udn 版己丑 0.8、癸亥 0.7 为异文，取多数（0.7 / 0.6）
// 日、时：三源一致
const BONE_Y=[1.2,0.9,0.6,0.7,1.2,0.5,0.9,0.8,0.7,0.8,1.5,0.9,1.6,0.8,0.8,1.9,1.2,0.6,0.8,0.7,0.5,1.5,0.6,1.6,1.5,0.7,0.9,1.2,1.0,0.7,1.5,0.6,0.5,1.4,1.4,0.9,0.7,0.7,0.9,1.2,0.8,0.7,1.3,0.5,1.4,0.5,0.9,1.7,0.5,0.7,1.2,0.8,0.8,0.6,1.9,0.6,0.8,1.6,1.0,0.6];
// 月：正月…腊月（2026-10-22 校对：ba-zi chinese-astrology-skill、udn giveman、CMoney 三源一致）
const BONE_M=[0.6,0.7,1.8,0.9,0.5,1.6,0.9,1.5,1.8,0.8,0.9,0.5];
const BONE_D=[0.5,1.0,0.8,1.5,1.6,1.5,0.8,1.6,0.8,1.6,0.9,1.7,0.8,1.7,1.0,0.8,0.9,1.8,0.5,1.5,1.0,0.9,0.8,0.9,1.5,1.8,0.7,0.8,1.6,0.6];
const BONE_H=[1.6,0.6,0.7,1.0,0.9,1.6,1.0,0.8,0.8,0.9,0.6,0.6];
const BONE_BAND=[
 [2.1,2.9,'先苦后甜型','早年要多靠自己打拼，正因为起点低，每一步都走得扎实。贵在坚持，中年以后路越走越宽。'],
 [3.0,3.5,'踏实耕耘型','做事认真、肯下功夫，适合在一个领域里慢慢深耕。身边会遇到愿意拉你一把的人。'],
 [3.6,3.9,'稳中求进型','性格稳重，不爱冒险，但关键时刻判断准。财来得不快却守得住，晚运比早运好。'],
 [4.0,4.5,'独立担当型','能扛事，常常是团队里拿主意的那个人。只要不把所有事都揽在自己身上，福气自然来。'],
 [4.6,4.9,'贵人相助型','人缘好，做事容易得到前辈和朋友的支持。多和人合作，机会比单打独斗多。'],
 [5.0,5.5,'聪明进取型','头脑灵活、学东西快，有开拓新局面的能力。注意别分心太多，专注一件事成就更大。'],
 [5.6,5.9,'福禄双全型','一生衣食不愁，家庭和事业都有不错的根基。懂得分享，福气会越分越多。'],
 [6.0,6.5,'大展宏图型','格局大、有领导力，适合做需要担当的事业。保持谦逊，就能走得长远。'],
 [6.6,7.2,'富贵天成型','骨重在传统说法里属于上格。天赋和机遇都不缺，最大的功课是珍惜和回馈。']];
// 称骨歌 52 首（2.1–7.2 两），古本公版原文；来源 chinese-astrology-skill (MIT) 整理本，异文待通读
const BONE_POEM={"2.1": "短命非业谓大空，平生灾难事重重，凶祸频临陷逆境，终世困苦事不成。", "2.2": "身寒骨冷苦伶仃，此命推来行乞人，劳劳碌碌无度日，终年打拱过平生。", "2.3": "此命推来骨格轻，求谋作事事难成，妻儿兄弟应难许，别处他乡作散人。", "2.4": "此命推来福禄无，门庭困苦总难荣，六亲骨肉皆无靠，流浪他乡作老翁。", "2.5": "此命推来祖业微，门庭营度似稀奇，六亲骨肉如冰炭，一世勤劳自把持。", "2.6": "平生衣禄苦中求，独自营谋事不休，离祖出门宜早计，晚来衣禄自无忧。", "2.7": "一生作事少商量，难靠祖宗作主张，独马单枪空做去，早年晚岁总无长。", "2.8": "一生行事似飘蓬，祖宗产业在梦中，若不过房改名姓，也当移徒二三通。", "2.9": "初年运限未曾亨，纵有功名在后成，须过四旬方可立，移居改姓始为良。", "3.0": "劳劳碌碌苦中求，东奔西走何日休，若使终身勤与俭，老来稍可免忧愁。", "3.1": "忙忙碌碌苦中求，何日云开见日头，难得祖基家可立，中年衣食渐无忧。", "3.2": "初年运蹇事难谋，渐有财源如水流，到得中年衣食旺，那时名利一齐收。", "3.3": "早年做事事难成，百计徒劳枉费心，半世自如流水去，后来运到得黄金。", "3.4": "此命福气果如何，僧道门中衣禄多，离祖出家方为妙，朝晚拜佛念弥陀。", "3.5": "生平福量不周全，祖业根基觉少传，营事生涯宜守旧，时来衣食胜从前。", "3.6": "不须劳碌过平生，独自成家福不轻，早有福星常照命，任君行去百般成。", "3.7": "此命般般事不成，弟兄少力自孤成，虽然祖业须微有，来得明时去不明。", "3.8": "一身骨肉最清高，早入簧门姓氏标，待到年将三十六，蓝衣脱去换红袍。", "3.9": "此命终身运不通，劳劳做事尽皆空，苦心竭力成家计，到得那时在梦中。", "4.0": "平生衣禄是绵长，件件心中自主张，前面风霜多受过，后来必定享安康。", "4.1": "此命推来自不同，为人能干异凡庸，中年还有逍遥福，不比前时运未通。", "4.2": "得宽怀处且宽怀，何用双眉总不开，若使中年命运济，那时名利一齐来。", "4.3": "为人心性最聪明，做事轩昂近贵人，衣禄一生天数定，不须劳碌过平生。", "4.4": "万事由天莫苦求，须知福碌赖人修，当年财帛难如意，晚景欣然便不忧。", "4.5": "名利推来竟若何，前番辛苦后奔波，命中难养男和女，骨肉扶持也不多。", "4.6": "东西南北尽皆通，出姓移居更觉隆，衣禄无亏天数定，中年晚景一般同。", "4.7": "此命推来旺末年，妻荣子贵自怡然，平生原有滔滔福，可卜财源若水泉。", "4.8": "初年运道未曾亨，若是蹉跎再不兴，兄弟六亲皆无靠，一身事业晚来整。", "4.9": "此命推来福不轻，自成自立显门庭，从来富贵人亲近，使婢差奴过一生。", "5.0": "为利为名终日劳，中年福禄也多遭，老来是有财星照，不比前番目下高。", "5.1": "一世荣华事事通，不须劳碌自亨通，弟兄叔侄皆如意，家业成时福禄宏。", "5.2": "一世亨通事事能，不须劳思自然宁，宗族欣然心皆好，家业丰亨自称心。", "5.3": "此格推来气象真，兴家发达在其中，一生福禄安排定，却是人间一富翁。", "5.4": "此命推来厚且清，诗书满腹看功成，丰衣足食自然稳，正是人间有福人。", "5.5": "走马扬鞭争利名，少年做事废功成，一朝福禄源源至，富贵荣华显六亲。", "5.6": "此格推来礼义通，一生福禄用无穷，甜酸苦辣皆尝过，财源滚滚稳且丰。", "5.7": "福禄盈盈万事全，一生荣耀显双亲，名扬威震人钦敬，处世逢凶化为吉。", "5.8": "平生福禄自然来，名利兼全福寿偕，雁塔题名为贵客，紫袍金带走金阶。", "5.9": "细推此格妙且清，必定才高礼义通，甲第之中应有分，扬鞭走马显威荣。", "6.0": "一朝金榜快题名，显祖荣宗立大功，衣食定然原裕足，田园财帛更丰盈。", "6.1": "不做朝中金榜客，定为世上大财翁，聪明天赋经书熟，名显高科自是荣。", "6.2": "此命生来福不穷，读书必定显亲宗，紫衣金带为卿相，富贵荣华皆可同。", "6.3": "命主为官福禄长，得来富贵定非常，名题金塔传金榜，定中高科天下扬。", "6.4": "此格权威不可当，紫袍金带坐高堂，荣华富贵谁能及，积玉堆金满储仓。", "6.5": "细推此命福非轻，富贵荣华孰与争，定国安邦人极品，威声显赫震寰瀛。", "6.6": "此格人间一福人，堆金积玉满堂春，从来富贵由天定，正笏垂绅谒圣君。", "6.7": "此命生来福自宏，田园家业最高隆，平生衣禄盈丰足，一世荣华万事通。", "6.8": "富贵由天莫苦求，万金家计不须谋，十年不比前番事，一但遂心百事谋。", "6.9": "君是人间衣禄星，一生富贵众人钦，纵然福禄由天定，安享荣华过一生。", "7.0": "此命推来福禄宏，不须劳碌自亨通，田园产业家昌盛，大富大贵不受穷。", "7.1": "此命生成大不同，公侯卿相在其中，一生自有逍遥福，富贵荣华极品隆。", "7.2": "此格世界罕有生，十代积善产此人，天上紫微来照命，统治万民乐太平。"};
function bone(lunar){
 const y=BONE_Y[gzIdx(lunar.getYearInGanZhi())],m=BONE_M[Math.abs(lunar.getMonth())-1],d=BONE_D[lunar.getDay()-1],h=BONE_H[ZHI.indexOf(lunar.getTimeZhi())];
 const total=Math.round((y+m+d+h)*10)/10;const band=BONE_BAND.find(b=>total>=b[0]-1e-9&&total<=b[1]+1e-9)||BONE_BAND[0];
 return{poem:BONE_POEM[total.toFixed(1)]||'',parts:[['年',lunar.getYearInGanZhi()+'年',y],['月',lunar.getMonthInChinese()+'月',m],['日',lunar.getDayInChinese(),d],['时',lunar.getTimeZhi()+'时',h]],total,type:band[2],text:band[3]};
}

/* ---------- 奇门遁甲 · 时家转盘 · 拆补法 ---------- */
const JU={冬至:[1,7,4],小寒:[2,8,5],大寒:[3,9,6],立春:[8,5,2],雨水:[9,6,3],惊蛰:[1,7,4],春分:[3,9,6],清明:[4,1,7],谷雨:[5,2,8],立夏:[4,1,7],小满:[5,2,8],芒种:[6,3,9],
 夏至:[9,3,6],小暑:[8,2,5],大暑:[7,1,4],立秋:[2,5,8],处暑:[1,4,7],白露:[9,3,6],秋分:[7,1,4],寒露:[6,9,3],霜降:[5,8,2],立冬:[6,9,3],小雪:[5,8,2],大雪:[4,7,1]};
const YANG='冬至小寒大寒立春雨水惊蛰春分清明谷雨立夏小满芒种';
const QIYI='戊己庚辛壬癸丁丙乙';
const RING=[1,8,3,4,9,2,7,6];// 外圈顺时针（坎艮震巽离坤兑乾）
const STAR0={1:'天蓬',8:'天任',3:'天冲',4:'天辅',9:'天英',2:'天芮',7:'天柱',6:'天心',5:'天禽'};
const DOOR0={1:'休门',8:'生门',3:'伤门',4:'杜门',9:'景门',2:'死门',7:'惊门',6:'开门'};
const GODS=['值符','螣蛇','太阴','六合','白虎','玄武','九地','九天'];
const GUA={1:'坎',2:'坤',3:'震',4:'巽',5:'中',6:'乾',7:'兑',8:'艮',9:'离'};
const DIR={1:'正北',2:'西南',3:'正东',4:'东南',5:'中央',6:'西北',7:'正西',8:'东北',9:'正南'};
const XUN_YI='戊己庚辛壬癸';// 甲子戊 甲戌己 甲申庚 甲午辛 甲辰壬 甲寅癸
const g5=p=>p===5?2:p;
function qimen(lunar){
 const jq=lunar.getPrevJieQi(false).getName();
 if(lunar.getHour()===23){const nx=lunar.getSolar().next(1);lunar=Solar.fromYmdHms(nx.getYear(),nx.getMonth(),nx.getDay(),0,lunar.getMinute(),0).getLunar()}
 const yang=YANG.includes(jq);
 const di=gzIdx(lunar.getDayInGanZhi()),fh=di-(di%10)%5,fz=fh%12;
 const yuan=[0,6,3,9].includes(fz)?0:[2,8,5,11].includes(fz)?1:2;
 const ju=JU[jq][yuan];
 const earth={},where={};
 for(let i=0;i<9;i++){const p=yang?((ju-1+i)%9)+1:((ju-1-i+18)%9)+1;earth[p]=QIYI[i];where[QIYI[i]]=p}
 const ti=gzIdx(lunar.getTimeInGanZhi()),xun=Math.floor(ti/10),xunYi=XUN_YI[xun],steps=ti%10;
 const srcReal=where[xunYi],src=g5(srcReal);
 const tg=GAN[ti%10]==='甲'?xunYi:GAN[ti%10];
 const dst=g5(where[tg]);
 const shift=(RING.indexOf(dst)-RING.indexOf(src)+8)%8;
 const star={},heaven={};
 RING.forEach((p,i)=>{const from=RING[(i-shift+8)%8];star[p]=STAR0[from];heaven[p]=earth[from]+(from===2?earth[5]:'')});
 const doorDst=g5(yang?((srcReal-1+steps)%9)+1:((srcReal-1-steps+90)%9)+1);
 const dshift=(RING.indexOf(doorDst)-RING.indexOf(src)+8)%8,door={};
 RING.forEach((p,i)=>door[p]=DOOR0[RING[(i-dshift+8)%8]]);
 const god={},gi=RING.indexOf(dst);
 GODS.forEach((g,k)=>god[RING[((yang?gi+k:gi-k)%8+8)%8]]=g);
 return{jq,yang,yuan:'上中下'[yuan]+'元',ju,earth,heaven,star,door,god,zhifu:STAR0[src],zhishi:DOOR0[src],zhifuAt:dst,zhishiAt:doorDst,shiGZ:lunar.getTimeInGanZhi(),dayGZ:lunar.getDayInGanZhi(),
  ringShift:shift,doorShift:dshift};
}
const QM_DOOR={休门:[2,'休养生息，宜求人、谈和、休整'],生门:[2,'生机之门，宜求财、开业、置业'],伤门:[0,'冲劲大但易起摩擦，宜运动、追讨，不宜谈判'],杜门:[1,'闭塞之象，宜保密、躲避、潜心做事'],景门:[1,'光明显扬，宜考试、宣传、展示'],死门:[0,'沉寂之门，宜收尾、整理，不宜开新事'],惊门:[0,'易有口舌惊扰，宜谨言慎行'],开门:[2,'开阔通达，宜求职、开工、见贵人']};
const QM_STAR={天蓬:[0,'胆大敢为，也易冒进'],天任:[2,'稳重可靠，得人扶持'],天冲:[1,'行动快，宜速战速决'],天辅:[2,'文昌之星，利学习与考试'],天英:[1,'热情显眼，注意急躁'],天芮:[0,'宜调整步伐，先解决积压问题'],天柱:[0,'言语多变，宜守不宜攻'],天心:[2,'思路清楚，利谋划与决策'],天禽:[2,'居中调和，诸事平稳']};
const QM_GOD={值符:[2,'有权威和贵人庇护'],螣蛇:[0,'事情容易反复，别轻信'],太阴:[2,'暗中有助力，适合低调筹划'],六合:[2,'利合作、感情和撮合'],白虎:[0,'阻力较大，宜稳妥'],玄武:[0,'信息不透明，防小失'],九地:[1,'宜守成、积累、慢慢来'],九天:[2,'利远行、扩张、大胆出手']};
const QM_ASK=[['求财','生门','door'],['事业','开门','door'],['感情','六合','god'],['考试','景门','door'],['出行','值使','zhishi']];
function qimenRead(q,askIdx){
 const[ask,key,type]=QM_ASK[askIdx];let p;
 if(type==='door')p=+Object.keys(q.door).find(k=>q.door[k]===key);else if(type==='god')p=+Object.keys(q.god).find(k=>q.god[k]===key);else p=q.zhishiAt;
 const d=q.door[p],s=q.star[p],g=q.god[p];const sc=QM_DOOR[d][0]+QM_STAR[s][0]+QM_GOD[g][0];
 const vc=sc>=5?0:sc>=3?1:sc>=2?2:3,verdict=['大顺','顺','平','宜缓'][vc];
 return{vc,score:sc,ask,key:type==='zhishi'?'值使（'+q.zhishi+'）':key,palace:p,gua:GUA[p],dir:DIR[p],door:d,star:s,god:g,verdict,lines:[d+'：'+QM_DOOR[d][1],s+'：'+QM_STAR[s][1],g+'：'+QM_GOD[g][1]]};
}


/* ---------- 随机（mulberry32，固件逐位一致） ---------- */
function rng(seed){let a=seed>>>0;return{u32(){a=(a+0x6D2B79F5)|0;let t=Math.imul(a^a>>>15,1|a);t=t+Math.imul(t^t>>>7,61|t)^t;return (t^t>>>14)>>>0},int(n){return Math.floor(this.u32()/4294967296*n)}}}
function dailySeed(y,m,d,py,pm,pd,ph,pg){let a=Math.imul(y*10000+m*100+d,0x9E3779B1)^Math.imul(py*10000+pm*100+pd,0x85EBCA77)^Math.imul(ph*2+pg+1,0xC2B2AE3D);a^=a>>>16;a=Math.imul(a,0x7FEB352D);a^=a>>>15;return a>>>0}
const YI=['早起','喝咖啡','整理桌面','给朋友发消息','散步','读十页书'],JI=['熬夜','冲动下单','空腹喝咖啡','和人争论','拖延'];
function pickN(r,n,k){const a=[...Array(n).keys()],o=[];while(o.length<k)o.push(a.splice(r.int(a.length),1)[0]);return o}
function dailyTicket(seed,sign){const r=rng(seed);const t={sign,signStars:2+r.int(4),tarot:r.int(22),tarotRev:r.int(10)<3,lotNo:1+r.int(100),hexUp:r.int(8),hexLo:r.int(8),yi:pickN(r,YI.length,3),ji:pickN(r,JI.length,2),serial:1+r.int(9999)};t.lotLevel=lotLevel(t.lotNo);return t}
/* 观音灵签签级（1..100）：0 上签 1 中签 2 下签。来源 game-art/fortune/text/guanyin/guanyin_lots.json */
const LOT_LEVEL='上下下上中中下上中中上上中中中中中上中中上中中下中中中中中中中中中中中中中下下中中上上中上中上中中中上中中下中中中中中下中中中下下下上中中下中中上下中中中上中上上中中下中上中中上上中上中下中上中下下下';
function lotLevel(no){return '上中下'.indexOf(LOT_LEVEL[no-1])}
function tarotHand(r){return{hand:pickN(r,22,7),rev:r.int(10)<3}}
/* ---------- 六爻 ---------- */
const TRIKEY={'111':0,'110':1,'101':2,'100':3,'011':4,'010':5,'001':6,'000':7};
function coinLine(backs){return backs.reduce((a,b)=>a+(b?3:2),0)}
function liuyao(vals){const yy=vals.map(v=>v===7||v===9?1:0),ch=vals.map(v=>v===6?1:v===9?0:(v===7?1:0));
 const idx=b=>TRIKEY[b.slice(3).join('')]*8+TRIKEY[b.slice(0,3).join('')];return{ben:idx(yy),bian:idx(ch),moving:vals.some(v=>v===6||v===9)}}


/* 六爻断法（朱熹《易学启蒙》）：动爻数决定读哪几条卦辞/爻辞；items 第一条为主
   kind: gua=卦辞 line=爻辞(pos 0=初爻) special=用九/用六；卦编号 = 上卦*8+下卦 */
function liuyaoRead(vals){const r=liuyao(vals),mv=[];vals.forEach((v,i)=>{if(v===6||v===9)mv.push(i)});
 const k=mv.length,still=[0,1,2,3,4,5].filter(i=>!mv.includes(i));let items;
 if(k===0)items=[['gua',r.ben]];else if(k===1)items=[['line',r.ben,mv[0]]];else if(k===2)items=[['line',r.ben,mv[1]],['line',r.ben,mv[0]]];
 else if(k===3)items=[['gua',r.ben],['gua',r.bian]];else if(k===4)items=[['line',r.bian,still[0]],['line',r.bian,still[1]]];
 else if(k===5)items=[['line',r.bian,still[0]]];else items=(r.ben===0||r.ben===63)?[['special',r.ben]]:[['gua',r.bian]];
 return{ben:r.ben,bian:r.bian,moving:k,items,main:items[0][1]}}

/* ---------- 紫微解读（原创短句） ---------- */
const ZW={紫微:['稳重有主见，天生的组织者','善于统筹大笔资源','适合管理、带团队','对伴侣要求高，也很护家'],
天机:['聪明灵活，点子多','靠头脑和策划赚钱','适合技术、策划、研究','感情细腻，想得多'],
太阳:['热情大方，乐于助人','财来自名声和口碑','适合公众面前的工作','付出型，照顾对方'],
武曲:['果断务实，执行力强','理财能力突出','适合金融、实业','外冷内热，慢热专一'],
天同:['温和随缘，懂得享受','财运平顺，不必强求','适合服务、艺术、轻松环境','重感情，相处舒服'],
廉贞:['个性鲜明，有原则','进财有起伏，宜稳','适合制度、法务、需要坚持的事','感情浓烈，需多沟通'],
天府:['包容稳当，有安全感','善于守财积累','适合管理、财务、守成','顾家可靠'],
太阴:['温柔细腻，审美好','财来得稳，适合积蓄','适合文艺、设计、幕后','感情含蓄而深'],
贪狼:['多才多艺，人缘好','机会多，进财快','适合社交、娱乐、销售','魅力强，桃花旺'],
巨门:['口才好，善分析','靠专业和表达赚钱','适合教学、咨询、讲解','先磨合后长久'],
天相:['正直守信，善协调','财务清楚有条理','适合辅佐、行政、协调','重承诺'],
天梁:['有长者风范，爱照顾人','财运来自稳定积累','适合医疗、教育、公益','成熟稳重型'],
七杀:['敢闯敢拼，有魄力','财来得猛，也要防大起大落','适合开拓、创业','爱憎分明'],
破军:['勇于变革，不怕从头来','先破后立，财运多变','适合改革、新领域','需要新鲜感']};
const ZW_FIELD={命宫:[0,'性格'],身宫:[0,'性格'],福德:[0,'心态'],迁移:[0,'在外表现'],财帛:[1,'财运'],田宅:[1,'家业'],官禄:[2,'事业'],仆役:[0,'交友'],兄弟:[0,'手足'],父母:[0,'长辈缘'],夫妻:[3,'感情'],子女:[3,'晚辈缘']};
const ZW_BRIGHT={庙:'星曜庙旺，力量最足。',旺:'星曜旺，表现积极。',得:'星曜得地，发挥正常。',利:'星曜得利，小有助力。',平:'星曜平和，需要自己用力。',不:'星曜力量偏弱，多积累更好。',陷:'星曜落陷，先打基础再发力。'};
const ZW_MUT={禄:'化禄：有福缘和资源。',权:'化权：掌控力强。',科:'化科：有名声、得贵人。',忌:'化忌：容易执着，多留意心态。'};
function ziweiRead(a,idx){
 const p=a.palaces[idx];let stars=p.majorStars,borrow=false;
 if(!stars.length){stars=a.palaces[(idx+6)%12].majorStars;borrow=true}
 if(p.name==='疾厄')return{p,stars,borrow,lines:['疾厄宫只看星曜分布，','这里不做健康方面的判断。']};
 const f=ZW_FIELD[p.name]||[0,'整体'];const lines=[];
 if(borrow)lines.push('空宫，借对宫星曜来看。');
 stars.forEach(s=>{const t=ZW[s.name];if(t)lines.push(f[1]+'：'+t[f[0]]+'。');if(s.brightness&&ZW_BRIGHT[s.brightness]&&!borrow)lines.push(ZW_BRIGHT[s.brightness]);if(s.mutagen)lines.push(ZW_MUT[s.mutagen])});
 p.minorStars.forEach(s=>{if(s.mutagen)lines.push(s.name+ZW_MUT[s.mutagen])});
 return{p,stars,borrow,lines};
}

/* 今日双色球 v1：数值字段按固定顺序混合，独立于互动随机源；当天东八区 12:00 起局。
   个性化娱乐选号，不是预测模型，不读取开奖历史。改字段顺序即改变算法版本。 */
function dailySsq(y,m,d,py,pm,pd,ph,pg){
 const valid=(y,m,d)=>Number.isInteger(y)&&y>=1901&&y<=2100&&Number.isInteger(m)&&m>=1&&m<=12&&Number.isInteger(d)&&d>=1&&d<=new Date(Date.UTC(y,m,0)).getUTCDate();
 if(!valid(y,m,d)||!valid(py,pm,pd)||!Number.isInteger(ph)||ph<0||ph>11||!Number.isInteger(pg)||pg<0||pg>1)return null;
 const iz=root.iztro||(typeof require!=='undefined'?require('iztro'):undefined);
 const pl=Solar.fromYmdHms(py,pm,pd,ph===0?0:ph*2,30,0).getLunar(),l=Solar.fromYmdHms(y,m,d,12,0,0).getLunar();
 const sign=(()=>{let s=9;for(const[v,k]of[[120,10],[219,11],[321,0],[420,1],[521,2],[622,3],[723,4],[823,5],[923,6],[1024,7],[1123,8],[1222,9]])if(pm*100+pd>=v)s=k;return s})();
 const base=dailySeed(y,m,d,py,pm,pd,ph,pg),t=dailyTicket(base,sign),b=bone(pl),q=qimen(l),qr=qimenRead(q,0);
 const a=iz.astro.bySolar(`${py}-${pm}-${pd}`,ph,pg===0?'男':'女',true,'zh-CN');
 const five={水二局:2,木三局:3,金四局:4,土五局:5,火六局:6}[a.fiveElementsClass];
 const major=['紫微','天机','太阳','武曲','天同','廉贞','天府','太阴','贪狼','巨门','天相','天梁','七杀','破军'];
 const minor=['左辅','右弼','文昌','文曲','天魁','天钺','禄存','天马','地空','地劫','火星','铃星','擎羊','陀罗'];
 const pal=['命宫','父母','福德','田宅','官禄','仆役','迁移','疾厄','财帛','子女','夫妻','兄弟'];
 const soul=a.palaces.findIndex(p=>p.name==='命宫'),body=a.palaces.findIndex(p=>p.isBodyPalace);
 const starCode=n=>major.includes(n)?major.indexOf(n):100+minor.indexOf(n);
 const ma=Array(14).fill(-1),br=Array(14).fill(0),mu=Array(14).fill(-1),mi=Array(14).fill(-1),nm=Array(14).fill(-1);
 a.palaces.forEach((p,i)=>{p.majorStars.forEach(s=>{const k=major.indexOf(s.name);if(k<0)return;ma[k]=i;br[k]=Math.max(0,['','庙','旺','得','利','平','不','陷'].indexOf(s.brightness||''));mu[k]=s.mutagen?'禄权科忌'.indexOf(s.mutagen):-1});p.minorStars.forEach(s=>{const k=minor.indexOf(s.name);if(k<0)return;mi[k]=i;nm[k]=s.mutagen?'禄权科忌'.indexOf(s.mutagen):-1})});
 const lucky=1+rng((base^0x4C55434B)>>>0).int(9),e=l.getEightChar(),pe=pl.getEightChar();
 const values=[0x53535101,base,pl.getYear(),Math.abs(pl.getMonth()),pl.getDay(),+(pl.getMonth()<0),...['Year','Month','Day','Time'].map(k=>gzIdx(pe['get'+k]())),
  l.getYear(),Math.abs(l.getMonth()),l.getDay(),+(l.getMonth()<0),...['Year','Month','Day','Time'].map(k=>gzIdx(e['get'+k]())),
  sign,t.signStars,lucky,t.tarot,+t.tarotRev,t.lotNo,t.lotLevel,t.hexUp,t.hexLo,...t.yi,...t.ji,...b.parts.map(x=>Math.round(x[2]*10)),Math.round(b.total*10),
  soul,body,five,starCode(a.soul),starCode(a.body)];
 a.palaces.forEach(p=>values.push(GAN.indexOf(p.heavenlyStem),pal.indexOf(p.name)));
 for(let i=0;i<14;i++)values.push(ma[i],br[i],mu[i],mi[i],nm[i]);
 values.push(+q.yang,'上中下'.indexOf(q.yuan[0]),q.ju,gzIdx(q.dayGZ),gzIdx(q.shiGZ),q.zhifuAt,q.zhishiAt);
 for(let p=1;p<=9;p++){values.push(GAN.indexOf(q.earth[p]));if(p!==5)values.push(GAN.indexOf(q.heaven[p][0]),Object.keys(STAR0).map(Number).find(k=>STAR0[k]===q.star[p]),Object.keys(DOOR0).map(Number).find(k=>DOOR0[k]===q.door[p]),GODS.indexOf(q.god[p]))}
 let seed=2166136261;for(const v of values)seed=Math.imul(seed^(v>>>0),16777619)>>>0;
 const r=rng(seed),bounded=n=>{let u;do{u=r.u32()}while(u<4294967296%n);return u%n},pool=Array.from({length:33},(_,i)=>i+1),red=[];
 for(let i=0;i<6;i++)red.push(pool.splice(bounded(pool.length),1)[0]);red.sort((a,b)=>a-b);
 return{seed,red,blue:1+bounded(16),lucky,ticket:t,lunar:[l.getYear(),Math.abs(l.getMonth()),l.getDay(),+(l.getMonth()<0)],dayGz:gzIdx(l.getDayInGanZhi()),boneTotal:Math.round(b.total*10),soul,body,five,qYang:q.yang,qYuan:'上中下'.indexOf(q.yuan[0]),qJu:q.ju,qPalace:qr.palace};
}
root.FortuneCore={dailySsq,liuyaoRead,lotLevel,LOT_LEVEL,rng,dailySeed,dailyTicket,tarotHand,coinLine,liuyao,YI,JI,BONE_BAND,GAN,ZHI,gzIdx,bone,qimen,qimenRead,QM_ASK,GUA,DIR,RING,ziweiRead,ZW_FIELD};
})(typeof window!=='undefined'?window:globalThis);
