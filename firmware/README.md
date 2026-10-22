# NABO 赛博算命机：独立固件

ESP32-C3 / 8 MB Flash / AI Passport，开机直接进入算命机，无联网流程。工程使用 ESP-IDF **5.5.3**；本轮构建的程序版本为 0.1.2。

## 构建

激活 ESP-IDF 5.5.3 环境后，在本目录运行：

```sh
idf.py set-target esp32c3
idf.py build
idf.py merge-bin -o nabo-fortune-merged.bin
python tools/verify_firmware.py build
```

`merge-bin` 的输出路径相对于 `build/`，因此上述产物为 `build/nabo-fortune-merged.bin`。校验脚本会检查镜像、分区 MD5、精确分区布局、素材 FNAS 目录与合并内容。

Windows 本次使用离线组件时可运行 `tools/build.ps1`。它默认从用户缓存目录 `.cache/nabo-fortune/` 查找 IDF 5.5.3 和 `vendor-components/`，从 `.espressif/` 查找已安装的 Python 环境、CMake、Ninja 和 GCC；均可通过参数改路径。`-Action build` 仅编译，`-Action merge` 合并并校验，默认执行全部。

离线组件来自交接的 `idf553-vendor-components.tgz`，解压后组件目录名为 `button`、`cmake_utilities`、`esp_codec_dev`、`esp_lvgl_port`、`lvgl`。该归档未带 LVGL 示例目录，需创建空的 `lvgl/examples/` 与 `lvgl/demos/`，供其 CMake include 路径检查使用；配置中关闭示例与演示。在线构建可使用组件管理器读取 BSP 的 `idf_component.yml`。

## 分区与安装

| 镜像或分区 | 偏移 | 分区容量 |
|---|---|---|
| bootloader | 0x0 | 到 0x8000 |
| partition table | 0x8000 | 0x1000 |
| nvs | 0x9000 | 0x6000 |
| phy_init | 0xf000 | 0x1000 |
| factory app | 0x10000 | 0x1f0000 |
| assets | 0x200000 | 0x600000 |

这是独立固件的全新安装布局。首次从其他固件切换时，应核对目标 MAC、8 MB 容量及安全状态，完整备份并验证原 Flash，获得全覆盖授权后安装；合并镜像写入地址为 0x0。不能套用单词护照的素材偏移或小智版的升级脚本。

已经安装本布局后，调试时可只更新 0x10000 的应用；素材改变时还需更新 0x200000 的素材包。应先核对 bootloader 和分区表一致，保留 NVS。刷后必须读回比对，再做真机运行验证。

备份、串口日志和个人档案只保存到项目根目录的 `private/`，不进入发布包。原固件的整片备份也不能作为发布镜像。

## 操作与诊断

- 上、下键选择，OK 确认；摇卦与摇签使用 OK 按住、松开。
- 双击上键回主转盘；生辰输入与校准时间页禁用此快捷键。
- 解读卡片用上、下键翻页；称骨称完后保留杆秤与总重，按 OK 进入白话、原诗、骨重明细三页。OK 按该玩法返回或重新开始。
- 开机先响提示音，NABO 出场时播放“在此恭候”语音。正文 16 px，辅助提示至少 14 px。
- 开机 OK 开始；时间未知时进入校准页，开机页下键也可校准。
- 无操作 5 分钟保存时间、关闭屏幕与音频并进入深睡，按键唤醒。

USB 串口每行一条命令，可带 `>` 或 `nf ` 前缀：

```text
status
perf
go tarot
key o 1
key o 0
time 2026-10-22 14:30
pf 1985 6 15 4 0
sfx 0
restart
```

塔罗抽出 1.2 秒，停顿 0.6 秒，翻面 1.4 秒，揭晓后保留牌面直到按 OK。`perf` 的 `image_misses` 记录图像句柄查找失败次数。

`sfx 0..10` 播放音效；`restart` 保存时钟后软件重启。`pf clear` 删除本应用生辰档案并恢复示例，供测试清理使用。日期、时间、时辰与性别参数会校验。`NF` 回显含版本、heap、largest、uptime_ms；实体按键事件另记 `fortune: button key=... down=...`。

`tests/device_fortune.py` 是明确选择设备后运行的真机串口巡检。默认会写入测试生辰并在正常结束时清回示例，因此默认只允许在尚无个人档案的设备上运行；已有档案时使用 `--keep-profile`，跳过档案、时间的写入与软件重启测试。它检查串口注入操作，不代替实体按键、屏幕和喇叭验收；检测到实体输入会停止，以免把人工操作混进自动结果。测试过程中先不操作实体按键。

当前验收状态与镜像 SHA256 以 `../validation/M4-device.md`、`../validation/M4-device.json` 为准。构建通过或串口能运行都不等于已经通过发布验收。

本项目与 FoloToy 官方无关联；BSP 保留 LICENSE.FoloToy。NABO 形象和美术素材不随源码的 MIT 许可授权。
