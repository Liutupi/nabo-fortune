/* 称骨论命（原型 S.bone）、星座（原型 S.star） */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

/* ================= 称骨 ================= */
enum { B_CONFIRM, B_WEIGH, B_READ };
static int bph, wt0, added, dn, tk, bpage, br0;
static fc_bone_t B;
static int wq[4];                        /* 四项骨重（钱） */
static char wlab[4][16];
static int64_t tilt_u, vel_u;            /* 秤杆角度（微弧度）、角速度（微弧度/秒） */
static int load_q, p_m, pT_m;            /* 已放砝码（钱）；秤砣位置（千分之一像素） */
static struct { int q; int t; } fl[4];
static int nfl;

static void bone_enter(void) { bph = B_CONFIRM; bpage = 0; }
static void bone_start(void) {
  bpage = 0;
  fc_lunar_t l; pf_lunar(&PF, &l);
  fc_bone(&l, PF.h, &B);
  wq[0] = B.w_year; wq[1] = B.w_month; wq[2] = B.w_day; wq[3] = B.w_hour;
  char g[8]; gz_str(fc_lunar_year_gz(l.year), g);
  snprintf(wlab[0], 16, "%s年", g);
  snprintf(wlab[1], 16, "%s%s月", l.leap ? "闰" : "", FT_LMONTH[l.month - 1]);
  snprintf(wlab[2], 16, "%s", FT_LDAY[l.day - 1]);
  snprintf(wlab[3], 16, "%s时", FT_ZHI[PF.h]);
  bph = B_WEIGH; wt0 = (int)VT; tilt_u = vel_u = 0; load_q = 0; p_m = 20000; added = 0; nfl = 0; dn = 0; tk = -1;
  pT_m = 20000 + B.total * 76000 / 72;
}
static int read_m(void) { return (int)((int64_t)(p_m - 20000) * 7200 / 76000); }   /* 千分之一两 */
static void bone_key(int k, bool d) {
  if (!d) return;
  if (bph == B_CONFIRM) {
    if (k == K_OK) { fc_sfx_play(SFX_ENTER); bone_start(); }
    if (k == K_DOWN) { g_birth_ret = "bone"; fc_ui_go("birth"); }
  } else if (bph == B_WEIGH && (int)VT - wt0 > 9000) {
    if (k == K_OK) { bph = B_READ; br0 = (int)VT; fc_sfx_play(SFX_WHOOSH); }
  } else if (bph == B_READ) {
    if (k == K_OK) bph = B_CONFIRM;
    if (k == K_DOWN) { bpage = (bpage + 1) % 3; fc_sfx_play(SFX_TICK); }
    if (k == K_UP) { bpage = (bpage + 2) % 3; fc_sfx_play(SFX_TICK); }
  }
}
static void bone_update(int dt) {
  if (bph != B_WEIGH) return;
  int t = (int)VT - wt0;
  for (int i = 0; i < 4; i++) {
    int land = 800 + i * 1300 + 600;
    if (t >= land && added == i) {
      added++; load_q += wq[i]; vel_u -= 2200000; fc_sfx_play(SFX_THUMP);
      if (nfl < 4) { fl[nfl].q = wq[i]; fl[nfl].t = (int)VT; nfl++; }
    }
  }
  if (t > 6200) {
    int p = ease_io(fx_prog(t - 6200, 2200));
    p_m = fx_lerp(20000, pT_m, p);
    if (t / 60 != tk && t < 8400) { tk = t / 60; fc_sfx_play(SFX_TICK); }
  }
  int64_t target = (int64_t)(load_q * 100 - read_m()) * 100;
  if (target > 300000) target = 300000;
  if (target < -300000) target = -300000;
  vel_u += (60 * (target - tilt_u) - 7 * vel_u) * dt / 1000;
  tilt_u += vel_u * dt / 1000;
  if (t > 8400 && !dn) { dn = 1; fc_sfx_play(SFX_REVEAL); }
}
static void liang(int q, char *o) {          /* 钱 → “四两二钱” */
  strcpy(o, FT_CN[q / 10]); strcat(o, "两");
  if (q % 10) { strcat(o, FT_CN[q % 10]); strcat(o, "钱"); }
}
static void bone_draw(void) {
  if (bph == B_CONFIRM) { draw_confirm("称骨论命"); return; }
  int t = (int)VT - wt0;
  const int Px = 118, Py = 96;
  int a = (int)(-tilt_u * 163 / 1000000);           /* 微弧度 → 1/1024 圈 */
  int s = fx_sin(a), c = fx_cos(a);
#define WX(lx, ly) (Px + (((lx) * c - (ly) * s) >> 14))
#define WY(lx, ly) (Py + (((lx) * s + (ly) * c) >> 14))
  fc_rect(Px, 22, 1, Py - 4 - 22, C_DIM, 255);
  fc_disc(Px, Py - 4, 3, C_GOLD, 255);
  const fc_img_t *bm = prop("scale-beam");
  if (bm) fc_img_aff(bm, 0, Px, Py, 80, 3, a, 256, 256, 255);
  int hx = WX(-72, 2), hy = WY(-72, 2), panY = hy + 56;
  fc_line(hx, hy, hx - 22, panY, 1, C_DIM);
  fc_line(hx, hy, hx + 22, panY, 1, C_DIM);
  const fc_img_t *pn = prop("scale-pan");
  if (pn) fc_img(pn, 0, hx - 28, panY - 6, 255);
  int pp = p_m / 1000, gx = WX(pp, 2), gy = WY(pp, 2);
  fc_rect(gx, gy, 1, 20, C_DIM, 255);
  const fc_img_t *wt = prop("scale-weight");
  if (wt) fc_img(wt, 0, gx - 9, gy + 18, 255);
  static const char *const WN[4] = {"weight-nian", "weight-yue", "weight-ri", "weight-shi"};
  for (int i = 0; i < 4; i++) {
    int st = 800 + i * 1300; if (t < st) continue;
    int x = hx + (i * 2 - 3) * 6;
    int y = t < st + 600 ? fx_lerp(24, panY - 6, ease_in(fx_prog(t - st, 600))) : panY - 6;
    const fc_img_t *wi = prop(WN[i]);
    if (wi) fc_img(wi, 0, x - 6, y - 6, 255);
  }
  for (int i = 0; i < nfl; i++) {
    int e = (int)VT - fl[i].t; if (e >= 900) continue;
    TF(&F_SANS11, hx + 36, panY - 14 - e * 20 / 900, C_GOLD, AL_C, "+%d.%d两", fl[i].q / 10, fl[i].q % 10);
  }
  nabo("N-CALC", 56, 206, 48, (int)VT);
  if (t < 6200) TF(&F_SANS11, 120, 192, C_DIM, AL_C, "放入砝码 %d/4", added);
  else {
    char b[32]; liang((read_m() + 50) / 100, b);
    G(&F_DISP24, b, 120, 186, C_GOLD, RGB(0x5A, 0x3A, 0x00));
    char ln[128];
    snprintf(ln, sizeof ln, "%s %d.%d  %s %d.%d  %s %d.%d  %s %d.%d", wlab[0], wq[0] / 10, wq[0] % 10, wlab[1], wq[1] / 10, wq[1] % 10,
             wlab[2], wq[2] / 10, wq[2] % 10, wlab[3], wq[3] / 10, wq[3] % 10);
    T(&F_KAI10, ln, 120, 212, C_DIM, AL_C);
  }
  if (bph == B_WEIGH && t > 9000) T(&F_SANS11, "OK 看完整解读", 120, 300, C_CYAN, AL_C);
  if (bph == B_READ) {
    int p = ease_out(fx_prog((int)VT - br0, 650)), y = fx_lerp(330, 30, p), band = fc_bone_band(B.total);
    fc_rect(8, y, 224, 286, C_PAPER, 255);
    char b[32]; liang(B.total, b);
    T(&F_DISP24, b, 120, y + 25, C_INK, AL_C);
    T(&F_KAI15, FT_BONE_TYPE[band], 120, y + 55, RGB(0x8A, 0x2E, 0x1E), AL_C);
    fc_rect(24, y + 72, 192, 1, RGB(0xC9, 0xB4, 0x8C), 255);
    if (bpage == 0) {
      T(&F_SANS11, "白话解读", 24, y + 91, C_INK, AL_L);
      const char *L[12]; int n[12], k = wrap_px(&F_SANS11, FT_BONE_TEXT[band], 192, L, n, 12);
      for (int i = 0; i < k; i++) fc_text_n(&F_SANS11, 24, y + 119 + i * 22, L[i], n[i], RGB(0x4A, 0x30, 0x20), AL_L);
    } else if (bpage == 1 && B.total >= 21 && B.total <= 72) {
      T(&F_SANS11, "称骨歌原文", 24, y + 91, C_INK, AL_L);
      const char *po = FT_BONE_POEM[B.total - 21]; int row = 0;
      while (*po && row < 6) {
        const char *end = strstr(po, "，"); if (!end) end = po + strlen(po); else end += 3;
        fc_text_n(&F_KAI13, 120, y + 125 + row++ * 29, po, (int)(end - po), RGB(0x8A, 0x2E, 0x1E), AL_C);
        po = end;
      }
    } else {
      T(&F_SANS11, "骨重明细", 24, y + 91, C_INK, AL_L);
      static const char *const label[] = {"年", "月", "日", "时"};
      for (int i = 0; i < 4; i++) TF(&F_SANS11, 24, y + 126 + i * 32, C_INK, AL_L, "%s重  %s  %d.%d两", label[i], wlab[i], wq[i] / 10, wq[i] % 10);
    }
    T(&F_SANS9, "仅供娱乐", 120, y + 247, RGB(0x7A, 0x62, 0x48), AL_C);
    TF(&F_SANS9, 120, y + 271, RGB(0x7A, 0x62, 0x48), AL_C, "↑↓ 翻页 %d/3 · OK 返回", bpage + 1);
  }
#undef WX
#undef WY
}
const fc_scene_t SC_BONE = {"bone", "称骨论命", false, bone_enter, bone_key, bone_update, bone_draw};

