/* 灵签筒（原型 S.lot）：长按摇签 → 掉签弹跳 → 签飞到中间放大 → 卷轴展开逐句出签诗 → 盖印 → NABO 反应 */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

enum { L_READY, L_HOLD, L_SINK, L_DROP, L_REST, L_FLY, L_SCROLL, L_STAMP, L_DONE };
int fc_lot_force = -1;                    /* 测试钩子：≥1 时固定签号 */

static int ph, rise, hs, lr, pt, done_t, pick, no, lv, slam;
static int read_page, read_pages = 1;
static int ang_md;                        /* 筒倾角，毫度 */
static int hl[12];
static const char *msg;
/* 掉出的签：Q8 像素；rot 以 1/262144 圈（1/1024 圈 ×256）计 */
static struct { int32_t x, y, vx, vy, rot, vr; int b; } st;
static struct { int32_t x, y, rot; } from;

#define MD2A(md) ((int)((int64_t)(md) * 1024 / 360000))   /* 毫度 → 1/1024 圈 */

static void reset(void) {
  ph = L_READY; rise = 0; msg = NULL; ang_md = 0; slam = 0; read_page = 0;
  for (int i = 0; i < 12; i++) hl[i] = 14 + R1000() * 16 / 1000;
  pick = RI(12);
  no = fc_lot_force >= 1 ? fc_lot_force : RI(FC_LOT_COUNT) + 1;
  lv = fc_lot_level(no);
}
static void enter(void) { reset(); }

static void key(int k, bool d) {
  if (ph == L_DONE && d && (k == K_UP || k == K_DOWN)) { read_page = (read_page + (k == K_DOWN ? 1 : read_pages - 1)) % read_pages; fc_sfx_play(SFX_TICK); return; }
  if (k != K_OK) return;
  if (d) {
    if (ph == L_DONE) { reset(); return; }
    if (ph == L_READY) { ph = L_HOLD; hs = (int)VT; lr = 0; msg = NULL; }
  } else if (ph == L_HOLD) {
    if (rise >= 750) {
      int a = MD2A(ang_md), s = fx_sin(a), c = fx_cos(a);
      int sx = -22 + pick * 4, ly = -110 - hl[pick] - rise * 46 / 1000 + 40;
      st.x = (120 * 256) + ((sx * c - ly * s) >> 6);
      st.y = (250 * 256) + ((sx * s + ly * c) >> 6);
      st.vx = (70 + R1000() * 30 / 1000) * 256; st.vy = -140 * 256;
      st.rot = a * 256; st.vr = 7 * 163 * 256; st.b = 0;   /* 7 rad/s */
      ph = L_DROP; fc_sfx_play(SFX_WHOOSH);
    } else { ph = L_SINK; msg = "再摇久一点"; }
  }
}

static void update(int dt) {
  int t = (int)VT;
  if (ph == L_HOLD) {
    int h = t - hs, amp = fx_clamp(h * 1000 / 60, 0, 16000);
    ang_md = (int)((int64_t)fx_sin((int)((int64_t)VT * 163 / 55)) * amp >> 14);
    rise = fx_clamp(h * 1000 / 1600, 0, 1000);
    if (t - lr > 80) { lr = t; fc_sfx_play(SFX_RATTLE); }
  } else if (ph == L_SINK) {
    rise -= dt * 1500 / 1000; if (rise < 0) rise = 0;
    ang_md = ang_md * 85 / 100;
    if (!rise) ph = L_READY;
  } else ang_md = ang_md * 85 / 100;

  if (ph == L_DROP) {
    st.vy += 900 * 256 * dt / 1000;
    st.x += st.vx * dt / 1000; st.y += st.vy * dt / 1000; st.rot += (int32_t)((int64_t)st.vr * dt / 1000);
    if (st.y >= (292 * 256) && st.vy > 0) {
      st.y = 292 * 256; fc_sfx_play(SFX_CLINK);
      if (st.b < 1) { st.vy = -st.vy * 3 / 10; st.vr = st.vr * 4 / 10; st.b++; }
      else { ph = L_REST; pt = t; from.x = st.x; from.y = st.y; from.rot = st.rot; }
    }
  }
  if (ph == L_REST && t - pt > 350) { ph = L_FLY; pt = t; }
  if (ph == L_FLY && t - pt > 520) { ph = L_SCROLL; pt = t; }
  if (ph == L_SCROLL && t - pt > 900) { ph = L_STAMP; pt = t; }
  if (ph == L_STAMP && t - pt > 180 && !slam) { slam = 1; g_shake = 700; fc_sfx_play(SFX_STAMP); if (lv == 0) burst(176, 236, 50, 0); }
  if (ph == L_STAMP && t - pt > 700) { ph = L_DONE; slam = 0; done_t = t; }
}

