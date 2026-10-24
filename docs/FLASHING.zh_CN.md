<p align="right"><strong>简体中文</strong> · <a href="FLASHING.md">English</a></p>

# 刷机与升级

仅适用于 FoloToy AI Passport、ESP32-C3、8 MB Flash。写入前确认目标设备及 MAC。以下命令在发行 ZIP 解压目录运行；源码仓库使用时加 `--release-dir <解压目录>`。

## 核验下载

从 Release 下载 v0.1.3 ZIP 和 SHA256SUMS.txt，先核对 ZIP 摘要，解压后核对 bundle-SHA256SUMS.txt。manifest.json 绑定源码提交、四个分段镜像、合并镜像及匹配 ELF 的标识。包内不含用户 NVS、生辰或设备备份。

安装 esptool 4.x（已测试 4.12.0），检查已确认的目标：

```sh
python -m pip install 'esptool>=4.12,<5'
python flash_release.py --mode inspect --port <PORT> --expected-mac <MAC>
```

## 保留已有兼容安装的档案

```sh
python flash_release.py --mode upgrade --port <PORT> --expected-mac <MAC>
```

脚本核对镜像摘要、实际 MAC、ESP32-C3/8 MB、安全启动与 Flash 加密关闭状态，以及精确分区表。只写 0x10000 应用及 0x200000 素材，校验后重启，不写 NVS 或 PHY。布局不符会停止升级。

## 从其他分区安装

安装会替换程序与分区表，已有设置或数据可能不兼容或被覆盖，需要的记录请先另存。

```sh
python flash_release.py --mode install --port <PORT> --expected-mac <MAC> --confirm-full-install
```

安装按段写入已校验的 bootloader、分区表、应用和素材；两种模式都不会整片擦除，也不要求读回原固件。若明确需要私有整片备份，可额外指定 `--backup-dir <本机私有目录>`，不要上传备份。

## 合并镜像与网页刷机

`nabo-fortune-v0.1.3-full.bin` 为 **0x0** 合并镜像，会填充镜像间隙，可能重置 NVS/PHY。仅用于明确的完整安装；要保留档案请使用上述兼容分段升级。禁止将仅应用镜像写到 0x0。

| 区域 | 地址 | 容量 |
|---|---|---|
| Bootloader | 0x0 | 到 0x8000 |
| 分区表 | 0x8000 | 0x1000 |
| NVS | 0x9000 | 0x6000 |
| PHY | 0xf000 | 0x1000 |
| 应用 | 0x10000 | 0x1f0000 |
| 素材 | 0x200000 | 0x600000 |

不能套用小智版或单词护照的素材偏移。刷后检查启动，冷开机后校准东八区时间并确认生辰档案。“今日双色球”位于“今日一卦”中。写入成功不等于已经验证实体显示、按键、音质或休眠电流。某些 USB 串口监视器打开/重开时会触发额外 USB 复位，随后出现预期的时间校准提示。