/* ================= 星座 ================= */
enum { S_PICK, S_DRAW, S_DONE };
static int sph, sel, spt, slt, npt, rate[4], lucky_n, lucky_c;
static struct { int x, y; } pts[8];
static const char *const LUCKY_C[5] = {"金", "青", "紫", "红", "白"};
static int rfrac(fc_rng_t *r, int n) { return (int)(((uint64_t)fc_rng_u32(r) * (uint32_t)n) >> 32); }

static void star_enter(void) { sph = S_PICK; sel = fc_zodiac_sign(PF.m, PF.d); }
static void star_key(int k, bool d) {
  if (!d) return;
  if (sph == S_PICK) {
    if (k == K_UP) { sel = (sel + 11) % 12; fc_sfx_play(SFX_TICK); }
    if (k == K_DOWN) { sel = (sel + 1) % 12; fc_sfx_play(SFX_TICK); }
    if (k == K_OK) {
      sph = S_DRAW; spt = (int)VT; slt = -1;
      fc_rng_t r = {(uint32_t)(sel * 977 + 3)};
      npt = 6 + rfrac(&r, 3);
      for (int i = 0; i < npt; i++) { pts[i].x = 40 + rfrac(&r, 160); pts[i].y = 52 + rfrac(&r, 130); }
      for (int i = 1; i < npt; i++) for (int j = i; j > 0 && pts[j - 1].x > pts[j].x; j--) { int tx = pts[j].x, ty = pts[j].y; pts[j] = pts[j - 1]; pts[j - 1].x = tx; pts[j - 1].y = ty; }
      for (int i = 0; i < 4; i++) rate[i] = 2 + RI(4);
      lucky_n = RI(9) + 1; lucky_c = RI(5);
    }
  } else if (k == K_OK && sph == S_DONE) sph = S_PICK;
}
static void star_update(int dt) {
  (void)dt;
  if (sph != S_DRAW) return;
  int t = (int)VT - spt, ti = t / 120;
  if (ti != slt && ti < npt) { slt = ti; fc_sfx_play(SFX_LINE); }
  if (t > npt * 120 + (npt - 1) * 170 + 300) { sph = S_DONE; spt = (int)VT; }
}
static void star_draw(void) {
  fc_rng_t r = {42};
  for (int i = 0; i < 50; i++) {
    int x = rfrac(&r, 240), y = 26 + rfrac(&r, 190);
    bool hi = fx_sin((int)((int64_t)VT * 163 / 500) + i * 163) > 6554;      /* sin > 0.4 */
    fc_rect(x, y, 1, 1, hi ? C_WHITE : RGB(0x4A, 0x3D, 0x78), 255);
  }
  if (sph == S_PICK) {
    G(&F_DISP24, FT_SIGN_NAME[sel], 120, 140, C_WHITE, C_MAG);
    T(&F_SANS11, FT_SIGN_DATE[sel], 120, 174, C_DIM, AL_C);
    T(&F_SANS11, "▲", 120, 96, C_DIM, AL_C);
    T(&F_SANS11, "▼", 120, 198, C_DIM, AL_C);
    T(&F_SANS9, "已按生辰定好 · ↑↓ 可换 · OK 看今日", 120, 300, C_HINT, AL_C);
    return;
  }
  int t = sph == S_DRAW ? (int)VT - spt : 99999, base = npt * 120;
  for (int i = 0; i < npt - 1; i++) {
    int p = fx_prog(t - base - i * 170, 170);
    if (t - base - i * 170 <= 0) break;
    fc_line(pts[i].x, pts[i].y, fx_lerp(pts[i].x, pts[i + 1].x, p), fx_lerp(pts[i].y, pts[i + 1].y, p), 1, C_CYAN);
  }
  for (int i = 0; i < npt; i++) {
    if (t - i * 120 <= 0) continue;
    int p = fx_prog(t - i * 120, 200);
    int r10 = 20 + 40 * ease_back(p) / 1000 * (1000 - 4 * p / 10) / 1000 * 10 / 10;
    star5(pts[i].x, pts[i].y, r10, C_WHITE);
    fc_disc(pts[i].x, pts[i].y, 1, C_GOLD, 255);
  }
  T(&F_DISP13, FT_SIGN_NAME[sel], 120, 194, C_GOLD, AL_C);
  if (sph == S_DONE) {
    int tt = (int)VT - spt;
    static const char *const NM[4] = {"爱情", "事业", "财运", "心情"};
    for (int i = 0; i < 4; i++) {
      int y = 215 + i * 20;
      T(&F_SANS11, NM[i], 58, y, C_DIM, AL_R);
      for (int j = 0; j < 5; j++) star5(74 + j * 14, y, 50, j < rate[i] && tt > i * 220 + j * 70 ? C_GOLD : RGB(0x33, 0x29, 0x5A));
    }
    TF(&F_SANS9, 120, 294, C_CYAN, AL_C, "幸运数字 %d · 幸运色 %s", lucky_n, LUCKY_C[lucky_c]);
    T(&F_SANS9, "OK 换星座", 120, 312, C_HINT, AL_C);
  }
}
const fc_scene_t SC_STAR = {"star", "星座", false, star_enter, star_key, star_update, star_draw};
