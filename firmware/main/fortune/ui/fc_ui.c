#include "fc_ui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

fc_font_t F_KAI15, F_KAI13, F_KAI10, F_SANS11, F_SANS9, F_DISP24, F_DISP13, F_MONO10;
uint32_t VT;
int g_shake;
fc_rng_t g_rng;

/* 场景表在 scenes.c */
extern const fc_scene_t *const FC_SCENES[];
static const fc_scene_t *g_scene;
static uint32_t g_t0, g_last_up;
static bool g_held[3];
static bool g_up_was_down;
/* Full FNAS keys are up to 31 bytes; long tarot names must never be truncated.
 * Keep handles stable while a display list refers to them. The current pack
 * has 62 image resources; leave room for future art without per-frame allocation. */
typedef struct { char name[32]; fc_img_t im; bool ok; } slot_t;
static slot_t g_spr[96];
static int g_nspr;
static unsigned g_image_misses;
unsigned fc_ui_image_misses(void) { return g_image_misses; }

uint32_t fc_ui_scene_t0(void) { return g_t0; }
const char *fc_ui_scene_id(void) { return g_scene ? g_scene->id : ""; }
bool fc_ui_held(int k) { return g_held[k]; }

__attribute__((weak)) void fc_sfx_play(int id) { (void)id; }
__attribute__((weak)) void fc_sfx_welcome(void) { fc_sfx_play(SFX_ENTER); }

bool fc_ui_init(const uint8_t *blob, uint32_t size, uint32_t seed) {
  fc_res_init(blob, size);
  memset(g_spr, 0, sizeof g_spr); g_nspr = 0; g_image_misses = 0;
  bool ok = fc_font_open("font/kai15", &F_KAI15) & fc_font_open("font/kai13", &F_KAI13) & fc_font_open("font/kai10", &F_KAI10) &
            fc_font_open("font/sans11", &F_SANS11) & fc_font_open("font/sans9", &F_SANS9) & fc_font_open("font/disp24", &F_DISP24) &
            fc_font_open("font/disp13", &F_DISP13) & fc_font_open("font/mono10", &F_MONO10);
  g_rng.a = seed;
  VT = 0; g_shake = 0;
  return ok;
}

void particles_clear(void);
void fc_ui_go(const char *id) {
  for (int i = 0; FC_SCENES[i]; i++)
    if (!strcmp(FC_SCENES[i]->id, id)) {
      g_scene = FC_SCENES[i]; g_t0 = VT; particles_clear();
      if (g_scene->enter) g_scene->enter();
      return;
    }
}
void fc_ui_back(void) {
  if (strcmp(fc_ui_scene_id(), "menu") && strcmp(fc_ui_scene_id(), "boot")) { fc_sfx_play(SFX_TICK); fc_ui_go("menu"); }
}

void fc_ui_key(int k, bool down) {
  if (k < 0 || k > 2 || g_held[k] == down) return;
  g_held[k] = down;
  if (down && k == K_UP && !g_scene->no_double) {
    if (g_up_was_down && VT - g_last_up < 300) { g_up_was_down = false; fc_ui_back(); return; }
    g_last_up = VT; g_up_was_down = true;
  }
  if (g_scene->key) g_scene->key(k, down);
}

/* ---------- 粒子：位置 Q8 像素，速度 Q8 像素/秒 ---------- */
#define NP 160
static struct { int32_t x, y, vx, vy; int16_t l; uint16_t c; } P[NP];
static int g_np;
void particles_clear(void) { g_np = 0; }
void burst(int x, int y, int n, uint16_t col) {
  for (int i = 0; i < n && g_np < NP; i++) {
    int a = RI(ANG_TURN), v = 40 + R1000() * 110 / 1000;
    P[g_np].x = x * 256; P[g_np].y = y * 256;
    P[g_np].vx = (fx_cos(a) * v) >> 6; P[g_np].vy = ((fx_sin(a) * v) >> 6) - 60 * 256;
    P[g_np].l = (int16_t)(600 + R1000() / 2);
    P[g_np].c = col ? col : (R1000() < 500 ? C_GOLD : RGB(0xFF, 0xF3, 0xB0));
    g_np++;
  }
}
static void step_p(int dt) {
  for (int i = g_np - 1; i >= 0; i--) {
    P[i].vy += 220 * 256 * dt / 1000;
    P[i].x += P[i].vx * dt / 1000; P[i].y += P[i].vy * dt / 1000;
    P[i].l = (int16_t)(P[i].l - dt);
    if (P[i].l <= 0) P[i] = P[--g_np];
  }
}

