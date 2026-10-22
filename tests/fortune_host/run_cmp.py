"""与原型逐屏对照用的固定状态截图（默认示例生辰 1985-06-15 辰时 男，时间 2026-10-22 14:30）。
用法：sh run.sh <assets.bin> <out_dir> run_cmp.py"""
import ctypes, os, sys
from run_ui import UI, L, UP, DOWN, OK

def main(blob, out):
    os.makedirs(out, exist_ok=True)
    u = UI(blob, seed=7)
    snap = lambda tag: u.png(os.path.join(out, 'c_' + tag + '.png'))
    force = (ctypes.c_int * 6).in_dll(L, 'fc_shake_force')
    for i, v in enumerate([7, 8, 9, 6, 7, 8]): force[i] = v
    u.go('shake'); u.run(200)
    for k in range(6):
        u.key(OK, 1); u.run(500); u.key(OK, 0); u.run(2600)
    u.run(2500); snap('shake_done'); u.tap(OK); u.run(2500); snap('shake_card')
    u.go('bone'); u.run(200); snap('bone_confirm'); u.tap(OK); u.run(7000); snap('bone_card')
    u.go('ziwei'); u.run(200); u.tap(OK); u.run(4000); snap('ziwei_chart')
    u.tap(DOWN); u.run(200); u.tap(OK); u.run(2000); snap('ziwei_detail')
    u.go('qimen'); u.run(200); snap('qimen_ask'); u.tap(OK); u.run(5000); snap('qimen_done')
    u.tap(OK); u.run(1500); snap('qimen_read')
    u.go('daily'); u.run(300); u.tap(OK); u.run(9000); snap('daily_done')
    u.go('birth'); u.run(300); snap('birth')
    print('mismatchFrames', u.bad)

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