/* 签：(x,y) 为签中心，rot 1/1024 圈，sc 千分比 */
static void stick(int x, int y, int rot, int sc, bool label) {
  const fc_img_t *si = prop("stick");
  if (si) fc_img_aff(si, 0, x, y, si->w / 2, si->h / 2, rot, sc * 256 / 1000, sc * 256 / 1000, 255);
  if (label && sc >= 1300) {
    char cn[16], s[32]; cn_num(no, cn); strcpy(s, "第"); strcat(s, cn); strcat(s, "签");
    int n = utf8_len(s), sn = fx_sin(rot), cs = fx_cos(rot);
    for (int i = 0; i < n; i++) {
      const char *a = utf8_at(s, i), *b = utf8_at(s, i + 1);
      int v = (-24 + i * 9) * sc / 1000;
      fc_text_n(&F_KAI10, x - (v * sn >> 14), y + (v * cs >> 14), a, (int)(b - a), C_INK, AL_C);
    }
  }
}

static void draw(void) {
  int t = (int)VT;
  if (ph == L_DONE) {
    char body[2048] = "", title[64], cn[16]; cn_num(no, cn);
    snprintf(title, sizeof title, "第%s签 · %s", cn, lv == 0 ? "上签" : lv == 1 ? "中签" : "下签");
    fc_body_add(body, sizeof body, "签诗\n%s\n解曰\n%s\n白话\n%s\n签象\n%s", fc_txt("lot_poem", no - 1), fc_txt("lot_jie", no - 1), fc_txt("lot_bh", no - 1), fc_txt("lot_xiang", no - 1));
    read_pages = fc_read_card(title, fc_txt("lot_story", no - 1), body, read_page);
    return;
  }
  bool tube = ph <= L_REST;
  if (tube) {
    int a = MD2A(ang_md), s = fx_sin(a), c = fx_cos(a);
    const fc_img_t *si = prop("stick");
    for (int i = 0; i < 12; i++) {
      if (i == pick && (ph == L_DROP || ph == L_REST)) continue;
      int x = -22 + i * 4, top = -110 - hl[i] - (i == pick ? rise * 46 / 1000 : 0);
      if (ph == L_HOLD) top += fx_sin((int)((int64_t)VT * 163 / 40) + i * 163) * 2 >> 14;
      int px = 120 + ((x * c - top * s) >> 14), py = 250 + ((x * s + top * c) >> 14);
      if (i == pick && rise > 750) {
        int bx = 120 + ((x * c - (top + 41) * s) >> 14), by = 250 + ((x * s + (top + 41) * c) >> 14);
        int tx = 120 + ((x * c - (top - 1) * s) >> 14), ty = 250 + ((x * s + (top - 1) * c) >> 14);
        fc_line(tx, ty, bx, by, 5, RGB(0xFF, 0xE0, 0x8A));
      }
      if (si) fc_img_aff(si, 0, px, py, si->w / 2, 0, a, 128, 256, 255);
    }
    const fc_img_t *tu = prop("tube-tilt");
    if (tu) {
      int k = fx_clamp((ang_md + 16000 + 1600) / 3200, 0, 10);
      fc_img(tu, k, 120 - 48, 250 - 112, 255);
    }
    nabo("N-TUBE", 76, 40, 90, t);
    if (ph == L_READY || ph == L_SINK) T(&F_SANS11, msg ? msg : "长按 OK 摇签", 120, 302, msg ? C_MAG : C_DIM, AL_C);
    if (ph == L_HOLD) {
      fc_rect(70, 300, 100, 4, C_BG2, 255); fc_rect(70, 300, rise / 10, 4, C_MAG, 255);
      if (rise >= 750) T(&F_SANS9, "松开！", 120, 312, C_GOLD, AL_C);
    }
  }
  if (ph == L_DROP || ph == L_REST) stick(st.x >> 8, st.y >> 8, st.rot >> 8, 1000, false);
  if (ph == L_FLY) {
    int p = ease_io(fx_prog(t - pt, 520));
    stick(fx_lerp(from.x >> 8, 120, p), fx_lerp(from.y >> 8, 150, p), fx_lerp(from.rot >> 8, 0, p), fx_lerp(1000, 1600, p), true);
  }
  if (ph >= L_SCROLL) {
    int p = ph == L_SCROLL ? ease_out(fx_prog(t - pt, 600)) : FX_ONE;
    int tl = ph == L_SCROLL ? (t - pt - 250) * 1000 / 140 : 9000;      /* 第 i 句在 tl > i 时出现 */
    const fc_img_t *sc = prop("scroll");
    int hh = (152 * p + 500) / 1000, y0 = 134 - (hh + 1) / 2;
    if (sc) {
      if (hh > 0) fc_img_part(sc, 0, 0, 32, 208, hh, 16, y0);
      fc_img_part(sc, 0, 0, 0, 208, 32, 16, y0 - 32);
      fc_img_part(sc, 0, 0, 184, 208, 32, 16, y0 + hh);
    }
    if (hh > 0) {
      fc_gfx_clip(42, y0, 156, hh);
      char cn[16], head[96]; cn_num(no, cn);
      strcpy(head, "第"); strcat(head, cn); strcat(head, "签 · "); strncat(head, fc_txt("lot_story", no - 1), 60);
      T(&F_KAI13, head, 120, 70, C_INK, AL_C);
      fc_rect(56, 81, 128, 1, RGB(0xB5, 0x8E, 0x5A), 255);
      const char *poem = fc_txt("lot_poem", no - 1);
      for (int i = 0; i < 4 && *poem; i++) {
        const char *e = strchr(poem, '\n'); int n = e ? (int)(e - poem) : (int)strlen(poem);
        if (i * 1000 < tl) fc_text_n(&F_KAI15, 120, 97 + i * 18, poem, n, C_INK, AL_C);
        poem += n + (e ? 1 : 0);
      }
      if (tl > 4000) {
        const char *j = fc_txt("lot_jie", no - 1), *sp1 = strchr(j, ' '), *sp2 = sp1 ? strchr(sp1 + 1, ' ') : NULL;
        char l1[96] = "解曰  ";
        int n1 = sp2 ? (int)(sp2 - j) : (int)strlen(j);
        if (n1 > 80) n1 = 80;
        size_t prefix = strlen(l1);
        memcpy(l1 + prefix, j, (size_t)n1);
        l1[prefix + (size_t)n1] = 0;
        T(&F_KAI10, l1, 120, 168, RGB(0x8A, 0x2E, 0x1E), AL_C);
        if (sp2) T(&F_KAI10, sp2 + 1, 135, 181, RGB(0x8A, 0x2E, 0x1E), AL_C);
      }
      if (tl > 5000) {
        const char *ln[8]; int lens[8];
        int n = wrap(fc_txt("lot_bh", no - 1), 14, ln, lens, 8);
        for (int i = 0; i < n && i < 3; i++) fc_text_n(&F_SANS9, 120, 192 + i * 11, ln[i], lens[i], RGB(0x6B, 0x4A, 0x2A), AL_C);
      }
      fc_gfx_clip(0, 0, 0, 0);
    }
  }
  if (ph == L_STAMP || ph == L_DONE) {
    int p = fx_prog(t - pt, 180), sc = ph == L_DONE ? 1000 : fx_lerp(2600, 1000, ease_in(p));
    int al = ph == L_STAMP ? fx_clamp(p * 1500 / 1000, 0, 1000) * 255 / 1000 : 255;
    static const char *const SEAL[3] = {"seal-shang", "seal-zhong", "seal-xia"};
    const fc_img_t *se = prop(SEAL[lv < 0 ? 1 : lv]);
    if (se) fc_img_aff(se, 0, 176, 262, 26, 20, -23, sc * 256 / 1000, sc * 256 / 1000, (uint8_t)al);
    if (ph == L_DONE) {
      const char *id = lv == 0 ? "N-WOW" : lv == 2 ? "N-SIGH" : "N-NOD";
      nabo_then(id, 76, 42, 278, t - done_t, "N-IDLE");
      T(&F_SANS9, "OK 再求一签", 120, 312, C_CYAN, AL_C);
    }
  }
}

const fc_scene_t SC_LOT = {"lot", "灵签筒", false, enter, key, update, draw};
