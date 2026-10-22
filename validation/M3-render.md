# M3 渲染验收：固件直绘引擎 + 全部场景（宿主机）

日期：2026-10-22

## 做了什么

- `firmware/main/fortune/gfx/`：显示列表 + 8 行条带合成（RGB565），每条带按落在其中的绘制操作做哈希，和上一帧相同就不重画、不送屏。全部整数运算（千分比进度、1/1024 圈角度、Q14 正弦表）。
  - 图元：矩形（带透明度）、线、圆/椭圆、圆环、三角形、多边形（五角星、旋转矩形）、太极、调色板图像（整幅/局部/仿射旋转缩放）、4 bpp 抗锯齿文字。
  - 素材总包 `fortune_assets.bin` 原地读取（设备上直接 mmap 分区）；精灵帧解码缓存按 (图, 帧) 做 LRU，字节预算 112 KB，同一帧里引用同一张图的不同帧也安全。
- `firmware/main/fortune/ui/`：场景框架（时间、按键、双击 ↑ 返回、震屏、粒子、顶栏、NABO 精灵 12 fps）和 11 个场景，逐行对照原型移植：开机、主转盘、生辰输入、今日小票、摇卦（含解卦卡）、灵签、塔罗、称骨、星座、紫微、奇门。
- `firmware/main/fortune/fortune_text.c`：界面和解读文案由 `tools/gen_fortune_text.js` 直接从原型抽取生成，固件和原型一字不差；`fortune_names.c` 为干支、节气、星曜、宫名等固定名称表。
- 字体：`tools/gen_fonts_fortune.py` 字符集改为 原型 + 固件 C 字符串 + 签文/周易文本 全量统计；霞鹜文楷和站酷庆科缺字时回退思源黑体同字号（站酷缺的 ▲▼★☆ 和生僻字）。

## 怎么验的

`tests/fortune_host/`：把固件 UI 编成共享库（ASan + UBSan），Python 用 ctypes 按键、推进时间、取帧缓冲。

| 检查 | 结果 |
|---|---|
| 每一帧：增量合成（只重画脏条带）与整屏重画逐像素比较 | 3534 帧全部一致（run_all.py），另 761 帧（run_ui.py）一致 |
| ASan / UBSan | 无报错（修掉 2 处负数左移） |
| 显示列表峰值 | 195 / 400 个操作，无溢出 |
| 精灵解码缓存峰值 | 109 KB（预算 112 KB） |
| 平均每帧重画条带 | 6.1 / 40（静止画面为 0） |
| 字形覆盖：界面、名称表、签文、卦爻辞用到的 2008 个字 × 7 套字体 | 0 缺字（check_glyphs.py） |
| 与原型同状态对照（同生辰、同时间 2026-10-22 14:30） | 摇卦解卦卡、称骨卡、紫微盘面与解读、奇门盘面与解读、今日小票、生辰页：内容逐字一致，版式一致，见 m3/compare_*.png |

对照图左为原型（浏览器），右为固件渲染。差异只有字号取整（原型 9.5/10.5/12.5 px 对应到 9/10/11/13/15/24 六档点阵）和原型里标题字体没加载时的系统字回退。

## 复现

```sh
sh tests/fortune_host/build.sh
sh tests/fortune_host/run.sh firmware/assets/fortune_assets.bin out run_all.py   # 全场景 + 增量/整屏逐帧比对
sh tests/fortune_host/run.sh firmware/assets/fortune_assets.bin out run_cmp.py   # 固定状态截图 c_*.png
python tests/fortune_host/proto_shots.py out                                    # 原型同状态截图 p_*.png（先开 打开原型.bat）
python tests/fortune_host/check_glyphs.py firmware/assets/fortune_assets.bin firmware/assets/assets_manifest.json firmware/main/fortune
```

## 留给 M4

- 真机速度：条带合成 + SPI 送屏的帧率要在 ESP32-C3 上实测；显示列表约 37 KB、条带 3.8 KB、文字区 8 KB、解码缓存 ≤112 KB。
- disp24 字体 448 KB 是全字符集，标题字只用几十个字，可以裁到只含标题用字。
- 音效目前是 `fc_sfx_play` 空实现，M4 接蜂鸣器/功放；`fc_now`、`fc_profile_save` 接 RTC 与 NVS。
