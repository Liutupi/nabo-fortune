"""Daily SSQ: JS/external calendar vs C, fixed v1 vector, bounds and input checks."""
import ctypes as C
import json
import os
from pathlib import Path
import subprocess
import tempfile
import test_fortune as core

ROOT = Path(__file__).resolve().parents[1]
class Ssq(C.Structure):
    _fields_ = [('seed', C.c_uint32), ('red', C.c_uint8 * 6), ('blue', C.c_uint8),
                ('lucky', C.c_uint8), ('ticket', core.Ticket), ('lunar', core.Lunar),
                ('day_gz', C.c_int8), ('bone_total', C.c_uint8), ('soul', C.c_int8),
                ('body', C.c_int8), ('five', C.c_int8), ('q_yang', C.c_bool),
                ('q_yuan', C.c_int8), ('q_ju', C.c_int8), ('q_palace', C.c_int8)]

def normalize(o):
    return dict(seed=o.seed, red=list(o.red), blue=o.blue, lucky=o.lucky,
                lunar=[o.lunar.year, o.lunar.month, o.lunar.day, int(o.lunar.leap)],
                dayGz=o.day_gz, boneTotal=o.bone_total, soul=o.soul, body=o.body,
                five=o.five, qYang=o.q_yang, qYuan=o.q_yuan, qJu=o.q_ju, qPalace=o.q_palace)

def main():
    standalone = (ROOT/'prototype/fortune-core.js').read_text().strip()
    html = (ROOT/'prototype/index.html').read_text()
    assert standalone in html, 'embedded browser core drifted from standalone core'
    oracle = r'''
require('./prototype/fortune-core.js'); const F=FortuneCore;
const cases=[[2026,10,24,1985,6,15,4,0]];
for(const date of [[1901,1,1],[2100,12,31],[2000,2,29],[2026,2,4],[2026,2,17],[2026,12,31],[2027,1,1]])
 for(const birth of [[1901,1,1],[1933,7,23],[1984,11,23],[2000,2,29],[2023,3,22],[2023,4,6],[2100,12,31]])
  for(const h of [0,6,11])for(const g of [0,1])cases.push([...date,...birth,h,g]);
const r=F.rng(20261024);for(let i=0;i<1500;i++){
 const y=1901+r.int(200),m=1+r.int(12),d=1+r.int(new Date(Date.UTC(y,m,0)).getUTCDate());
 const py=1930+r.int(96),pm=1+r.int(12),pd=1+r.int(new Date(Date.UTC(py,pm,0)).getUTCDate());
 cases.push([y,m,d,py,pm,pd,r.int(12),r.int(2)]);
}
console.log(JSON.stringify(cases.map(input=>({input,out:F.dailySsq(...input)}))));'''
    rows = json.loads(subprocess.check_output(['node','-e',oracle], cwd=ROOT, text=True))
    assert rows[0]['out']['seed'] == 277198611
    assert rows[0]['out']['red'] == [10,13,17,19,25,32] and rows[0]['out']['blue'] == 4
    with tempfile.TemporaryDirectory() as tmp:
        lib = core.build(core.SRC, str(Path(tmp)/'ssq.so'))
        lib.fc_daily_ssq.argtypes = [C.c_int]*8+[C.POINTER(Ssq)]
        lib.fc_daily_ssq.restype = C.c_bool
        seen_red, seen_blue, seen_numbers = set(), set(), set()
        for row in rows:
            o = Ssq()
            assert lib.fc_daily_ssq(*row['input'],C.byref(o)), row['input']
            expected = {k:row['out'][k] for k in normalize(o)}
            assert normalize(o)==expected, (row['input'], normalize(o), expected)
            red=list(o.red)
            assert len(set(red))==6 and red==sorted(red) and 1<=red[0]<=red[-1]<=33
            assert 1<=o.blue<=16
            seen_red.update(red);seen_blue.add(o.blue);seen_numbers.add((*red,o.blue))
            other = Ssq()
            assert lib.fc_daily_ssq(*row['input'],C.byref(other))
            assert bytes(o)==bytes(other), 'repeat call changed result'
            # Daily draws and other RNG use must not alter the SSQ stream.
            rng=core.Rng(o.seed);lib.fc_rng_u32(C.byref(rng))
            assert lib.fc_daily_ssq(*row['input'],C.byref(other)) and bytes(o)==bytes(other)
        assert seen_red==set(range(1,34)) and seen_blue==set(range(1,17))
        assert len(seen_numbers)>len(rows)*.9
        base=[2026,10,24,1985,6,15,4,0]
        invalid=[(0,1900),(0,2101),(1,0),(1,13),(2,0),(2,32),(3,1900),(3,2101),(4,13),(5,31),(6,-1),(6,12),(7,-1),(7,2)]
        for idx,value in invalid:
            args=base.copy();args[idx]=value;o=Ssq()
            assert not lib.fc_daily_ssq(*args,C.byref(o)),args
        for args in [[2026,2,29,*base[3:]],[*base[:3],1985,2,29,4,0],[*base[:3],1985,4,31,4,0]]:
            assert not lib.fc_daily_ssq(*args,C.byref(Ssq())),args
        assert not lib.fc_daily_ssq(*base,None)
        seeds=set()
        for idx,value in enumerate([2027,11,25,1986,7,16,5,1]):
            args=base.copy();args[idx]=value;o=Ssq()
            assert lib.fc_daily_ssq(*args,C.byref(o));seeds.add(o.seed)
        assert len(seeds)==8 and 277198611 not in seeds, 'an input field was ignored'
    print(f'SSQ PASS: {len(rows)} JS/C pairs, fixed vector, repeatability, all balls, invalid inputs, all profile fields')

if __name__=='__main__':main()
