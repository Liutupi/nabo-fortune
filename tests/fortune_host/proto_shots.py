import asyncio
from playwright.async_api import async_playwright
import os, sys
D=os.path.join(sys.argv[1] if len(sys.argv) > 1 else 'out', 'p_'); os.makedirs(os.path.dirname(D), exist_ok=True)
async def main():
  async with async_playwright() as p:
    b=await p.chromium.launch(); ctx=await b.new_context(timezone_id='Asia/Shanghai'); pg=await ctx.new_page(); errs=[]
    pg.on('pageerror',lambda e:errs.append(str(e)))
    await pg.clock.set_fixed_time('2026-10-22T14:30:00+08:00')
    await pg.route('https://**/*',lambda r:r.abort())
    await pg.goto('http://localhost:8765/index.html#menu'); await pg.wait_for_timeout(1200)
    async def shot(n,js,w=400):
      await pg.evaluate("(()=>{const N=window.__nf,S=N.S;"+js+"})()"); await pg.wait_for_timeout(w); await pg.locator('#scr').screenshot(path=D+n+'.png')
    L="[7,8,9,6,7,8].map(x=>({v:x,t:0}))"
    await shot('shake_done',f"N.go('shake');const s=S.shake;s.lines={L};s.ph='done';s.rt=-99999;s.card=0")
    await shot('shake_card',f"const s=S.shake;s.card=1;s.ct=-99999")
    await shot('bone_confirm',"N.go('bone')")
    await shot('bone_card',"S.bone.start();S.bone.wt0=-99999",1500)
    await shot('ziwei_chart',"N.go('ziwei');S.ziwei.start();S.ziwei.ct=-99999",600)
    await shot('ziwei_detail',"const z=S.ziwei;z.cur=(z.soul+1)%12;z.ph='detail';z.dt=-99999;z.rd=FortuneCore.ziweiRead(z.a,z.cur)")
    await shot('qimen_ask',"N.go('qimen');S.qimen.sel=0;S.qimen.enter()")
    await shot('qimen_done',"S.qimen.start();S.qimen.at=-99999",600)
    await shot('qimen_read',"S.qimen.ph='read';S.qimen.rt=-99999")
    await shot('daily_done',"N.go('daily');const d=S.daily;d.make();d.ph='done';d.L=d.TH;d.tt=-99999")
    await shot('birth',"N.go('birth')")
    print('ERR',errs); await b.close()
asyncio.run(main())
