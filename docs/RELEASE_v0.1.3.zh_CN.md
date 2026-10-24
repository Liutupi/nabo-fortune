<p align="right"><strong>简体中文</strong> · <a href="RELEASE_v0.1.3.md">English</a></p>

# NABO 赛博算命机 v0.1.3

[下载 v0.1.3](https://github.com/Liutupi/nabo-fortune/releases/tag/v0.1.3)。

“今日一卦”新增**今日双色球**，与原有命签小票并列。上下选择、OK 掐指和打印；结果页上下翻阅 6 红 1 蓝及生成依据。同一东八区日期、同一档案，重复查看或重启保持固定。输入综合生辰、农历干支、每日星座/塔罗/灵签/卦象小票、称骨、紫微及固定午时奇门。仅供娱乐，不预测开奖，也不提高中奖概率。详见 [v1 规则](DAILY_SSQ.zh_CN.md)。

八卦转盘、档案格式和 8 MB 分区不变。已有兼容安装可使用 ZIP 的 `upgrade` 模式保留 NVS。**0x0** 合并镜像可能重置 NVS/PHY，不能在该地址写仅应用镜像。备份为可选项，安装工具不执行整片擦除。详见[刷机说明](FLASHING.zh_CN.md)。

## 验证与确切镜像

宿主检查覆盖 1,795 组 JS/C 双色球对照、19 项变异检查、10,741 帧增量/整屏零差异、中文字形、离线浏览器键盘/跨日/刷新、M4 音频及错误布局拒绝，以及六项安装工具测试。macOS 渲染检查启用 ASan/UBSan，不声称执行过 Windows 原生 sanitizer。

开发镜像证据位于 validation/SSQ-v1-device.json：0.1.3-dev 已刷写并读回校验，NVS 逐字节不变；双色球、原小票及重启自动流程通过，用户确认实屏号码、依据翻页及三个实体键正常。巡检采样最低空闲堆 97476 字节、最大连续块最低 73728 字节。开发完整镜像摘要为 `03013c4035fcbb3bd6822b0730af4c347563105dc6c14db1d50341dcd19838e4`。

正式 v0.1.3 镜像及独立设备检查由 validation/v0.1.3-build.json、validation/v0.1.3-device.json、validation/RELEASE-v0.1.3.json 绑定。这些记录明确区分正式镜像检查和开发镜像实屏验收，不把 v0.1.2 或开发版硬件结论挪给另一份二进制。

本次未重新测实体音质、五分钟休眠唤醒或休眠电流。v0.1.2 的历史基线和摇签少量慢帧仍单独保留。打开 USB 监视器可能引起额外 USB 复位；保持连接的软件重启已验证保留校准时间。

## 构建、附件与许可

激活 ESP-IDF 5.5.3，安装 package.json 开发依赖及宿主 Python 渲染库，运行 `./tools/validate.sh`。可用 HOST_PYTHON 指定独立于 IDF 的解释器；macOS Framework Python 需真实 Python.app 可执行文件以预加载 ASan。目标仍为 ESP32-C3 / 8 MB / 无 PSRAM，全部离线。

发行包含完整合并镜像、四个分段镜像、ZIP、manifest.json 和 SHA256SUMS.txt。manifest 绑定标签源码提交和镜像摘要。下载后核对 SHA256SUMS.txt 以及 ZIP 内逐成员 bundle-SHA256SUMS.txt。全部固件来自源码构建，不使用设备备份或个人 NVS 作为发行物；匹配 ELF/MAP 留在本机调试归档中。

自有程序源码 MIT；NABO 美术、视频和音频不随 MIT 授权；第三方程序和字体保留原许可。本项目与 FoloToy 官方无关联。
