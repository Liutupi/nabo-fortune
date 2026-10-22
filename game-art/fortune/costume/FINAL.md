# C 造型定稿

2026-10-22：用户明确选 C，并授权定稿成两阶平涂、继续制作 Grok 起始图。

- `final.png`：1024×1024，#0D0820 纯色背景，锁定的 C 造型。
- `final-transparent.png`：同一造型的透明母版。
- `final-120.png`、`final-100.png`、`final-76.png`、`final-56.png`：目标尺寸预览。
- `final-original.png`：1254×1254 AI 重绘原始结果，未覆盖。
- `C-flat-draft-original.png`：第一次平涂重绘草稿，保留。
- `final-palette.json`：19 色人物用色；每种主要材质为底色和硬边阴影，另有描边、帽标、虹膜等必要颜色。

使用内置 image_gen 收敛造型，再以固定用色导出，清除原图中残余的柔和明暗。罩衫采用不透明浅紫色块；不做实际透视透明。内部为离散实色色块，边缘保留必要抗锯齿；接近不透明的源 alpha 统一为 255。原始 NABO 身份图片只读。

Grok 起始图和本轮完整交付见 `../grok-start/DELIVERY.md`。原始提示词见该目录 `generation-prompts.json`。本次未生成视频，不代表已通过动画循环、抠图、64 色打包或设备验证。
