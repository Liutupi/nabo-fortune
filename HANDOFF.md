# 交接本：NABO 赛博算命机

更新：2026-10-22。首个公开版本为 **v0.1.2**，设备程序版本同为 0.1.2。

- 仓库：https://github.com/Liutupi/nabo-fortune
- 固件：https://github.com/Liutupi/nabo-fortune/releases/tag/v0.1.2
- 安装：docs/FLASHING.md
- 构建：firmware/README.md
- 验收：validation/M4-device.md、M4-device.json、M4-user-feedback.json
- 构建哈希及保存的源码快照：validation/m4-build.json
- 四个构建镜像的设备校验：validation/RELEASE-v0.1.2.json

## 完成边界

M0–M4 完成。用户对 0.1.2 的塔罗牌面与节奏、称骨动画与解读、新声音和字号回复“可以了”。已验证实际断电保留出生资料，以及空闲 300163 ms 后休眠、上键恢复、uptime 重新计时、时钟推进与档案一致。

早期复位原因日志未捕获，Windows COM12 在休眠期间仍被枚举；没有将 reset reason 8 或实际休眠电流写成已测项目。休眠流程中的已停止 I2S 重复 disable 提示按 BSP 的幂等停止行为记录。自动串口巡检遇到一次回执缺失后重发成功，不能声称 USB 通信完全没有丢失。

设备最初按用户“全覆盖”授权安装前已完整备份并验证原 Flash。后续修改保留 NVS。首次发布前补刷本次构建的 bootloader，四镜像摘要均与构建输出一致，进入 0.1.2 主转盘，出生资料保留。备份、个人数据和原始日志只在 Git 忽略的 private/，不上传。

## 关键修复

- 0.1.0 被指出声音太小且破音、字号太小、动画卡顿、称骨解释不完整，未验收。
- 0.1.1 放大字号后，用户发现牌面和杆秤部件消失。复现为长牌名被截短，重复请求占满 40 个句柄；0.1.2 保留完整资源键并增加稳定句柄容量。
- 长名牌面重复 200 次、全部道具 147 次读取、22 牌非透明解码均通过。专项 3718 帧、全场景 4113 帧，增量/整屏零差异；2009 字无缺字。
- 正文 16 px、提示至少 14 px。称骨九档白话和 52 档原诗能完整显示，长解读按页阅读。
- 塔罗牌面 112×184，抽出 1.2 秒、停顿 0.6 秒、翻面 1.4 秒，结果保留。称骨约 9 秒称完，保留杆秤，按 OK 再进三页解读。
- 开机语音“NABO，在此恭候”已生成、处理、打包；来源见 game-art/fortune/audio/generation.json，不要重复付费生成。
- 旋转绘制改用等价 32 位运算；过载帧主动让出 CPU，修复看门狗风险。音频任务优先级高于绘制，音效加入柔和起音。

## 修改约定

- 用户已将工程接手与发布交给 Codex，见 AGENTS.md。
- 保留 NABO 身份与两阶平涂方案，Grok v1 与 v2 原片均保留。
- 改 UI 先同步 prototype/index.html，再生成 fortune_text.c、字体、素材包；勿手改 fortune_text.c / fortune_tables.c。
- 变更程序或素材后重新绑定构建哈希、真机验收范围。不能把旧镜像的验收挪到新镜像上。
- 设备上已有个人档案时，巡检使用 --keep-profile；不要调用 pf clear。
- 八玩法自动巡检、十二场景十轮切换无异常重启。最低 heap 88144、largest 24576 字节；摇签仍有慢帧，详见验收表。
- 原生 Windows 宿主测试没有 ASan/UBSan；串口注入按键不能替代实体操作验证。

## 构建与发行

```powershell
python tools/pack_assets_fortune.py
./firmware/tools/build.ps1 -Action all
./tests/fortune_host/build_windows.ps1 -Compiler gcc
python tests/fortune_host/run_all.py firmware/assets/fortune_assets.bin validation/m4-v012/all
python tests/fortune_host/review_readable.py firmware/assets/fortune_assets.bin validation/m4-v012
python tests/fortune_host/test_asset_lifetime.py firmware/assets/fortune_assets.bin validation/m4-v012/asset-lifetime.json
python tools/package_release.py --version 0.1.2
```

打包必须在源码提交后执行，只取构建输出，不能从设备备份做发行镜像。manifest 绑定源码提交，SHA256SUMS 覆盖七个附件；ZIP 内另有逐成员校验。发布后必须匿名下载所有附件（包括 SHA256SUMS.txt）并核对，标签应指向 manifest 的源码提交。

程序源码 MIT；NABO 美术与音视频不随 MIT 授权。字体和第三方代码保留原许可。后续新功能继续按 DESIGN 的阶段流程推进。
