# AGENTS.md · nabo-fortune（NABO 赛博算命机）

FoloToy AI Passport 工牌（ESP32-C3，240×320 竖屏）上的离线算命机。整机只装这一个程序。

## 分工

| 谁 | 负责 | 入口文档 |
|---|---|---|
| **Codex** | 美术、接手 M4 固件与测试、GitHub 发布 | `HANDOFF.md`、`game-art/fortune/CODEX_PROMPTS.md` |
| **Grok** | NABO 半身表演视频（图生视频） | `game-art/fortune/GROK_PROMPTS.md` |
| **Claude** | 设计、原型、排盘规则、抽帧打包、渲染、固件、测试 | `game-art/fortune/DESIGN.md` |

开工前先读 `game-art/fortune/ART_BRIEF.md`，这是美术的唯一依据。

## 目录

```
game-art/fortune/
  DESIGN.md  ART_BRIEF.md  CODEX_PROMPTS.md  GROK_PROMPTS.md
  reference/      nabo-idle.png（NABO 身份基准，只读）
  costume/        造型方案 A/B/C 和定稿 final.png
  grok-start/     Grok 起始图，文件名 = 片段 ID
  grok-clips/     Grok 原始视频，文件名 = 片段 ID.mp4
  props/          道具
  tarot/          00-fool.png … 21-world.png
  anchors.json    锚点坐标
prototype/        浏览器原型（动画节奏和规则的标准答案）
tools/            抽帧、打包、校验脚本（Claude）
firmware/         独立固件（Claude）
```

## 规则

- Grok 只交付原始视频。2026-10-22 用户按 HANDOFF 明确将 M4 工程与后续仓库发布交给 Codex；Codex 可维护原型、工具、固件、测试和文档，UI 改动保持原型与固件一致。
- NABO 身份以 `reference/nabo-idle.png` 为准：脸、眼镜、发型、白帽和帽标、比例都不能改；帽标不能镜像。
- 文件名严格按 ART_BRIEF 的 ID 命名，打包脚本靠文件名对号入座。
- 原始大图和视频都要保留，不要只交缩小版。
- 塔罗牌面全部原创，不参照任何现有牌组。
- 本项目与 FoloToy 官方无关联。NABO 形象和美术素材不随源码的 MIT 许可授权。