void fc_ui_step(int dt) {
  VT += (uint32_t)dt;
  if (g_scene->update) g_scene->update(dt);
  step_p(dt);
  g_shake -= 3 * dt;                       /* 原型：每毫秒衰减 0.03 */
  if (g_shake < 0) g_shake = 0;
}

void fc_ui_render(fc_flush_fn flush, void *ctx, bool force) {
  fc_gfx_begin();
  if (g_shake > 0) {
    int ox = RS(g_shake) / 100, oy = RS(g_shake) / 100;
    fc_gfx_offset(ox, oy);
  }
  g_scene->draw();
  for (int i = 0; i < g_np; i++) fc_rect(P[i].x >> 8, P[i].y >> 8, 2, 2, P[i].c, 255);
  fc_gfx_offset(0, 0); fc_gfx_clip(0, 0, 0, 0);
  if (strcmp(g_scene->id, "boot")) {
    fc_rect(0, 0, FC_W, 24, RGB(0x08, 0x05, 0x1A), 255);
    T(&F_DISP13, "NABO 赛博算命机", 6, 12, C_CYAN, AL_L);
    T(&F_SANS9, g_scene->name, 234, 12, C_DIM, AL_R);
    if ((VT / 700) % 2) fc_disc(166, 12, 2, C_MAG, 255);
  }
  fc_gfx_end(flush, ctx, force);
}

/* ---------- 精灵 ---------- */
static const fc_img_t *open_named(const char *name) {
  for (int i = 0; i < g_nspr; i++) if (!strcmp(g_spr[i].name, name)) return g_spr[i].ok ? &g_spr[i].im : NULL;
  if (strlen(name) >= sizeof g_spr[0].name || g_nspr >= (int)(sizeof g_spr / sizeof g_spr[0])) { g_image_misses++; return NULL; }
  slot_t *s = &g_spr[g_nspr++];
  snprintf(s->name, sizeof s->name, "%s", name);
  s->ok = fc_img_open(name, &s->im);
  if (!s->ok) g_image_misses++;
  return s->ok ? &s->im : NULL;
}
const fc_img_t *prop(const char *name) { char b[40]; snprintf(b, sizeof b, "prop/%s", name); return open_named(b); }
static const fc_img_t *spr_any(const char *id) {
  static const int SZ[] = {120, 76, 56, 100};
  char b[40];
  for (int i = 0; i < 4; i++) { snprintf(b, sizeof b, "spr/%s/%d", id, SZ[i]); if (fc_res(b, NULL)) return open_named(b); }
  return NULL;
}
int spr_frames(const char *id) { const fc_img_t *im = spr_any(id); return im ? im->frames : 1; }
int spr_ms(const char *id) { return spr_frames(id) * 1000 / 12; }
void nabo_frame(const char *id, int size, int cx, int cy, int f) {
  char b[40]; snprintf(b, sizeof b, "spr/%s/%d", id, size);
  const fc_img_t *im = open_named(b);
  if (!im) return;
  fc_img(im, f, cx - size / 2, cy - size / 2, 255);
}
void nabo(const char *id, int size, int cx, int cy, int t) {
  char b[40]; snprintf(b, sizeof b, "spr/%s/%d", id, size);
  const fc_img_t *im = open_named(b);
  if (!im) return;
  int n = im->frames, f = (int)((int64_t)t * 12 / 1000);
  if (t < 0) f = (int)(((int64_t)t * 12 - 999) / 1000);
  f = im->loop ? ((f % n) + n) % n : fx_clamp(f, 0, n - 1);
  fc_img(im, f, cx - size / 2, cy - size / 2, 255);
}
void nabo_then(const char *id, int size, int cx, int cy, int t, const char *idle) {
  int ms = spr_ms(id);
  if (t < ms) nabo(id, size, cx, cy, t); else nabo(idle ? idle : "N-IDLE", size, cx, cy, t - ms);
}

