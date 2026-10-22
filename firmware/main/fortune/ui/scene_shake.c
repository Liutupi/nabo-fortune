/* 摇卦问事（原型 S.shake）：长按摇龟甲 → 三枚铜钱抛出翻转落地 → 六爻成卦 → 解卦卡（朱熹断法 + 卦爻辞原文 + 白话） */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

enum { H_READY, H_HOLD, H_TOSS, H_REVEAL, H_DONE };
static int ph, hs, lr, toss_t, launch_at, at, rt, ct, card, wh;
static int read_page, read_pages = 1;
static const char *msg;
static struct { uint8_t v; int t; } lines[6];
static int nl;
/* 铜钱：位置 Q8 像素；rot 以 1/1024 圈 ×256 计 */
static struct { int32_t x, y, vx, vy, rot, vr; int face, b; bool land; } coins[3];
static int ncoin;
int fc_shake_force[6] = {0};               /* 测试钩子：非 0 时第 i 爻固定为该值 */

#define TURN256 (ANG_TURN * 256)
static void enter(void) { nl = 0; ph = H_READY; ncoin = 0; msg = NULL; card = 0; wh = 0; at = 0; read_page = 0; }

static void key(int k, bool d) {
  if (ph == H_DONE && card && d && (k == K_UP || k == K_DOWN)) { read_page = (read_page + (k == K_DOWN ? 1 : read_pages - 1)) % read_pages; fc_sfx_play(SFX_TICK); return; }
  if (k != K_OK) return;
  if (d) {
    if (ph == H_DONE) { if (!card) { card = 1; ct = (int)VT; read_page = 0; fc_sfx_play(SFX_WHOOSH); } else { card = 0; nl = 0; ph = H_READY; } return; }
    if (ph == H_READY) { ph = H_HOLD; hs = (int)VT; lr = 0; msg = NULL; }
  } else if (ph == H_HOLD) {
    int h = (int)VT - hs;
    if (h < 250) { ph = H_READY; msg = "按住久一点再松开"; return; }
    /* 龟甲口：N-SHELL-TOSS 120px 画在 (60,20)，锚点 (64.8,71.5)，第 8 帧出手 */
    int mx = 60 * 256 + 64 * 256 + 205, my = 20 * 256 + 71 * 256 + 128;
    toss_t = (int)VT; launch_at = (int)VT + 8 * 1000 / 12;
    int force = fc_shake_force[nl];
    for (int i = 0; i < 3; i++) {
      coins[i].x = mx + (i - 1) * 6 * 256; coins[i].y = my;
      coins[i].vx = ((i - 1) * 75 + (R1000() - 500) * 30 / 1000) * 256;
      coins[i].vy = -(60 + R1000() * 50 / 1000) * 256;
      coins[i].rot = 0; coins[i].vr = (16 + R1000() * 10 / 1000) * 163 * 256;
      coins[i].face = R1000() < 500 ? 0 : 1; coins[i].b = 0; coins[i].land = false;
    }
    if (force >= 6 && force <= 9) { int backs = force - 6; for (int i = 0; i < 3; i++) coins[i].face = i < backs; }
    ncoin = 3; ph = H_TOSS;
  }
}

