/* 塔罗（原型 S.tarot）、今日一卦小票（原型 S.daily） */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

#define MRAD(m) ((int)((int64_t)(m) * 163 / 1000))      /* 毫弧度 → 1/1024 圈 */

/* ================= 塔罗 ================= */
enum { T_SHUFFLE, T_FAN, T_FLY, T_POISE, T_FLIP, T_SHOW };
static int tph, tpt, tsel, tfl;
static int8_t hand[7];
static bool rev;
static const char *const TFILE[22] = {"00-fool", "01-magician", "02-high-priestess", "03-empress", "04-emperor", "05-hierophant", "06-lovers",
                                      "07-chariot", "08-strength", "09-hermit", "10-wheel-of-fortune", "11-justice", "12-hanged-man", "13-death",
                                      "14-temperance", "15-devil", "16-tower", "17-star", "18-moon", "19-sun", "20-judgement", "21-world"};
static void deal(void) {
  tph = T_SHUFFLE; tpt = (int)VT;
  fc_rng_t r = {fc_rng_u32(&g_rng)};
  fc_tarot_hand(&r, hand, &rev);
  tsel = 3; tfl = -1;
}
static void t_enter(void) { deal(); }
static void t_key(int k, bool d) {
  if (!d) return;
  if (tph == T_FAN && (int)VT - tpt > 1000) {
    if (k == K_UP && tsel > 0) { tsel--; fc_sfx_play(SFX_TICK); }
    if (k == K_DOWN && tsel < 6) { tsel++; fc_sfx_play(SFX_TICK); }
    if (k == K_OK) { tph = T_FLY; tpt = (int)VT; fc_sfx_play(SFX_WHOOSH); }
  } else if (tph == T_SHOW && (int)VT - tpt > 1500 && k == K_OK) deal();
}
static void t_update(int dt) {
  (void)dt;
  int t = (int)VT - tpt;
  if (tph == T_SHUFFLE) { if (t / 90 != tfl) { tfl = t / 90; fc_sfx_play(SFX_PRINT); } if (t > 2600) { tph = T_FAN; tpt = (int)VT; } }
  else if (tph == T_FLY && t > 1200) { tph = T_POISE; tpt = (int)VT; }
  else if (tph == T_POISE && t > 600) { tph = T_FLIP; tpt = (int)VT; }
  else if (tph == T_FLIP && t > 1400) { tph = T_SHOW; tpt = (int)VT; fc_sfx_play(SFX_REVEAL); burst(120, 130, 36, 0); }
}
static void fan_pos(int i, int *x, int *y, int *a_m) {
  int a = (i - 3) * 170, lift = i == tsel ? 16 : 0, A = MRAD(a);
  *x = 120 + (fx_sin(A) * (200 + lift) >> 14); *y = 470 - (fx_cos(A) * (200 + lift) >> 14); *a_m = a;
}
/* 牌：(x,y) 中心，w×h 目标尺寸，a 毫弧度，sx 千分比横向缩放 */
static void card(const fc_img_t *im, int x, int y, int w, int h, int a_m, int sx) {
  if (!im) return;
  int kx = w * 256 * sx / 1000 / im->w, ky = h * 256 / im->h;
  if (kx <= 0) kx = 1;
  fc_img_aff(im, 0, x, y, im->w / 2, im->h / 2, MRAD(a_m), kx, ky, 255);
}
static void back(int x, int y, int w, int h, int a_m, int sx) { card(prop(w <= 48 ? "card-back-s" : "card-back-l"), x, y, w, h, a_m, sx); }
static void front(int x, int y, int w, int h, int a_m, int sx, int idx) { char n[40]; snprintf(n, sizeof n, "tarot-%s", TFILE[idx]); card(prop(n), x, y, w, h, a_m, sx); }
static void t_draw(void) {
  int t = (int)VT - tpt;
  if (tph == T_SHUFFLE) {
    int p = t * 1000 / 2600, s = fx_sin(p * 1024 / 1000); if (s < 0) s = -s;
    int fade = p < 850 ? 1000 : (1000 - p) * 1000 / 150; if (fade < 0) fade = 0;
    for (int k = 0; k < 10; k++) {
      int side = k % 2 ? -1 : 1, off = (int)((int64_t)side * 42 * s * fade / 1000 >> 14);
      back(120 + off, 150 - k * 14 / 10, 44, 72, side * 50 * s >> 14, 1000);
    }
    T(&F_SANS11, "洗牌中……", 120, 236, C_DIM, AL_C);
  }
  if (tph == T_FAN) {
    for (int i = 0; i < 7; i++) {
      int x, y, a; fan_pos(i, &x, &y, &a);
      back(x, fx_lerp(380, y, ease_out(fx_prog(t - i * 90, 450))), 44, 72, a, 1000);
    }
    int x, y, a; fan_pos(tsel, &x, &y, &a);
    frame_rot(x, y, 46, 74, MRAD(a), 1, C_GOLD);
    G(&F_DISP24, "选一张牌", 120, 70, C_WHITE, C_MAG);
    T(&F_SANS11, "心里想着你的问题", 120, 98, C_DIM, AL_C);
    T(&F_SANS9, "↑↓ 选牌   OK 抽出", 120, 140, C_HINT, AL_C);
  }
  if (tph == T_FLY) {
    int x0, y0, a; fan_pos(tsel, &x0, &y0, &a);
    int p = ease_io(fx_prog(t, 1200));
    int x = fx_lerp(x0, 120, p), y = fx_lerp(y0, 136, p) - (fx_sin(p * 512 / 1000) * 30 >> 14);
    back(x, y, fx_lerp(44, 112, p), fx_lerp(72, 184, p), fx_lerp(a, 0, p), 1000);
  }
  if (tph == T_POISE) {
    back(120, 136, 112, 184, 0, 1000);
    T(&F_SANS11, "翻牌中…", 120, 262, C_DIM, AL_C);
  }
  if (tph == T_FLIP) {
    int p = fx_prog(t, 1400), c = fx_cos(p * 512 / 1000), sx = (c < 0 ? -c : c) * 1000 >> 14, rot = rev ? p * 3142 / 1000 : 0;
    if (p < 500) back(120, 136, 112, 184, rot, sx); else front(120, 136, 112, 184, rot, sx, hand[tsel]);
  }
  if (tph == T_SHOW) {
    int idx = hand[tsel];
    front(120, 136, 112, 184, rev ? 3142 : 0, 1000, idx);
    char b[64];
    snprintf(b, sizeof b, "%s · %s", FT_TAROT_NAME[idx], rev ? "逆位" : "正位");
    G(&F_DISP24, b, 120, 252, C_WHITE, rev ? C_CYAN : C_MAG);
    if (rev) snprintf(b, sizeof b, "留意：%s受阻", FT_TAROT_KEY[idx]); else snprintf(b, sizeof b, "关键词：%s", FT_TAROT_KEY[idx]);
    T(&F_SANS11, b, 120, 280, C_GOLD, AL_C);
    T(&F_SANS9, "OK 重新洗牌", 120, 308, C_CYAN, AL_C);
  }
}
const fc_scene_t SC_TAROT = {"tarot", "塔罗", false, t_enter, t_key, t_update, t_draw};

