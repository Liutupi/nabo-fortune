// 由 lunar-javascript 1.7.7 生成固件用的农历表和节气表（只在开发机上运行）
// 输出 firmware/main/fortune/fortune_tables.c
const fs=require('fs');const {Solar,LunarYear}=require('lunar-javascript');
const Y0=1900,Y1=2101;
const JQ=['小寒','大寒','立春','雨水','惊蛰','春分','清明','谷雨','立夏','小满','芒种','夏至','小暑','大暑','立秋','处暑','白露','秋分','寒露','霜降','立冬','小雪','大雪','冬至'];
const jdn=(y,m,d)=>{const a=Math.floor((14-m)/12),yy=y+4800-a,mm=m+12*a-3;return d+Math.floor((153*mm+2)/5)+365*yy+Math.floor(yy/4)-Math.floor(yy/100)+Math.floor(yy/400)-32045};
const PY={DA_XUE:'大雪',DONG_ZHI:'冬至',XIAO_HAN:'小寒',DA_HAN:'大寒',LI_CHUN:'立春',YU_SHUI:'雨水',JING_ZHE:'惊蛰'};
const J0=jdn(1900,1,1);
// 农历年：正月初一 JDN、月大小（按月序，最多 13 个）、闰月
const ly=[];
for(let y=Y0;y<=Y1;y++){const ms=LunarYear.fromYear(y).getMonths().filter(m=>m.getYear()===y);
 let bits=0,leap=0;ms.forEach((m,i)=>{if(m.getDayCount()===30)bits|=1<<i;if(m.getMonth()<0)leap=-m.getMonth()});
 if(ms.length!==(leap?13:12))throw new Error('month count '+y);
 const s=Solar.fromJulianDay(ms[0].getFirstJulianDay());ly.push([jdn(s.getYear(),s.getMonth(),s.getDay())-J0,bits,leap])}
// 节气：每个公历年 24 个，分钟（向上取整），自 1900-01-01 00:00 起，东八区
const jq=[];
for(let y=Y0;y<=Y1;y++){const m=new Map();for(const md of[[1,1],[4,1],[7,1],[10,1],[12,31]]){const t=Solar.fromYmd(y,md[0],md[1]).getLunar().getJieQiTable();for(let[k,v]of Object.entries(t)){k=PY[k]||k;if(!JQ.includes(k))continue;if(v.getYear()!==y)continue;m.set(k,v)}}
 const row=JQ.map(n=>{const v=m.get(n);if(!v)throw new Error('miss '+y+n);return (jdn(v.getYear(),v.getMonth(),v.getDay())-J0)*1440+v.getHour()*60+v.getMinute()+(v.getSecond()>0?1:0)});
 for(let i=1;i<24;i++)if(row[i]<=row[i-1])throw new Error('order '+y);jq.push(row)}
let c=`/* 自动生成：tools/gen_fortune_tables.js（来源 lunar-javascript 1.7.7，MIT）。不要手改。 */
#include "fortune_core.h"
const int32_t FC_LUNAR_YEAR0 = ${Y0};
const int32_t FC_LUNAR_YEAR1 = ${Y1};
/* {正月初一距 1900-01-01 的天数, 月大小位(第 i 个月为 30 天), 闰月(0=无)} */
const fc_lunar_year_t FC_LUNAR_YEARS[${ly.length}] = {\n${ly.map(r=>`  {${r[0]},0x${r[1].toString(16)},${r[2]}}`).join(',\n')}\n};
/* 每年 24 节气（小寒起），距 1900-01-01 00:00 的分钟数（秒向上取整） */
const int32_t FC_JIEQI_MIN[${jq.length}][24] = {\n${jq.map(r=>'  {'+r.join(',')+'}').join(',\n')}\n};
`;
fs.writeFileSync(__dirname+'/../firmware/main/fortune/fortune_tables.c',c);
console.log('lunar years',ly.length,'jieqi years',jq.length,'bytes ~',ly.length*8+jq.length*96);