/* ---------- 文字 ---------- */
void T(const fc_font_t *f, const char *s, int x, int y, uint16_t c, int al) {
  int width = al == AL_L ? FC_W - 12 - x : al == AL_R ? x - 12 : FC_W - 24;
  if (width > 0 && fc_text_width(f, s) > width && (f == &F_SANS9 || f == &F_SANS11 || f == &F_KAI10)) {
    const char *ln[12]; int len[12], n = wrap_px(f, s, width, ln, len, 12);
    int step = f == &F_SANS11 ? 21 : 18;
    int top = y >= 280 ? y - (n - 1) * step : y;
    for (int i = 0; i < n; i++) fc_text_n(f, x, top + i * step, ln[i], len[i], c, al);
  } else fc_text(f, x, y, s, c, al);
}
void TF(const fc_font_t *f, int x, int y, uint16_t c, int al, const char *fmt, ...) {
  char b[160]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
  T(f, b, x, y, c, al);
}
void G(const fc_font_t *f, const char *s, int x, int y, uint16_t c, uint16_t glow) {
  fc_text(f, x - 1, y, s, glow, AL_C); fc_text(f, x + 1, y, s, glow, AL_C);
  fc_text(f, x, y - 1, s, glow, AL_C); fc_text(f, x, y + 1, s, glow, AL_C);
  fc_text(f, x, y, s, c, AL_C);
}
void glitch(const fc_font_t *f, const char *s, int x, int y, uint16_t c, int k) {
  int o = k > 0 ? RS(2 * k) : 0;
  fc_text(f, x - 2 * k + o, y, s, C_CYAN, AL_C);
  fc_text(f, x + 2 * k - o, y, s, C_MAG, AL_C);
  fc_text(f, x, y, s, c, AL_C);
}

int utf8_len(const char *s) { int n = 0; while (*s) { fc_utf8_next(&s); n++; } return n; }
const char *utf8_at(const char *s, int i) { while (i-- > 0 && *s) fc_utf8_next(&s); return s; }

static bool is_tail_punct(uint32_t c) { return c == 0xFF0C || c == 0x3002 || c == 0xFF1B || c == 0xFF1A || c == 0x3001 || c == 0xFF09; }
int wrap_px(const fc_font_t *f, const char *s, int width, const char **lines, int *lens, int max) {
  int n = 0;
  while (*s && n < max) {
    const char *start = s, *prev = s; int w = 0;
    while (*s && *s != '\n') {
      const char *next = s; uint32_t cp = fc_utf8_next(&next); fc_glyph_t g;
      int adv = cp == ' ' ? f->space : fc_font_glyph(f, cp, &g) ? g.adv : f->space;
      if (s > start && w + adv > width) { if (is_tail_punct(cp) && prev > start) s = prev; break; }
      w += adv; prev = s; s = next;
    }
    lines[n] = start; lens[n++] = (int)(s - start);
    if (*s == '\n') s++;
  }
  return n;
}
int wrap(const char *s, int n, const char **lines, int *lens, int max) {
  int k = 0;
  while (*s && k < max) {
    const char *st = s; int c = 0;
    while (*s && c < n) { fc_utf8_next(&s); c++; }
    if (*s) { const char *q = s; uint32_t cp = fc_utf8_next(&q); if (is_tail_punct(cp)) s = q; }
    lines[k] = st; lens[k] = (int)(s - st); k++;
  }
  return k;
}

void fc_body_add(char *body, int capacity, const char *fmt, ...) {
  int used = (int)strlen(body); if (used >= capacity) return;
  va_list ap; va_start(ap, fmt); vsnprintf(body + used, (size_t)(capacity - used), fmt, ap); va_end(ap);
}
int fc_read_card(const char *title, const char *subtitle, const char *body, int page) {
  const char *ln[64]; int len[64], n = wrap_px(&F_SANS11, body, 192, ln, len, 64);
  int pages = (n + 6) / 7; if (pages < 1) pages = 1;
  page = (page % pages + pages) % pages;
  fc_rect(8, 30, 224, 286, C_PAPER, 255);
  T(&F_KAI15, title, 120, 55, C_INK, AL_C);
  T(&F_SANS9, subtitle, 120, 82, RGB(0x8A, 0x2E, 0x1E), AL_C);
  fc_rect(24, 98, 192, 1, RGB(0xC9, 0xB4, 0x8C), 255);
  for (int i = 0; i < 7 && page * 7 + i < n; i++) fc_text_n(&F_SANS11, 24, 120 + i * 22, ln[page * 7 + i], len[page * 7 + i], RGB(0x3A, 0x24, 0x14), AL_L);
  T(&F_SANS9, "仅供娱乐", 120, 278, RGB(0x7A, 0x62, 0x48), AL_C);
  TF(&F_SANS9, 120, 302, RGB(0x7A, 0x62, 0x48), AL_C, "↑↓ 翻页 %d/%d · OK 返回", page + 1, pages);
  return pages;
}

static const char *CN[] = {"零", "一", "二", "三", "四", "五", "六", "七", "八", "九"};
void cn_num(int n, char *o) {
  o[0] = 0;
  if (n <= 10) { strcpy(o, n == 10 ? "十" : CN[n]); return; }
  if (n < 20) { strcpy(o, "十"); if (n % 10) strcat(o, CN[n % 10]); return; }
  if (n == 100) { strcpy(o, "一百"); return; }
  strcpy(o, CN[n / 10]); strcat(o, "十"); if (n % 10) strcat(o, CN[n % 10]);
}

