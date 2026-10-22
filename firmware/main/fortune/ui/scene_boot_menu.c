/* 开机、主转盘（原型 S.boot / S.menu） */
#include "fc_ui.h"
#include <string.h>

/* 弧度 → 1/1024 圈：×163/1000 */
#define RAD_M(mrad) ((int)((int64_t)(mrad) * 163 / 1000))

const uint8_t MENU_BITS[8][3] = {{1, 1, 1}, {1, 1, 0}, {1, 0, 1}, {1, 0, 0}, {0, 1, 1}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
const char *const MENU_ID[8] = {"daily", "shake", "lot", "tarot", "bone", "star", "ziwei", "qimen"};
static const char *const MENU_N[8] = {"今日一卦", "摇卦问事", "灵签筒", "塔罗", "称骨论命", "星座", "紫微斗数", "奇门遁甲"};
static const char *const MENU_D[8] = {"星座·塔罗·灵签合成小票", "三枚铜钱，六次成卦", "长按摇签，抽出灵签", "洗牌、选牌、翻开解读",
                                      "杆秤称出你的骨重", "连星成座，今日星级", "十二宫排盘，星曜入宫", "洛书飞宫，三层转盘"};

/* ---------------- 开机 ---------------- */
static void boot_enter(void) { fc_sfx_welcome(); }
static void boot_key(int k, bool d) {
  if (!d || EL() <= 500) return;
  if (k == K_OK) { fc_sfx_play(SFX_ENTER); fc_ui_go(g_time_unknown ? "clock" : "menu"); }
  if (k == K_DOWN) { fc_sfx_play(SFX_ENTER); fc_ui_go("clock"); }
}
static void boot_draw(void) {
  int t = EL();
  if (t < 450) {
    fc_rect(0, 0, FC_W, FC_H, C_BLACK, 255);
    if (t < 250) { int w = FC_W * ease_out(fx_prog(t, 250)) / FX_ONE; fc_rect(120 - w / 2, 159, w, 2, 0xFFFF, 255); }
    else { int h = FC_H * ease_out(fx_prog(t - 250, 200)) / FX_ONE; fc_rect(0, 160 - h / 2, FC_W, h, 0xFFFF, 255); }
    return;
  }
  int rp = ease_back(fx_prog(t - 500, 700));
  if (rp > 0) {
    int rot = RAD_M((FX_ONE - rp) * 2) + (int)((int64_t)VT * 163 / 6000);
    int th = fx_clamp(15 * rp / 10000, 1, 3);
    fc_ring(120, 120, 70 * rp / FX_ONE, th, C_MAG);
    fc_ring(120, 120, 56 * rp / FX_ONE, th, C_CYAN);
    int w = 16 * rp / FX_ONE, gap = 4 * rp / FX_ONE, tth = fx_clamp(2 * rp / FX_ONE, 1, 3);
    if (w >= 2)
      for (int i = 0; i < 8; i++) tri3_rot(120, 120, 63 * rp / FX_ONE, rot + i * 128, MENU_BITS[i], w, C_GOLD, gap, tth);
  }
  static const int NT = 9;                 /* "NABO赛博算命机" 9 个字 */
  int shown = fx_clamp((t - 600) / 90, 0, NT);
  if (t >= 600 && shown > 0) {
    int k = ((t - 600) % 90 < 40 && shown < NT) ? 1 : 0;
    const fc_img_t *ti = prop("title");
    if (ti) {
      fc_gfx_clip(20, 190, 200 * shown / NT, 70);
      if (k) { fc_img(ti, 0, 18, 190, 140); fc_img(ti, 0, 22, 190, 140); }
      fc_img(ti, 0, 20, 190, 255);
      fc_gfx_clip(0, 0, 0, 0);
    }
  }
  if (t > 1400) { int p = ease_out(fx_prog(t - 1400, 500)); nabo_then("N-WELCOME", 120, 120, fx_lerp(380, 124, p), t - 1400, "N-IDLE"); }
  if (t > 1900 && (VT / 500) % 2) T(&F_SANS11, g_time_unknown ? "按 OK 校准时间" : "按 OK 开始", 120, 292, C_CYAN, AL_C);
  if (t > 1900) T(&F_SANS9, g_time_unknown ? "断电后时间已重置" : "↓ 校准时间", 120, 310, C_HINT, AL_C);
  if (t < 620) fc_rect(0, 0, FC_W, FC_H, 0xFFFF, (uint8_t)(255 - 255 * (t - 450) / 170));
}
const fc_scene_t SC_BOOT = {"boot", "开机", false, boot_enter, boot_key, NULL, boot_draw};

/* ---------------- 主转盘 ---------------- */
static int m_sel;
static int m_ang;        /* 1/16384 圈 */
static void menu_key(int k, bool d) {
  if (!d) return;
  if (k == K_DOWN) { m_sel = (m_sel + 1) % 8; fc_sfx_play(SFX_TICK); }
  if (k == K_UP) { m_sel = (m_sel + 7) % 8; fc_sfx_play(SFX_TICK); }
  if (k == K_OK) { fc_sfx_play(SFX_ENTER); fc_ui_go(MENU_ID[m_sel]); }
}
static void menu_update(int dt) {
  int d = -m_sel * 2048 - m_ang;
  while (d > 8192) d -= 16384;
  while (d < -8192) d += 16384;
  m_ang += d * fx_clamp(dt * 13, 0, 1000) / 1000;
}
static void menu_draw(void) {
  const int cx = 120, cy = 136, Rr = 86;
  int ang = m_ang >> 4;
  fc_ring(cx, cy, Rr, 28, C_BG2);
  fc_ring(cx, cy, Rr + 14, 1, C_MAG);
  fc_ring(cx, cy, Rr - 14, 1, C_CYAN);
  for (int i = 0; i < 64; i++) {
    int a = ang + i * 16;
    fc_rect(cx + (fx_cos(a) * (Rr + 18) >> 14) - 1, cy + (fx_sin(a) * (Rr + 18) >> 14) - 1, 2, 2, i % 8 ? C_DIM : C_MAG, 255);
  }
  for (int i = 0; i < 8; i++) {
    int a = ang + i * 128 - 256, x = cx + (fx_cos(a) * Rr >> 14), y = cy + (fx_sin(a) * Rr >> 14);
    bool on = i == m_sel;
    char nm[16]; nm[0] = 0; strcpy(nm, "trigram-0"); nm[8] = (char)('0' + i);
    const fc_img_t *ti = prop(nm);
    int w = on ? 20 : 14, h = on ? 14 : 10;
    if (ti) {
      if (on) fc_img(ti, 0, x - w / 2, y - h / 2, 255);
      else fc_img_aff(ti, 0, x, y, ti->w / 2, ti->h / 2, 0, w * 256 / ti->w, h * 256 / ti->h, 115);
    } else tri3(x, y, MENU_BITS[i], w, on ? C_GOLD : C_DIM, on ? 5 : 4, on ? 3 : 2);
  }
  fc_tri(cx, cy - Rr - 20, cx - 6, cy - Rr - 30, cx + 6, cy - Rr - 30, C_GOLD);
  fc_taiji(cx, cy, 52, (int)((int64_t)VT * 163 / 2400), RGB(0x24, 0x1A, 0x4C), RGB(0x17, 0x0F, 0x33));
  nabo("N-IDLE", 76, cx, cy - 2, (int)VT);
  G(&F_DISP24, MENU_N[m_sel], 120, 256, C_WHITE, C_MAG);
  T(&F_SANS11, MENU_D[m_sel], 120, 282, C_DIM, AL_C);
  T(&F_SANS9, "↑↓ 转盘   OK 进入", 120, 306, C_HINT, AL_C);
}
static void menu_enter(void) {}
const fc_scene_t SC_MENU = {"menu", "主转盘", false, menu_enter, menu_key, menu_update, menu_draw};
