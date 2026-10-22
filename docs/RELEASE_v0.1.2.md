# v0.1.2 首发说明

首个公开版本沿用用户已验收的程序版本 0.1.2。下载：https://github.com/Liutupi/nabo-fortune/releases/tag/v0.1.2

八个离线玩法，22 张原创 NABO 塔罗，开机语音，大字号与完整解读翻页。包含源码、原始美术、Grok v1/v2 视频、字体与构建工具。

首次安装选择 `nabo-fortune-v0.1.2-full.bin`，地址 0x0，先备份再完整安装；会替换旧固件和数据。已有相同分区布局的算命机请解压 ZIP，用 flash_release.py 的 upgrade 模式保留出生资料。步骤见 FLASHING.md。

所有固件文件来自构建输出。发布前已核对四镜像的实际设备摘要，并补验证本次构建 bootloader 启动；程序与素材沿用已接受的字节。私有设备备份、NVS、出生资料和原始日志不发布。

验证范围：八玩法自动流程、十二场景十轮进出、7831 帧宿主一致性、22 牌解码与长名资源反复读取、真实断电档案保留、五分钟熄屏及上键恢复。用户接受塔罗、称骨、声音、字号复测。详细范围及串口回执重试等限制见 validation/M4-device.md。

下载后先核对 SHA256SUMS.txt；ZIP 内所有成员另有 bundle-SHA256SUMS.txt。manifest.json 绑定源码提交、四个镜像及合并镜像的 SHA256。

源码 MIT；NABO 美术与音视频不随 MIT 授权；字体与第三方程序保留原许可。本项目与 FoloToy 官方无关联。