/* 原型 tri()：j=0 为下爻，画在 y+gap 处；阴爻两段各 0.42w */
void tri3(int x, int y, const uint8_t bits[3], int w, uint16_t col, int gap, int th) {
  for (int j = 0; j < 3; j++) {
    int ly = y + (1 - j) * gap - th / 2;
    if (bits[j]) fc_rect(x - w / 2, ly, w, th, col, 255);
    else { int s = w * 42 / 100; fc_rect(x - w / 2, ly, s, th, col, 255); fc_rect(x + w * 8 / 100, ly, s, th, col, 255); }
  }
}
void tri3_rot(int cx, int cy, int r, int ang, const uint8_t bits[3], int w, uint16_t col, int gap, int th) {
  int s = fx_sin(ang), c = fx_cos(ang);
  /* 局部坐标 (u,v)：v 指向圆心外为负；屏幕 = 中心 + R(ang)·(u, v) */
#define PX(u, v) (cx + (((u) * c - (v) * s) >> 14))
#define PY(u, v) (cy + (((u) * s + (v) * c) >> 14))
  for (int j = 0; j < 3; j++) {
    int v = -r + (1 - j) * gap;
    if (bits[j]) fc_line(PX(-w / 2, v), PY(-w / 2, v), PX(w / 2, v), PY(w / 2, v), th, col);
    else {
      int a0 = -w / 2, a1 = -w / 2 + w * 42 / 100, b0 = w * 8 / 100, b1 = w * 8 / 100 + w * 42 / 100;
      fc_line(PX(a0, v), PY(a0, v), PX(a1, v), PY(a1, v), th, col);
      fc_line(PX(b0, v), PY(b0, v), PX(b1, v), PY(b1, v), th, col);
    }
  }
#undef PX
#undef PY
}

/* ---------- 档案、时间 ---------- */
fc_profile_t PF = {1985, 6, 15, 4, 0, true};
const char *g_birth_ret;
__attribute__((weak)) void fc_profile_save(void) { PF.demo = false; }
bool g_time_unknown;
int fc_weekday(int y, int m, int d) { return (int)((fc_days_from_1900(y, m, d) + 1) % 7); }   /* 1900-01-01 周一 */
static fc_time_t g_host_now = {2026, 10, 22, 14, 30, 4};
__attribute__((weak)) void fc_set_now(const fc_time_t *t) { g_host_now = *t; g_host_now.wday = (int8_t)fc_weekday(t->y, t->m, t->d); }
__attribute__((weak)) void fc_now(fc_time_t *t) { *t = g_host_now; }

/* ---------- 形状 ---------- */
void star5(int x, int y, int r10, uint16_t col) {
  int16_t v[20];
  for (int i = 0; i < 10; i++) {
    int a = -256 + i * 1024 / 10, rr = (i % 2) ? r10 * 45 / 100 : r10;   /* 外顶点朝上 */
    v[2 * i] = (int16_t)(x * 16 + 8 + (int)((int64_t)fx_cos(a) * rr * 16 / 10 >> 14));
    v[2 * i + 1] = (int16_t)(y * 16 + 8 + (int)((int64_t)fx_sin(a) * rr * 16 / 10 >> 14));
  }
  fc_poly(v, 10, col, 255);
}
void rect_rot(int cx, int cy, int w, int h, int ang, uint16_t col) {
  int s = fx_sin(ang), c = fx_cos(ang), U[4] = {-w * 8, w * 8, w * 8, -w * 8}, V[4] = {-h * 8, -h * 8, h * 8, h * 8};
  int16_t v[8];
  for (int i = 0; i < 4; i++) { v[2 * i] = (int16_t)(cx * 16 + ((U[i] * c - V[i] * s) >> 14)); v[2 * i + 1] = (int16_t)(cy * 16 + ((U[i] * s + V[i] * c) >> 14)); }
  fc_poly(v, 4, col, 255);
}
void frame_rot(int cx, int cy, int w, int h, int ang, int th, uint16_t col) {
  int s = fx_sin(ang), c = fx_cos(ang), U[4] = {-w / 2, w / 2, w / 2, -w / 2}, V[4] = {-h / 2, -h / 2, h / 2, h / 2}, X[4], Y[4];
  for (int i = 0; i < 4; i++) { X[i] = cx + ((U[i] * c - V[i] * s) >> 14); Y[i] = cy + ((U[i] * s + V[i] * c) >> 14); }
  for (int i = 0; i < 4; i++) fc_line(X[i], Y[i], X[(i + 1) % 4], Y[(i + 1) % 4], th, col);
}