static void update(int dt) {
  int t = (int)VT;
  if (ph == H_HOLD && t - lr > 90) { lr = t; fc_sfx_play(SFX_RATTLE); }
  if (ph == H_TOSS && t >= launch_at) {
    if (!wh) { wh = 1; fc_sfx_play(SFX_WHOOSH); }
    bool all = true;
    for (int i = 0; i < 3; i++) {
      if (coins[i].land) continue;
      all = false;
      coins[i].vy += 900 * 256 * dt / 1000;
      coins[i].x += coins[i].vx * dt / 1000; coins[i].y += coins[i].vy * dt / 1000;
      coins[i].rot += (int32_t)((int64_t)coins[i].vr * dt / 1000);
      if (coins[i].y >= 196 * 256 && coins[i].vy > 0) {
        coins[i].y = 196 * 256; fc_sfx_play(SFX_CLINK);
        if (coins[i].b < 1) { coins[i].vy = -coins[i].vy * 32 / 100; coins[i].vx /= 2; coins[i].b++; }
        else {
          coins[i].land = true;
          int32_t tgt = coins[i].face ? TURN256 / 2 : 0, d = coins[i].rot - tgt;
          int32_t n = (d >= 0 ? d + TURN256 / 2 : d - TURN256 / 2) / TURN256;
          coins[i].rot = tgt + n * TURN256;
        }
      }
    }
    if (all) {
      if (!at) at = t;
      if (t - at > 500) {
        at = 0;
        int sum = 0; for (int i = 0; i < 3; i++) sum += coins[i].face ? 3 : 2;
        lines[nl].v = (uint8_t)sum; lines[nl].t = t; nl++;
        fc_sfx_play(SFX_LINE); wh = 0;
        if (nl == 6) { ph = H_REVEAL; rt = t; fc_sfx_play(SFX_REVEAL); } else ph = H_READY;
      }
    }
  }
  if (ph == H_REVEAL && t - rt > 1400) { ph = H_DONE; burst(120, 110, 30, 0); }
}

/* 铜钱：16 帧翻转图覆盖 0..π，另半圈镜像 */
static void coin(int x, int y, int rot) {
  fc_ellipse(x, 206, 10, 3, 0, 89);
  const fc_img_t *im = prop("coin-flip"); if (!im) return;
  int k = (int)(((int64_t)(rot % TURN256 + TURN256) % TURN256 * 32 + TURN256 / 2) / TURN256) % 32;
  int f = k < 16 ? k : (32 - k > 15 ? 15 : 32 - k);
  fc_img(im, f, x - 11, y - 11, 255);
}

static const char *const RULE[6] = {"六爻皆静，看本卦卦辞", "一爻动，看本卦动爻的爻辞", "二爻动，看本卦两动爻，以上爻为主", "三爻动，看本卦和变卦卦辞，以本卦为主",
                                    "四爻动，看变卦两静爻，以下爻为主", "五爻动，看变卦静爻的爻辞"};
static void draw_card(void) {
  uint8_t v[6]; for (int i = 0; i < 6; i++) v[i] = lines[i].v;
  fc_liuyao_read_t rd; fc_liuyao_read(v, &rd);
  char title[64], subtitle[64] = "", body[2048] = "";
  snprintf(title, sizeof title, "本卦 %s", FT_HEX_NAME[rd.ben]);
  if (rd.moving) snprintf(subtitle, sizeof subtitle, "变卦 %s", FT_HEX_NAME[rd.bian]);
  const char *rule = rd.moving < 6 ? RULE[rd.moving] : rd.ben == 0 ? "六爻皆动，乾卦看“用九”" : rd.ben == 63 ? "六爻皆动，坤卦看“用六”" : "六爻皆动，看变卦卦辞";
  fc_body_add(body, sizeof body, "%s\n", rule);
  for (int i = 0; i < rd.n; i++) {
    const fc_yao_item_t *it = &rd.it[i];
    if (it->kind == 0) fc_body_add(body, sizeof body, "%s · 卦辞\n%s\n", FT_HEX_NAME[it->hex], fc_txt("zy_gua", it->hex));
    else {
      const char *txt = it->kind == 1 ? fc_txt("zy_line", it->hex * 6 + it->pos) : fc_txt("zy_special", it->hex);
      const char *tab = strchr(txt, '\t'); int n = tab ? (int)(tab - txt) : 0;
      fc_body_add(body, sizeof body, "%s · %.*s\n%s\n", FT_HEX_NAME[it->hex], n, txt, tab ? tab + 1 : txt);
    }
  }
  fc_body_add(body, sizeof body, "白话\n%s", fc_txt("zy_bh", rd.main));
  read_pages = fc_read_card(title, subtitle, body, read_page);
}

static int hex_of(int changed) {
  int yy[6];
  for (int i = 0; i < 6; i++) { int v = lines[i].v; yy[i] = changed ? (v == 6 ? 1 : v == 9 ? 0 : v == 7) : (v == 7 || v == 9); }
  static const int8_t TK[8] = {7, 6, 5, 4, 3, 2, 1, 0};     /* 位 (下,中,上) → 乾兑离震巽坎艮坤 序号 */
  int lo = TK[yy[0] * 4 + yy[1] * 2 + yy[2]], up = TK[yy[3] * 4 + yy[4] * 2 + yy[5]];
  return up * 8 + lo;
}

