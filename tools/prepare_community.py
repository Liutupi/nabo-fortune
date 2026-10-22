"""Compose community gallery from the accepted v0.1.2 C-renderer screenshots."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "validation/m4-v012"
OUT = ROOT / "docs/community/v0.1.2"
FONT = ROOT / "tools/fonts/SourceHanSansCN-Medium.otf"
BG, PANEL = "#0D0820", "#19112F"
INK, MUTED, CYAN, GOLD = "#F5F0FF", "#BDB2D3", "#35F0DC", "#F2C45B"

def font(size):
    return ImageFont.truetype(str(FONT), size)

def text(draw, xy, value, size=30, color=INK):
    draw.text(xy, value, font=font(size), fill=color)

def base(title, subtitle, number):
    im = Image.new("RGB", (1200, 1600), BG)
    d = ImageDraw.Draw(im)
    d.line((72, 32, 1128, 32), fill=CYAN, width=4)
    text(d, (72, 56), title, 48)
    text(d, (72, 126), subtitle, 27, MUTED)
    text(d, (72, 1554), "固件渲染预览 · 原始屏幕 240×320 · 仅供娱乐", 23, MUTED)
    text(d, (1050, 1554), f"{number}/5", 23, CYAN)
    return im, d

def screen(im, d, filename, xy, label):
    x, y = xy
    image = Image.open(SRC / filename).convert("RGB")
    assert image.size == (240, 320), (filename, image.size)
    d.rounded_rectangle((x-8, y-8, x+488, y+648), radius=14, fill=PANEL,
                        outline="#43305E", width=2)
    im.paste(image.resize((480, 640), Image.Resampling.NEAREST), (x, y))
    text(d, (x, y-44), label, 27, GOLD)

def four(title, subtitle, number, items, filename):
    im, d = base(title, subtitle, number)
    for (image, label), xy in zip(items, [(80, 218), (640, 218), (80, 900), (640, 900)]):
        screen(im, d, image, xy, label)
    im.save(OUT / filename, optimize=True)

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    # The boot screen is the hero. Preserve the real UI and NABO welcome pose;
    # place brief gallery captions only in the screen's empty top/bottom areas.
    im = Image.open(SRC / "boot.png").convert("RGB")
    assert im.size == (240, 320)
    im = im.resize((1200, 1600), Image.Resampling.NEAREST)
    d = ImageDraw.Draw(im)
    text(d, (90, 72), "AI PASSPORT  /  独立离线固件", 34, CYAN)
    d.rounded_rectangle((910, 64, 1114, 126), radius=20, fill=PANEL,
                        outline="#43305E", width=2)
    text(d, (944, 75), "v0.1.2", 30, GOLD)
    text(d, (190, 1350), "“NABO，在此恭候。”", 54, INK)
    text(d, (190, 1440), "八种离线玩法  ·  22 张原创塔罗", 36, GOLD)
    text(d, (90, 1540), "开机画面 · 固件渲染预览 · 仅供娱乐", 26, MUTED)
    text(d, (1050, 1540), "1/5", 26, CYAN)
    im.save(OUT / "01-cover.png", optimize=True)

    four("掷铜钱，摇灵签", "按住 OK，再松开；结果可用上下键翻页。", 2, [
        ("all/shake_2toss.png", "三枚铜钱，六次成卦"),
        ("all/shake_6card.png", "成卦后查看解读"),
        ("lot.png", "卷轴展开，读签诗"),
        ("lot_page2.png", "翻页查看白话与签象"),
    ], "02-coins-and-lot.png")

    im, d = base("22 张原创 NABO 塔罗", "洗牌 → 选牌 → 抽出 → 翻面 → 正逆位结果", 3)
    screen(im, d, "tarot_fan.png", (80, 280), "上下选择，OK 抽出")
    screen(im, d, "tarot.png", (640, 280), "放大牌面，保留到按 OK")
    text(d, (80, 1010), "让 NABO 扮演每一张大阿卡纳", 44, CYAN)
    text(d, (80, 1110), "愚者、魔术师、女祭司……直到世界。", 33)
    text(d, (80, 1180), "每张都有独立原创牌面，包含正位与逆位。", 33)
    text(d, (80, 1300), "抽牌和翻面留出停顿，让牌面看得清。", 32, GOLD)
    text(d, (80, 1370), "按 OK 重新洗牌，双击上键回主转盘。", 31, MUTED)
    im.save(OUT / "03-tarot.png", optimize=True)

    four("称骨看得见，解读翻得全", "年月日时化作四块砝码；称完停住，按 OK 看解读。", 4, [
        ("bone_balance.png", "杆秤称重，停留看结果"),
        ("bone_white.png", "第 1 页 · 白话解读"),
        ("bone_poem.png", "第 2 页 · 称骨原文"),
        ("bone_parts.png", "第 3 页 · 骨重明细"),
    ], "04-bone-reading.png")

    four("排盘、连星与今日小票", "紫微十二宫、奇门九宫、星座连线，都在这块小屏里。", 5, [
        ("ziwei_chart.png", "紫微斗数 · 十二宫"),
        ("qimen_chart.png", "奇门遁甲 · 九宫盘"),
        ("star.png", "星座 · 连星与幸运提示"),
        ("daily.png", "今日一卦 · 汇总小票"),
    ], "05-charts-and-stars.png")
    for p in sorted(OUT.glob("*.png")):
        with Image.open(p) as check:
            assert check.size == (1200, 1600)
        print(f"{p.name}: {p.stat().st_size} bytes")

if __name__ == "__main__":
    main()
