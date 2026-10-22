# 第三方许可与来源

| 部分 | 来源与许可 | 许可文件 |
|---|---|---|
| AI Passport BSP | FoloToy，MIT；本项目无官方关联 | firmware/LICENSE.FoloToy、licenses/FoloToy-MIT.txt |
| ESP-IDF 5.5.3 | Espressif，Apache 2.0 | licenses/ESP-IDF-Apache-2.0.txt |
| button、cmake_utilities、esp_codec_dev、esp_lvgl_port | Espressif；原许可随发行包保留 | licenses/ 下对应文件 |
| LVGL | LVGL contributors，MIT | licenses/LVGL-MIT.txt |
| lunar-javascript | 农历参考实现，MIT | prototype/vendor/LICENSE-lunar-javascript.txt |
| iztro | 紫微参考实现，MIT | prototype/vendor/LICENSE-iztro.txt |
| 源字体及点阵子集 | 霞鹜文楷、思源黑体、思源宋体、Space Mono、站酷庆科黄油体；SIL OFL 1.1 | tools/fonts/OFL-*.txt，发行包 licenses/ |

源字体哈希见 tools/fonts/README.md，固件子集字号与来源哈希见 game-art/fortune/sprites/fonts/fonts.json。点阵子集经过裁字、栅格化和 4 bpp 编码，原字体许可与署名保留。

观音灵签采用传统文字，现代白话由本项目编写。采用来源及异文记录见 game-art/fortune/text/guanyin/SOURCES.md、VARIANTS.md。

周易底本、逐爻来源、转录署名和校定记录见 game-art/fortune/text/zhouyi/ZHOUYI.md。本项目保留“yigua.art網站轉譯”及《御纂周易折中》1715 武英殿本的来源说明；数据集对古籍原文与原创说明区分授权，具体见该目录 src/LICENSE.md（发行包 licenses/yigua-data-license.md）。CC0 对照数据来源 HeTaobaba123/iching-64-hexagrams-dataset；标点对照来源 sunls2/zhouyi。第三方材料不因本项目源码 MIT 许可而改变原许可。

开发对拍依赖固定版本见 package.json；它们不作为设备运行时联网服务。原型 vendor 目录保留对应许可证。NABO 美术与音视频的授权范围见 NOTICE.md。
