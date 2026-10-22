# 测试

以下逻辑和渲染测试在宿主机运行，不碰设备。需要 gcc（或用 `CC=clang` 指定）和 Python 3；末尾的 M4 串口测试另需明确指定设备。

```sh
python tests/test_fortune.py            # 逐项对拍 tests/fortune_host/golden/*.json
python tests/test_fortune.py --mutate   # 变异测试：19 处故意改坏的规则必须全部被抓到
gcc -std=c99 -fsanitize=address,undefined tests/fortune_host/sweep.c firmware/main/fortune/*.c -o sweep && ./sweep
```

重新生成表和标准答案（需要 Node.js，先 `npm install`）：

```sh
npm run tables    # tools/gen_fortune_tables.js → firmware/main/fortune/fortune_tables.c
npm run golden    # tests/fortune_host/record_golden.js → tests/fortune_host/golden/
```

改规则的顺序：先改 `prototype/fortune-core.js`，重新录标准答案，再改 C，最后跑对拍。

## 界面渲染（M3）

`fortune_host/` 里把固件 UI（`firmware/main/fortune/gfx`、`ui`）编成共享库，在电脑上跑全部场景：

```sh
sh tests/fortune_host/build.sh                                                        # ASan + UBSan
sh tests/fortune_host/run.sh firmware/assets/fortune_assets.bin out run_all.py        # 全场景，逐帧校验 增量合成 == 整屏重画
sh tests/fortune_host/run.sh firmware/assets/fortune_assets.bin out run_cmp.py        # 与原型同状态的固定截图
python tests/fortune_host/check_glyphs.py firmware/assets/fortune_assets.bin firmware/assets/assets_manifest.json firmware/main/fortune
```

改了原型文案后先跑 `node tools/gen_fortune_text.js prototype/index.html firmware/main/fortune/fortune_text.c`，再重跑 `tools/gen_fonts_fortune.py` 和 `tools/pack_assets_fortune.py`。

## M4：Windows 与真机

根据真机反馈，正文已放大为 16 px、提示至少 14 px，解读改为翻页；新图见 `validation/m4-v012/`。

```powershell
./tests/fortune_host/build_windows.ps1 -Compiler gcc
python tests/fortune_host/review_readable.py firmware/assets/fortune_assets.bin validation/m4-v012
python tests/fortune_host/run_all.py firmware/assets/fortune_assets.bin validation/m4-v012/all
python tests/test_m4_host.py
python tests/fortune_host/test_asset_lifetime.py firmware/assets/fortune_assets.bin validation/m4-v012/asset-lifetime.json
```

Windows 原生构建不带 ASan/UBSan。`test_m4_host.py` 检查 11 种合成音效的幅度、收尾和开机语音 PCM，以及打包检查器对缺素材、错误偏移、损坏头、合并字节不一致和截断镜像的拒绝。数字波形不削顶不能证明实体喇叭不破音。

以下命令只针对已经识别的测试设备；已有个人档案必须用 `--keep-profile`。它保留档案和时间，自动走八个玩法、翻页及十轮十二场景进出；巡检期间不要操作实体按键。

```powershell
python tests/device_fortune.py --port '<PORT>' --expected-mac '<MAC>' --keep-profile --out private/m4-readable-device
python tests/observe_fortune_sleep.py --port '<PORT>' --expected-mac '<MAC>' --out private/m4-readable-sleep
```

休眠观察器发送一次状态查询后保持静默，等待真实五分钟空闲，再等手动上键唤醒。它不缩短设备休眠阈值。原始日志和个人数据只存 `private/`；验收结论见 `validation/M4-device.md`。
