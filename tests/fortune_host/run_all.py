"""跑遍全部场景：每个场景按原型的操作走一遍，逐帧校验增量==整屏，导出关键帧和总览图。
用法：sh run.sh <assets.bin> <out_dir> run_all.py"""
import ctypes, os, sys
from run_ui import UI, L, UP, DOWN, OK, sheet

def main(blob, out):
    os.makedirs(out, exist_ok=True)
    u = UI(blob, seed=20261022)
    shots = []
    def snap(tag):
        p = os.path.join(out, tag + '.png'); u.png(p); shots.append(p)
    def run(ms, *tags_at):
        """推进 ms，并在 (时间, 标签) 处截图"""
        el = 0
        for at, tag in sorted(tags_at):
            u.run(at - el); el = at; snap(tag)
        u.run(ms - el)
    # 摇卦：固定六爻 7 8 9 6 7 8（两动爻），走一遍完整动画
    force = (ctypes.c_int * 6).in_dll(L, 'fc_shake_force')
    for i, v in enumerate([7, 8, 9, 6, 7, 8]): force[i] = v
    u.go('shake'); run(300, (300, 'shake_0ready'))
    for k in range(6):
        u.key(OK, 1); u.run(700 if k == 0 else 400)
        if k == 0: snap('shake_1hold')
        u.key(OK, 0)
        if k == 0: run(2600, (750, 'shake_2toss'), (1100, 'shake_3land'))
        elif k < 5: u.run(2600)
    run(1800, (1000, 'shake_3b_last'))
    run(1600, (600, 'shake_4reveal'), (1600, 'shake_5done'))
    u.tap(OK); run(1500, (1400, 'shake_6card'))
    for i in range(6): force[i] = 0
    # 塔罗
    u.go('tarot'); run(2600, (1200, 'tarot_0shuffle'))
    u.run(1200); snap('tarot_1fan'); u.tap(DOWN); u.run(300); u.tap(OK)
    run(4200, (600, 'tarot_2fly'), (2500, 'tarot_3flip'), (4200, 'tarot_4show'))
    # 生辰输入：改年份、月份，保存
    u.go('birth'); u.run(300); snap('birth_0')
    u.key(UP, 1); u.run(1000); u.key(UP, 0); u.run(300); snap('birth_1year')
    for _ in range(4): u.tap(OK); u.run(60)
    u.run(200); snap('birth_2gender'); u.tap(OK); u.run(200)
    # 称骨
    u.go('bone'); u.run(300); snap('bone_0confirm'); u.tap(OK)
    run(9600, (1600, 'bone_1drop'), (7100, 'bone_2slide'), (9600, 'bone_3balance'))
    u.tap(OK); run(900, (900, 'bone_4card'))
    # 星座
    u.go('star'); u.run(300); snap('star_0pick'); u.tap(OK)
    run(3800, (700, 'star_1draw'), (3800, 'star_2done'))
    # 紫微
    u.go('ziwei'); u.run(300); u.tap(OK)
    run(3300, (400, 'ziwei_0cells'), (1800, 'ziwei_1fly'), (3300, 'ziwei_2chart'))
    u.tap(DOWN); u.run(200); u.tap(OK); run(1200, (1200, 'ziwei_3detail'))
    # 奇门
    u.go('qimen'); u.run(300); snap('qimen_0ask'); u.tap(OK)
    run(4600, (1000, 'qimen_1fly'), (2200, 'qimen_2star'), (3300, 'qimen_3door'), (4600, 'qimen_4done'))
    u.tap(OK); run(1000, (1000, 'qimen_5read'))
    # 今日小票
    u.go('daily'); run(2000, (2000, 'daily_0idle')); u.tap(OK)
    run(5200, (1200, 'daily_1print'), (5200, 'daily_2done'))
    assert L.fc_ui_image_misses()==0
    print(f'frames={u.frames} mismatchFrames={u.bad} avgStrips={u.strips / u.frames:.1f}/40 maxOps={u.maxops} cache={L.host_cache_bytes()} image_misses={L.fc_ui_image_misses()}')
    sheet(shots, os.path.join(out, 'sheet_all.png'), cols=8)

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
