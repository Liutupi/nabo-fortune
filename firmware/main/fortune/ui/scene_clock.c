/* 校准时间（固件专有：工牌没有备用电池，断电后时间未知；原型直接用浏览器时间） */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

static const int FW[5] = {54, 34, 34, 42, 42};
static const char *const FN[5] = {"年", "月", "日", "时", "分"};
static fc_time_t v;
static int f, off[5], rep_dir, rep_at;

static int dim(int y, int m) {
  static const int D[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return m == 2 && ((y % 4 == 0 && y % 100) || y % 400 == 0) ? 29 : D[m - 1];
}
static void chg(fc_time_t *p, int fi, int dir) {
  if (fi == 0) p->y = (int16_t)fx_clamp(p->y + dir, 2024, 2099);
  if (fi == 1) p->m = (int8_t)((p->m - 1 + dir + 12) % 12 + 1);
  if (fi == 2) { int n = dim(p->y, p->m), d = p->d < n ? p->d : n; p->d = (int8_t)((d - 1 + dir + n) % n + 1); }
  if (fi == 3) p->hh = (int8_t)((p->hh + dir + 24) % 24);
  if (fi == 4) p->mm = (int8_t)((p->mm + dir + 60) % 60);
  int n = dim(p->y, p->m); if (p->d > n) p->d = (int8_t)n;
}
static void fmt(int fi, const fc_time_t *p, char *o) {
  if (fi == 0) sprintf(o, "%d", p->y);
  else if (fi == 1) sprintf(o, "%d月", p->m);
  else if (fi == 2) sprintf(o, "%d日", p->d);
  else if (fi == 3) sprintf(o, "%02d时", p->hh);
  else sprintf(o, "%02d分", p->mm);
}
static void step(int dir) { chg(&v, f, dir); off[f] = dir * 1800; fc_sfx_play(SFX_TICK); }
static void enter(void) { f = 0; fc_now(&v); memset(off, 0, sizeof off); rep_dir = 0; }
static void key(int k, bool d) {
  if (k == K_UP || k == K_DOWN) {
    int dir = k == K_UP ? -1 : 1;
    if (d) { step(dir); rep_dir = dir; rep_at = (int)VT + 380; } else rep_dir = 0;
    return;
  }
  if (k == K_OK && d) {
    if (f < 4) { f++; fc_sfx_play(SFX_TICK); }
    else { fc_set_now(&v); g_time_unknown = false; fc_sfx_play(SFX_ENTER); fc_ui_go("menu"); }
  }
}
static void update(int dt) {
  if (rep_dir && (int)VT >= rep_at) { step(rep_dir); rep_at = (int)VT + 70; }
  int k = fx_clamp(1000 - dt * 18, 0, 1000);
  for (int i = 0; i < 5; i++) off[i] = off[i] * k / 1000;
}
static void draw(void) {
  G(&F_DISP24, "校准时间", 120, 40, C_WHITE, C_MAG);
  T(&F_SANS9, "断电后需校准时间", 120, 66, C_DIM, AL_C);
  int x = 7;
  for (int i = 0; i < 5; i++) {
    int w = FW[i], cx = x + w / 2; bool on = i == f;
    T(&F_SANS9, FN[i], cx, 86, on ? C_MAG : C_DIM, AL_C);
    fc_gfx_clip(x, 94, w, 66);
    for (int dd = -1; dd <= 1; dd++) {
      fc_time_t vv = v; if (dd) chg(&vv, i, dd);
      char s[16]; fmt(i, &vv, s);
      T(dd ? &F_KAI10 : &F_KAI15, s, cx, 127 + dd * 22 + off[i] / 100, dd ? RGB(0x4A, 0x3D, 0x78) : (on ? C_GOLD : C_WHITE), AL_C);
    }
    fc_gfx_clip(0, 0, 0, 0);
    if (on) fc_frame(x, 115, w, 25, 1, C_MAG);
    x += w + 6;
  }
  static const char *const WD[7] = {"日", "一", "二", "三", "四", "五", "六"};
  char g[8], h[8]; gz_str(fc_day_gz(v.y, v.m, v.d), g); gz_str(fc_hour_gz(v.y, v.m, v.d, v.hh), h);
  TF(&F_SANS11, 120, 186, C_WHITE, AL_C, "%d.%02d.%02d 周%s %02d:%02d", v.y, v.m, v.d, WD[fc_weekday(v.y, v.m, v.d)], v.hh, v.mm);
  TF(&F_KAI13, 120, 208, C_GOLD, AL_C, "%s日 %s时", g, h);
  T(&F_SANS9, f < 4 ? "↑↓ 调整 · OK 下一项" : "OK 保存时间", 120, 304, f < 4 ? C_HINT : C_CYAN, AL_C);
}
const fc_scene_t SC_CLOCK = {"clock", "校准时间", true, enter, key, update, draw};
