# 字体源文件（tools/fonts）

全部为 SIL Open Font License 1.1，可以免费商用和再分发；固件里只放子集化后的点阵字形。
SHA256 与 `game-art/fonts.json` 的 `source_sha256` 对应；源字体一旦更换，必须重新生成字形并更新哈希。

| 文件 | 用途 | 许可 | SHA256 |
|---|---|---|---|
| ZCOOLQingKeHuangYou-Regular.ttf | 霓虹标题、菜单大字（站酷庆科黄油体） | OFL-ZCOOLQingKeHuangYou.txt | 54f0c0df4308cd74cd0f2fd3494ae054dbc4a1fd6fa7d71f4807eb4cdd8b4136 |
| SourceHanSerifCN-SemiBold.otf | 不再使用（已改用霞鹜文楷），可以删除 | OFL-SourceHanSerif.txt | 50d3f12201ce2dd840650b6bfa47ebb8572ecf767eec0e5ccb20a247b5bfa805 |
| SourceHanSansCN-Medium.otf | 解读正文、界面小字（思源黑体，和小智版同一个文件） | OFL-SourceHanSans.txt | a94e558a2fe972bee4f46bce0843abff37063fd68c33f1e7d9058f6f09432b01 |
| SpaceMono-Bold.ttf | 日期、数字、小票编号（和小智版同一个文件） | OFL-SpaceMono.txt | 405e73d41afb7e5906efce206a326af5c956f38e255f35421c260e861e599c59 |
| LXGWWenKai-Regular.ttf | **签诗、称骨歌、卦名、牌名、宫名、星曜名**（霞鹜文楷，2026-10-22 定） | OFL-LXGW.txt | bf73f995c05f3f7baf7840b9d76d8d9678c8b150a020946a394ca63c67ed1350 |

## 固件字号规划（M3 时用 lv_font_conv 1.5.3 生成，4 bpp）

| 字体名 | 源 | 像素 | 字符集 |
|---|---|---|---|
| fortune_disp_24 / _32 | 庆科黄油体 | 24 / 32 | 界面标题用字（几十个） |
| fortune_kai_14 / _18 | 霞鹜文楷 | 14 / 18 | 签诗 100 首 + 称骨歌 52 首 + 64 卦名和卦辞 + 22 牌名 + 星曜宫名（预计约 2500 字） |
| fortune_sans_11 / _13 | 思源黑体 | 11 / 13 | 解读文案和界面（预计约 1500 字） |
| fortune_mono_10 / _12 | Space Mono | 10 / 12 | ASCII |

字符集从文本库自动统计，不手写。

浏览器原型用的是 `prototype/fonts/LXGWWenKai-subset.woff2`（从文楷裁出原型用到的 2073 个字，约 460 KB）；其他字体从这里加载。用 `打开原型.bat` 打开，在电脑上不联网也能显示正确字形。
