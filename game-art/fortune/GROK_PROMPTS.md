# NABO 赛博算命机 · Grok 图生视频提示词

**用法**：输入图是 Codex 第 2 轮出的 `grok-start/<ID>.png`；导出的视频命名为 `grok-clips/<ID>.mp4`。
每段视频都附上下面这段通用约束（英文给 Grok 用，中文是说明）：

> Locked static camera, no zoom, no pan, no camera shake. Half-body front view. Keep the character's face, glasses, hairstyle, white cap and the "NABO" text on the cap exactly the same in every frame; never mirror the cap text. Flat cel-shaded animation style with dark outlines, no motion blur, no lighting changes. Plain solid background, unchanged throughout. No extra objects, no text, no particles.

> 中文说明：镜头完全固定；人物身份不能漂移；不要动态模糊；背景纯色不变；画面里不要出现多余物体和文字。

建议每段生成 2–3 个版本，选人物最稳的一版。时长比表格里多留 1 s 余量，抽帧时再裁剪。

---

| ID | 时长 | 英文提示词 |
|---|---|---|
| **N-IDLE** | 4 s | The character stands calmly, breathing gently with subtle shoulder movement, blinks twice, and once pushes up the glasses with one finger, then returns to the exact starting pose. Seamless loop: last frame matches first frame. |
| **N-WELCOME** | 3 s | The character raises both hands and performs a traditional Chinese fist-and-palm salute (抱拳), bows slightly, then straightens up with a warm smile and lowers the hands back to the starting pose. |
| **N-SHELL** | 2.5 s | The character holds a turtle shell with both hands at chest level, eyes closed in concentration, and shakes it up and down rhythmically. Only the arms and shell move; the head stays steady. Seamless loop. |
| **N-SHELL-TOSS** | 1.5 s | Holding the turtle shell at chest level, the character flicks it upward and forward once in a quick motion, as if tossing coins out of it, then brings it back to chest level. Do not show any coins. |
| **N-TUBE** | 3 s | The character leans slightly forward, fists clenched at chest, eyes fixed on something off-screen at the lower right, bouncing a little with excited anticipation. Seamless loop. |
| **N-WOW** | 2 s | The character's eyes widen with delight, both hands fly up in celebration, then the hands come back down to the starting pose with a big smile. |
| **N-NOD** | 1.5 s | The character smiles and nods twice approvingly, then returns to the starting pose. |
| **N-SIGH** | 2 s | The character gives a soft, gentle sigh with a slight shrug, then smiles encouragingly and makes a small "you can do it" fist pump. Friendly and warm, not sad. Ends in the starting pose. |
| **N-CALC** | 3 s | The character's left hand is raised near the chin, the thumb tapping across the other fingertips one by one as if counting, eyes looking upward in thought, lips moving slightly as if muttering. Seamless loop. |
| **N-PRINT** | 2.5 s | The character looks down and points toward the bottom of the frame, then looks back up and gives a thumbs-up with a cheerful smile, returning to the starting pose. |

## 验收（抽帧后由引擎检查）

- 逐帧检查人物大小和位置：头部中心漂移不能超过 2 px（按 120 px 版计算）。
- 循环片段首尾帧的差异超过阈值就重做。
- 帽标字样清楚，没有变形或镜像。
- 背景纯色不变，抠图后没有残边。