static void draw(void) {
  int t = (int)VT;
  int h = ph == H_HOLD ? t - hs : 0, amp = h / 120 < 6 ? h / 120 : 6;
  int jx = ph == H_HOLD ? RS(amp * 100) / 100 : 0, jy = ph == H_HOLD ? RS(amp * 60) / 100 : 0;
  if (ph != H_REVEAL && ph != H_DONE) {
    if (ph == H_TOSS) nabo("N-SHELL-TOSS", 120, 120, 80, t - toss_t);
    else if (ph == H_HOLD) nabo("N-SHELL", 120, 120 + jx, 80 + jy, t - hs);
    else nabo_frame("N-SHELL", 120, 120, 80, 0);
    if (ph == H_HOLD) { fc_rect(70, 150, 100, 5, C_BG2, 255); fc_rect(70, 150, h * 100 / 1200 < 100 ? h * 100 / 1200 : 100, 5, C_MAG, 255); }
  }
  for (int i = 0; i < ncoin; i++)
    if ((ph == H_TOSS && t >= launch_at) || ph == H_READY) coin(coins[i].x / 256, coins[i].y / 256, coins[i].rot);
  fc_rect(64, 212, 1, 94, C_BG2, 255);
  static const char *const POS[6] = {"初", "二", "三", "四", "五", "上"};
  for (int i = 0; i < 6; i++) T(&F_SANS9, POS[i], 54, 296 - i * 14, C_HINT, AL_R);
  static const char *const VN[10] = {"", "", "", "", "", "", "老阴", "少阳", "少阴", "老阳"};
  for (int i = 0; i < nl; i++) {
    int y = 296 - i * 14, p = ease_out(fx_prog(t - lines[i].t, 260)), w = 100 * p / 1000, v = lines[i].v;
    bool yang = v == 7 || v == 9, mv = v == 6 || v == 9; uint16_t col = mv ? C_GOLD : C_CYAN;
    if (yang) fc_rect(120 - w / 2, y - 2, w, 4, col, 255);
    else { int s = w * 42 / 100; fc_rect(120 - w / 2, y - 2, s, 4, col, 255); fc_rect(120 + w / 2 - s, y - 2, s, 4, col, 255); }
    if (mv && p > 900) {
      if (v == 9) fc_ring(184, y, 4, 1, C_GOLD);
      else { fc_line(180, y - 4, 188, y + 4, 1, C_GOLD); fc_line(188, y - 4, 180, y + 4, 1, C_GOLD); }
    }
    T(&F_SANS9, VN[v], 210, y, mv ? C_GOLD : C_DIM, AL_C);
  }
  if (ph == H_READY) {
    if (msg) T(&F_SANS11, msg, 120, 166, C_MAG, AL_C);
    else TF(&F_SANS11, 120, 166, C_DIM, AL_C, "长按 OK 摇卦 · 第 %d 爻", nl + 1);
  }
  if (ph == H_DONE && card) { draw_card(); return; }
  if (ph == H_REVEAL || ph == H_DONE) {
    bool mv = false; for (int i = 0; i < 6; i++) mv |= lines[i].v == 6 || lines[i].v == 9;
    int e = t - rt;
    T(&F_SANS11, "本卦", 120, 58, C_DIM, AL_C);
    glitch(&F_DISP24, FT_HEX_NAME[hex_of(0)], 120, 94, C_WHITE, e < 700 ? 1 : 0);
    if (mv && e > 700) { char b[64]; snprintf(b, sizeof b, "变卦  %s", FT_HEX_NAME[hex_of(1)]); T(&F_KAI13, b, 120, 136, C_GOLD, AL_C); }
    if (ph == H_DONE) { nabo_then("N-NOD", 56, 204, 166, e - 1400, "N-IDLE"); T(&F_SANS11, "OK 看解卦", 120, 190, C_CYAN, AL_C); }
  }
}
const fc_scene_t SC_SHAKE = {"shake", "摇卦问事", false, enter, key, update, draw};