/* ================= 今日小票 ================= */
enum { D_IDLE, D_PRINT, D_TEAR, D_DONE };
enum { R_D, R_H, R_M, R_DASH, R_T, R_BAR, R_S };
static int dph, L, lt, pause_, tt, print_n;
static int dpage, dpages = 1;
static struct { int16_t o; int8_t ty; char s[96]; } rows[16];
static int nrows;
#define TH 256
#define SLOT 296
static void addrow(int o, int ty, const char *s) { rows[nrows].o = (int16_t)o; rows[nrows].ty = (int8_t)ty; snprintf(rows[nrows].s, sizeof rows[0].s, "%s", s); nrows++; }
static void make(void) {
  fc_time_t d; fc_now(&d);
  static const char *const WD[7] = {"日", "一", "二", "三", "四", "五", "六"};
  fc_ticket_t t;
  fc_daily_ticket(fc_daily_seed(d.y, d.m, d.d, PF.y, PF.m, PF.d, PF.h, PF.g), fc_zodiac_sign(PF.m, PF.d), &t);
  print_n++;
  char b[96], cn[16];
  nrows = 0;
  addrow(14, R_D, "★ NABO 赛博算命机 ★");
  addrow(34, R_H, "今日一卦");
  snprintf(b, sizeof b, "%d.%02d.%02d  周%s", d.y, d.m, d.d, WD[d.wday % 7]); addrow(52, R_M, b);
  addrow(64, R_DASH, "");
  static const char *const STARS = "★★★★★☆☆☆☆☆";       /* 每个 3 字节 */
  snprintf(b, sizeof b, "星座  %s  %.15s", FT_SIGN_NAME[t.sign], STARS + (5 - t.sign_stars) * 3); addrow(80, R_T, b);
  snprintf(b, sizeof b, "塔罗  %s %s%s", FT_TAROT_NUM[t.tarot], FT_TAROT_NAME[t.tarot], t.tarot_rev ? " · 逆位" : ""); addrow(100, R_T, b);
  cn_num(t.lot_no, cn);
  snprintf(b, sizeof b, "灵签  第%s签 · %s", cn, t.lot_level == 0 ? "上签" : t.lot_level == 1 ? "中签" : "下签"); addrow(120, R_T, b);
  snprintf(b, sizeof b, "卦象  %s", FT_HEX_NAME[t.hex_up * 8 + t.hex_lo]); addrow(140, R_T, b);
  addrow(154, R_DASH, "");
  snprintf(b, sizeof b, "宜  %s · %s · %s", FT_YI[t.yi[0]], FT_YI[t.yi[1]], FT_YI[t.yi[2]]); addrow(170, R_T, b);
  snprintf(b, sizeof b, "忌  %s · %s", FT_JI[t.ji[0]], FT_JI[t.ji[1]]); addrow(188, R_T, b);
  addrow(202, R_DASH, "");
  addrow(220, R_BAR, "");
  snprintf(b, sizeof b, "仅供娱乐 · NO.%04d-%d", t.serial, print_n); addrow(242, R_S, b);
}
static void d_enter(void) { dph = D_IDLE; L = 0; dpage = 0; }
static void d_key(int k, bool d) {
  if (!d) return;
  if (dph == D_DONE) {
    if (k == K_UP || k == K_DOWN) { dpage = (dpage + (k == K_DOWN ? 1 : dpages - 1)) % dpages; fc_sfx_play(SFX_TICK); }
    if (k == K_OK) dph = D_IDLE;
  } else if (k == K_OK && dph != D_PRINT) { make(); dph = D_PRINT; L = 0; lt = (int)VT; pause_ = 0; dpage = 0; }
}
static void d_update(int dt) {
  (void)dt;
  int v = (int)VT;
  if (dph == D_PRINT) {
    if (v < pause_) return;
    if (v - lt > 26) {
      lt = v; L += 3; fc_sfx_play(SFX_PRINT);
      for (int i = 0; i < nrows; i++) { int dd = rows[i].o - L; if (dd > -2 && dd < 2 && rows[i].ty != R_DASH) pause_ = v + (rows[i].ty == R_D ? 180 : 90); }
      if (L >= TH) { L = TH; dph = D_TEAR; tt = v; fc_sfx_play(SFX_WHOOSH); }
    }
  }
  if (dph == D_TEAR && v - tt > 400) { dph = D_DONE; burst(120, 290, 20, C_CYAN); }
}
static void d_draw(void) {
  int v = (int)VT;
  if (dph == D_DONE) {
    char body[2048] = "";
    for (int i = 0; i < nrows; i++) if (rows[i].ty == R_T || rows[i].ty == R_S) fc_body_add(body, sizeof body, "%s\n", rows[i].s);
    dpages = fc_read_card("今日一卦", rows[2].s, body, dpage);
    return;
  }
  if (dph != D_IDLE) {
    int lift = (dph == D_TEAR || dph == D_DONE) ? -8 * ease_back(fx_prog(v - tt, 400)) / 1000 : 0, top = SLOT - L + lift;
    fc_gfx_clip(0, 0, FC_W, SLOT);
    fc_rect(32, top, 176, L, RGB(0xF4, 0xEF, 0xE4), 255);
    for (int x = 32; x < 208; x += 8) fc_tri(x, top - 1, x + 4, top + 4, x + 8, top - 1, C_BG);
    if (dph != D_PRINT) for (int x = 32; x < 208; x += 8) fc_tri(x, SLOT + lift + 1, x + 4, SLOT + lift - 4, x + 8, SLOT + lift + 1, C_BG);
    for (int i = 0; i < nrows; i++) {
      int o = rows[i].o, y = top + o; if (o > L - 4) continue;
      const char *s = rows[i].s;
      switch (rows[i].ty) {
        case R_D: T(&F_DISP13, s, 120, y, RGB(0xC0, 0x18, 0x7A), AL_C); break;
        case R_H: T(&F_KAI15, s, 120, y, C_INK, AL_C); break;
        case R_M: T(&F_SANS9, s, 120, y, RGB(0x5A, 0x4A, 0x3A), AL_C); break;
        case R_DASH: for (int x = 40; x < 200; x += 6) fc_rect(x, y, 3, 1, RGB(0xB8, 0xAB, 0x92), 255); break;
        case R_BAR: {
          fc_rng_t r = {(uint32_t)(o * 31 + 7)}; int x = 48;
          while (x < 192) { int w = 1 + fc_rng_int(&r, 3); fc_rect(x, y - 9, w, 18, C_INK, 255); x += w + 1 + fc_rng_int(&r, 3); }
          break; }
        case R_S: T(&F_SANS9, s, 120, y, RGB(0x8A, 0x7A, 0x62), AL_C); break;
        default: T(utf8_len(s) > 14 ? &F_SANS9 : &F_SANS11, s, 44, y, C_INK, AL_L);
      }
    }
    fc_gfx_clip(0, 0, 0, 0);
  }
  const fc_img_t *pr = prop("printer");
  if (pr) fc_img(pr, 0, 14, SLOT - 4, 255);
  if (dph == D_PRINT && (v / 120) % 2) fc_disc(204, SLOT + 14, 2, C_CYAN, 255);
  if (dph == D_IDLE) {
    nabo_then("N-PRINT", 120, 120, 138, EL(), "N-IDLE");
    G(&F_DISP24, "今日一卦", 120, 214, C_WHITE, C_MAG);
    T(&F_SANS11, "OK 打印今天的命签小票", 120, 244, C_DIM, AL_C);
  }
  if (dph == D_DONE) T(&F_SANS9, "OK 再打一张", 120, 32, C_CYAN, AL_C);
}
const fc_scene_t SC_DAILY = {"daily", "今日一卦", false, d_enter, d_key, d_update, d_draw};
