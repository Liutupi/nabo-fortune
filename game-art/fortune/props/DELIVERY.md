# 第 3 轮静态道具交付

交付日期：2026-10-22。已交齐 ART_BRIEF 第 6 节的 29 个道具 PNG，每项包含原尺寸与 4 倍版。造型沿用已选定的 C 方案，新增素材采用平涂、硬边和统一调色板。

## 文件入口

- `asset-manifest.json`：29 项的明确路径、尺寸、SHA256、透明度及颜色数。路径相对 `game-art/fortune/`。
- `qa/props-native-overview.png`：全部道具在 #0D0820 底上的原尺寸总览。
- `qa/props-detail-check.png`：印章、砝码、签筒等细节放大检查。
- `originals/`：11 张新生成的原始设计大图和各自提示词记录。
- `export-static-assets.ps1`：从大图制造原尺寸与 4 倍版的导出脚本；另负责准确字形和几何符号。
- `round34-palette.json`：新增素材的共享平涂颜色。
- `CLAUDE_HANDOFF.md`：程序接入说明。

## 内容

| 类别 | ID | 原尺寸 |
|---|---|---|
| 铜钱 | coin-front / coin-back | 22×22 |
| 龟甲 | shell | 68×40 |
| 签筒 / 签子 | tube / stick | 64×110 / 6×80 |
| 卷轴 | scroll | 208×216 |
| 印章 | seal-shangshang / seal-shang / seal-zhong / seal-xia | 各 52×40 |
| 杆秤 | scale-beam / scale-pan / scale-weight | 182×6 / 56×16 / 18×16 |
| 砝码 | weight-nian / weight-yue / weight-ri / weight-shi | 各 12×12 |
| 打印机 | printer | 212×28 |
| 牌背 | card-back-s / card-back-l | 44×72 / 82×135 |
| 标题 | title | 200×70 |
| 八卦 | trigram-0 … trigram-7 | 各 20×14 |

4 倍版命名为 `<id>-4x.png`。铜钱两面均有透明方孔；签筒为平视正面，倾角帧交给引擎生成；卷轴正文安全区为 `[30,36,148,144]`，数组含义为 `[x,y,width,height]`。砝码使用可辨认的像素字形“年、月、日、时”；印章为“上上、上、中、下”；标题为两行“NABO / 赛博算命机”。

卦符按 `乾、兑、离、震、巽、坎、艮、坤` 对应 0–7；由下爻至上爻的阳爻 bit mask 为 `7,3,5,1,6,2,4,0`。不要将文件序号直接当作 bit mask。

龟甲复用已经定稿的 `shell.png` 和 `shell-4x.png`，保持与既有 Grok 起始图中的道具一致，原始大图和旧调色板仍在。印章、砝码、两种牌背共享各自底图，属于同一设计的派生尺寸或字面。

## 检查与界限

尺寸、4 倍关系、可见像素、透明方孔、卦符拓扑和受保护输入文件哈希均已通过检查。新增道具 alpha 为 0 或 255；原有龟甲保留定稿的边缘处理。原尺寸和 4 倍版的单图不透明颜色数均小于 64。已经在 #0D0820 底上查看原尺寸和细节图。

这些是完成的静态 PNG，尚需 Claude 接入程序、生成索引精灵包并验证 RGB565 实机效果。PNG 的颜色数量检查不代替索引 0 透明、逐帧解码回比或设备渲染检查。本轮未修改原型、工具、固件及四份规范，也未修改 Grok 视频。

按项目 AGENTS.md：本项目与 FoloToy 官方无关联；NABO 形象和美术素材不随源码的 MIT 许可授权。
