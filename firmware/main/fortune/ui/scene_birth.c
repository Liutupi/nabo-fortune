/* 生辰八字输入（原型 S.birth）、生辰卡、确认页 */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

int gz_str(int gz, char *o) { strcpy(o, FT_GAN[gz % 10]); strcat(o, FT_ZHI[gz % 12]); return (int)strlen(o); }
static int pf_hour(const fc_profile_t *p) { return p->h == 0 ? 0 : p->h * 2; }   /* 原型 lunarOf：h*2 点 30 分 */
void pf_lunar(const fc_profile_t *p, fc_lunar_t *l) { fc_solar_to_lunar(p->y, p->m, p->d, l); }

void draw_pf(const fc_profile_t *p, int y0, bool demo) {
  fc_lunar_t l; pf_lunar(p, &l);
  fc_bazi_t bz; fc_bazi(p->y, p->m, p->d, pf_hour(p), 30, &bz);
  TF(&F_SANS11, 120, y0, C_WHITE, AL_C, "%d.%02d.%02d  %s时  %s", p->y, p->m, p->d, FT_ZHI[p->h], p->g ? "女" : "男");
  char yg[8]; gz_str(fc_lunar_year_gz(l.year), yg);
  TF(&F_KAI10, 120, y0 + 20, C_DIM, AL_C, "农历 %s年 %s%s月%s %s时", yg, l.leap ? "闰" : "", FT_LMONTH[l.month - 1], FT_LDAY[l.day - 1], FT_ZHI[p->h]);
  static const char *const LAB[4] = {"年柱", "月柱", "日柱", "时柱"};
  int8_t g4[4] = {bz.y, bz.m, bz.d, bz.h};
  for (int i = 0; i < 4; i++) {
    int x = 120 + (i * 2 - 3) * 28;
    fc_frame(x - 25, y0 + 36, 50, 65, 1, RGB(0x3A, 0x2D, 0x6A));
    T(&F_SANS9, LAB[i], x, y0 + 47, C_DIM, AL_C);
    T(&F_KAI15, FT_GAN[g4[i] % 10], x, y0 + 67, C_GOLD, AL_C);
    T(&F_KAI15, FT_ZHI[g4[i] % 12], x, y0 + 89, C_WHITE, AL_C);
  }
  TF(&F_SANS11, 120, y0 + 114, C_CYAN, AL_C, "属%s · %s", FT_SHENGXIAO[fc_lunar_year_gz(l.year) % 12], FT_SIGN_NAME[fc_zodiac_sign(p->m, p->d)]);
  if (demo && p->demo) T(&F_SANS9, "示例生辰 · ↓ 修改", 120, y0 + 132, C_MAG, AL_C);
}
void draw_confirm(const char *title) {
  G(&F_DISP24, title, 120, 40, C_WHITE, C_MAG);
  T(&F_SANS9, "命主生辰", 120, 62, C_DIM, AL_C);
  draw_pf(&PF, 84, true);
  T(&F_SANS11, "OK 开始", 120, 262, C_CYAN, AL_C);
  T(&F_SANS11, "↓ 修改生辰", 120, 284, C_DIM, AL_C);
}

/* ---------------- 输入页 ---------------- */
static const int FW[5] = {56, 34, 34, 44, 34};
static const char *const FN[5] = {"年", "月", "日", "时辰", "性别"};
static fc_profile_t v;
static int f, off[5], rep_dir, rep_at;      /* off：×100 像素 */

static int dim(int y, int m) {
  static const int D[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return m == 2 && ((y % 4 == 0 && y % 100) || y % 400 == 0) ? 29 : D[m - 1];
}
static void chg(fc_profile_t *p, int fi, int dir) {
  if (fi == 0) p->y = (int16_t)fx_clamp(p->y + dir, 1930, 2025);
  if (fi == 1) p->m = (int8_t)((p->m - 1 + dir + 12) % 12 + 1);
  if (fi == 2) { int n = dim(p->y, p->m), d = p->d < n ? p->d : n; p->d = (int8_t)((d - 1 + dir + n) % n + 1); }
  if (fi == 3) p->h = (int8_t)((p->h + dir + 12) % 12);
  if (fi == 4) p->g = (int8_t)!p->g;
  int n = dim(p->y, p->m); if (p->d > n) p->d = (int8_t)n;
}
static void fmt(int fi, const fc_profile_t *p, char *o) {
  if (fi == 0) sprintf(o, "%d", p->y);
  else if (fi == 1) sprintf(o, "%d月", p->m);
  else if (fi == 2) sprintf(o, "%d日", p->d);
  else if (fi == 3) sprintf(o, "%s时", FT_ZHI[p->h]);
  else strcpy(o, p->g ? "女" : "男");
}
static void step(int dir) { chg(&v, f, dir); off[f] = dir * 1800; fc_sfx_play(SFX_TICK); }
static void enter(void) { f = 0; v = PF; memset(off, 0, sizeof off); rep_dir = 0; }
static void key(int k, bool d) {
  if (k == K_UP || k == K_DOWN) {
    int dir = k == K_UP ? -1 : 1;
    if (d) { step(dir); rep_dir = dir; rep_at = (int)VT + 380; } else rep_dir = 0;
    return;
  }
  if (k == K_OK && d) {
    if (f < 4) { f++; fc_sfx_play(SFX_TICK); }
    else { v.demo = PF.demo; PF = v; fc_profile_save(); fc_sfx_play(SFX_ENTER); const char *r = g_birth_ret; g_birth_ret = NULL; fc_ui_go(r ? r : "menu"); }
  }
}
static void update(int dt) {
  if (rep_dir && (int)VT >= rep_at) { step(rep_dir); rep_at = (int)VT + 70; }
  int k = fx_clamp(1000 - dt * 18, 0, 1000);
  for (int i = 0; i < 5; i++) off[i] = off[i] * k / 1000;
}
static void draw(void) {
  G(&F_DISP24, "输入生辰", 120, 34, C_WHITE, C_MAG);
  T(&F_SANS9, "公历出生日期和时辰", 120, 54, C_DIM, AL_C);
  int x = 7;
  for (int i = 0; i < 5; i++) {
    int w = FW[i], cx = x + w / 2; bool on = i == f;
    T(&F_SANS9, FN[i], cx, 70, on ? C_MAG : C_DIM, AL_C);
    fc_gfx_clip(x, 78, w, 66);
    for (int dd = -1; dd <= 1; dd++) {
      fc_profile_t vv = v; if (dd) chg(&vv, i, dd);
      char s[16]; fmt(i, &vv, s);
      T(dd ? &F_KAI10 : &F_KAI15, s, cx, 111 + dd * 22 + off[i] / 100, dd ? RGB(0x4A, 0x3D, 0x78) : (on ? C_GOLD : C_WHITE), AL_C);
    }
    fc_gfx_clip(0, 0, 0, 0);
    if (on) fc_frame(x, 99, w, 25, 1, C_MAG);
    x += w + 6;
  }
  draw_pf(&v, 160, false);
  T(&F_SANS9, f < 4 ? "↑↓ 调整 · OK 下一项" : "OK 保存生辰", 120, 304, f < 4 ? C_HINT : C_CYAN, AL_C);
}
const fc_scene_t SC_BIRTH = {"birth", "生辰八字", true, enter, key, update, draw};
