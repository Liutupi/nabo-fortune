# 刷机与升级

适用：FoloToy AI Passport，ESP32-C3，8 MB Flash。先核对设备 MAC。以下命令在解压后的固件 ZIP 目录运行；源码仓库内使用 tools/flash_release.py 时需加 `--release-dir <解压目录>`。

## 发行文件

- `nabo-fortune-v0.1.2-full.bin`：完整合并镜像，地址 0x0。
- `nabo-fortune-v0.1.2.zip`：四个构建镜像、完整镜像、manifest、刷写脚本和许可证。
- `SHA256SUMS.txt`：七个发行文件的 SHA256；ZIP 内另有 bundle-SHA256SUMS.txt。

首次换成独立算命机将覆盖原固件与用户数据。先完整备份；不要把设备读回的整片 Flash 分享为固件。

## 命令行

安装 Python 与 esptool 4.x（本次验证为 4.12.0）：

```text
python -m pip install "esptool>=4.12,<5"
python flash_release.py --mode inspect --port <PORT> --expected-mac <MAC>
```

完整安装：

```text
python flash_release.py --mode install --port <PORT> --expected-mac <MAC> --confirm-full-install
```

已有相同分区布局的算命机，保留出生资料升级：

```text
python flash_release.py --mode upgrade --port <PORT> --expected-mac <MAC>
```

把 PORT 和 MAC 换成自己的设备。写入前脚本校验镜像、实际 MAC、8 MB 容量和安全状态，完整备份并验证原 Flash，升级时再核对分区表。升级写 app 和 assets，保留 NVS；安装先擦除整片。写后进行摘要校验。备份只保存在本机 private/backups。

## 网页本地安装

使用支持 AI Passport ESP32-C3 的网页刷机工具，选择“本地固件刷机”，加载完整镜像，从 **0x0** 进行完整安装。网页完整安装不保留旧档案；希望保留现有算命机档案时使用上述 upgrade 命令。

## 分区

| 项目 | 地址 | 容量 |
|---|---|---|
| bootloader | 0x0 | 到 0x8000 |
| partition table | 0x8000 | 0x1000 |
| NVS | 0x9000 | 0x6000 |
| PHY | 0xf000 | 0x1000 |
| app | 0x10000 | 0x1f0000 |
| assets | 0x200000 | 0x600000 |

首次开机校准时间并设置自己的出生资料。断电重开后需要重新校准时间；生辰档案保留。测试设备的个人档案与日志不在发行包内。
