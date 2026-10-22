/* 紫微斗数（原型 S.ziwei）、奇门遁甲（原型 S.qimen） */
#include "fc_ui.h"
#include <stdio.h>
#include <string.h>

#define C_FRAME RGB(0x3A, 0x2D, 0x6A)
#define C_CARD_RED RGB(0x8A, 0x2E, 0x1E)
#define C_CARD_TXT RGB(0x3A, 0x24, 0x14)
#define C_CARD_FOOT RGB(0x9A, 0x7A, 0x52)
#define C_RULE RGB(0xC9, 0xB4, 0x8C)

/* ================= 紫微 ================= */
enum { Z_CONFIRM, Z_CHART, Z_DETAIL };
static int zph, zct, zdt, cur, lk, dd_;
static int zpage, zpages = 1;
static fc_ziwei_t Z;
static struct { int8_t i, j, s; int16_t at; bool d; } fly[14];
static int nfly;
/* 地支 → 4×4 格位置 */
static const int8_t BPOS[12][2] = {{2, 3}, {1, 3}, {0, 3}, {0, 2}, {0, 1}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {3, 1}, {3, 2}, {3, 3}};
static void cell(int i, int *x, int *y) { int b = (i + 2) % 12; *x = 4 + BPOS[b][0] * 58; *y = 38 + BPOS[b][1] * 62; }
static uint16_t mutc(int m) { return m == 0 ? C_GOLD : m == 1 ? C_MAG : m == 2 ? C_CYAN : C_RED; }
static int majors_in(int pal, int8_t *out) { int n = 0; for (int s = 0; s < ZW_MAJOR_COUNT; s++) if (Z.major_at[s] == pal) out[n++] = (int8_t)s; return n; }
static int minors_in(int pal, int8_t *out) { int n = 0; for (int s = 0; s < ZM_MINOR_COUNT; s++) if (Z.minor_at[s] == pal) out[n++] = (int8_t)s; return n; }
static const char *star_name(int code) { return code >= 100 ? FT_ZW_MINOR[code - 100] : FT_ZW_MAJOR[code]; }

static void zw_enter(void) { zph = Z_CONFIRM; }
static void zw_start(void) {
  if (!fc_ziwei(PF.y, PF.m, PF.d, PF.h, &Z)) return;
  zph = Z_CHART; zct = (int)VT; cur = Z.soul; lk = 0; dd_ = 0; nfly = 0;
  for (int i = 0; i < 12; i++) {
    int8_t ms[14]; int n = majors_in(i, ms);
    for (int j = 0; j < n && nfly < 14; j++) { fly[nfly].i = (int8_t)i; fly[nfly].j = (int8_t)j; fly[nfly].s = ms[j]; fly[nfly].d = false; nfly++; }
  }
  for (int k = 0; k < nfly; k++) fly[k].at = (int16_t)(1300 + k * 90);
}
static void zw_key(int k, bool d) {
  if (!d) return;
  if (zph == Z_CONFIRM) {
    if (k == K_OK) { fc_sfx_play(SFX_ENTER); zw_start(); }
    if (k == K_DOWN) { g_birth_ret = "ziwei"; fc_ui_go("birth"); }
    return;
  }
  if (zph == Z_CHART && (int)VT - zct > 3000) {
    if (k == K_DOWN) { cur = (cur + 1) % 12; fc_sfx_play(SFX_TICK); }
    if (k == K_UP) { cur = (cur + 11) % 12; fc_sfx_play(SFX_TICK); }
    if (k == K_OK) { zph = Z_DETAIL; zdt = (int)VT; zpage = 0; fc_sfx_play(SFX_WHOOSH); }
    return;
  }
  if (zph == Z_DETAIL) {
    if (k == K_OK) zph = Z_CHART;
    if (k == K_UP || k == K_DOWN) { zpage = (zpage + (k == K_DOWN ? 1 : zpages - 1)) % zpages; fc_sfx_play(SFX_TICK); }
  }
}
static void zw_update(int dt) {
  (void)dt;
  if (zph != Z_CHART) return;
  int t = (int)VT - zct, l = t / 50 + 1; if (l > 12) l = 12;
  if (l > lk) { lk = l; fc_sfx_play(SFX_TICK); }
  for (int k = 0; k < nfly; k++) if (!fly[k].d && t > fly[k].at + 400) { fly[k].d = true; fc_sfx_play(SFX_TICK); }
  if (t > 2900 && !dd_) { dd_ = 1; fc_sfx_play(SFX_REVEAL); }
}
/* 原型 ziweiRead：解读句子 */
static int zw_read(int idx, const char **L, char buf[][64], int max) {
  int n = 0, pn = Z.palace_name[idx];
  int8_t ms[14]; int k = majors_in(idx, ms); bool borrow = false;
  if (!k) { k = majors_in((idx + 6) % 12, ms); borrow = true; }
  if (pn == 7) { L[0] = "疾厄宫只看星曜分布，"; L[1] = "这里不做健康方面的判断。"; return 2; }
  int fi = FT_ZW_FIELD_IDX[pn];
  if (borrow && n < max) L[n++] = "空宫，借对宫星曜来看。";
  for (int i = 0; i < k && n < max; i++) {
    int s = ms[i];
    snprintf(buf[n], 64, "%s：%s。", FT_ZW_FIELD_NAME[pn], FT_ZW_TEXT[s * 4 + fi]); L[n] = buf[n]; n++;
    if (Z.major_bright[s] && !borrow && n < max) L[n++] = FT_ZW_BRIGHT[Z.major_bright[s]];
    if (Z.major_mut[s] >= 0 && n < max) L[n++] = FT_ZW_MUT[Z.major_mut[s]];
  }
  int8_t mn[14]; int m = minors_in(idx, mn);
  for (int i = 0; i < m && n < max; i++)
    if (Z.minor_mut[mn[i]] >= 0) { snprintf(buf[n], 64, "%s%s", FT_ZW_MINOR[mn[i]], FT_ZW_MUT[Z.minor_mut[mn[i]]]); L[n] = buf[n]; n++; }
  return n;
}
static void zw_draw(void) {
  if (zph == Z_CONFIRM) { draw_confirm("紫微斗数"); return; }
  char b[160];
  if (zph == Z_DETAIL) {
    int idx = cur; char body[2048] = "", title[96];
    snprintf(title, sizeof title, "%s · %s%s%s", FT_ZW_PALACE[Z.palace_name[idx]], FT_GAN[Z.palace_stem[idx]], FT_ZHI[(idx + 2) % 12], idx == Z.body ? " · 身宫" : "");
    int8_t ms[14]; int k = majors_in(idx, ms); if (!k) k = majors_in((idx + 6) % 12, ms);
    b[0] = 0;
    for (int i = 0; i < k; i++) {
      int s = ms[i];
      if (i) strcat(b, "  ");
      strcat(b, FT_ZW_MAJOR[s]); strcat(b, FT_ZW_BRIGHT_NAME[Z.major_bright[s]]);
      if (Z.major_mut[s] >= 0) { strcat(b, "化"); strcat(b, FT_ZW_MUT_NAME[Z.major_mut[s]]); }
    }
    fc_body_add(body, sizeof body, "主星 %s\n", k ? b : "无主星");
    int8_t mn[14]; int m = minors_in(idx, mn);
    strcpy(b, "辅星 "); for (int i = 0; i < m; i++) { if (i) strcat(b, " "); strcat(b, FT_ZW_MINOR[mn[i]]); }
    if (!m) strcat(b, "—");
    fc_body_add(body, sizeof body, "%s\n", b);
    const char *S[12]; char buf[12][64]; int ns = zw_read(idx, S, buf, 12);
    for (int i = 0; i < ns; i++) fc_body_add(body, sizeof body, "%s\n", S[i]);
    zpages = fc_read_card(title, "星曜解读 · 仅供娱乐", body, zpage);
    return;
  }
  int t = (int)VT - zct;
  for (int k = 0; k < 12; k++) {
    if (t < k * 50) continue;
    int i = (Z.soul + k) % 12, x, y; cell(i, &x, &y);
    bool on = t > 3000 && i == cur, soul = i == Z.soul;
    fc_frame(x, y, 58, 62, on ? 2 : 1, on ? C_GOLD : (soul && t > 3000) ? C_MAG : C_FRAME);
    if (t > 600 + k * 50) {
      T(&F_SANS9, FT_ZW_PALACE[Z.palace_name[i]], x + 29, y + 52, soul ? C_MAG : C_WHITE, AL_C);
      if (i == Z.body) T(&F_SANS9, "身", x + 54, y + 12, C_CYAN, AL_R);
    }
  }
  const int cx = 120, cy = 164;
  for (int k = 0; k < nfly; k++) {
    if (t < fly[k].at) continue;
    int x0, y0; cell(fly[k].i, &x0, &y0);
    int tx = x0 + 12 + fly[k].j * 22, ty = y0 + 11, p = ease_out(fx_prog(t - fly[k].at, 400));
    int x = fx_lerp(cx, tx, p), y = fx_lerp(cy, ty, p), s = fly[k].s, br = Z.major_bright[s];
    if (p < 1000) { int q = p - 250 < 0 ? 0 : p - 250; fc_line(fx_lerp(cx, tx, q), fx_lerp(cy, ty, q), x, y, 1, RGB(0x21, 0x7C, 0x7E)); }
    uint16_t col = p < 1000 ? C_WHITE : (br == 1 || br == 2) ? C_GOLD : (br == 6 || br == 7) ? RGB(0x8F, 0x82, 0xB8) : C_CYAN;
    if (p >= 1000 && Z.major_mut[s] >= 0) col = mutc(Z.major_mut[s]);
    const char *nm = FT_ZW_MAJOR[s];
    fc_text_n(&F_KAI10, x, y, nm, 3, col, AL_C);
    fc_text_n(&F_KAI10, x, y + 17, nm + 3, 3, col, AL_C);
  }
  if (t < 2900) { nabo("N-CALC", 100, cx, cy - 10, (int)VT); T(&F_SANS11, "排盘中…", cx, cy + 50, C_DIM, AL_C); }
  else {
    T(&F_KAI10, FT_ZW_FIVE[Z.five], cx, 111, C_DIM, AL_C);
    TF(&F_SANS9, cx, 131, C_DIM, AL_C, "命主%s", star_name(Z.soul_star));
    TF(&F_SANS9, cx, 151, C_DIM, AL_C, "身主%s", star_name(Z.body_star));
    G(&F_DISP24, FT_ZW_PALACE[Z.palace_name[cur]], cx, 178, C_WHITE, cur == Z.soul ? C_MAG : C_FRAME);
    int8_t ms[14]; int k = majors_in(cur, ms);
    b[0] = 0;
    for (int i = 0; i < k; i++) { if (i) strcat(b, " · "); strcat(b, FT_ZW_MAJOR[ms[i]]); if (Z.major_mut[ms[i]] >= 0) strcat(b, FT_ZW_MUT_NAME[Z.major_mut[ms[i]]]); }
    T(&F_KAI10, k ? b : "空宫借对宫", cx, 202, C_GOLD, AL_C);
    T(&F_SANS9, "OK 解读", cx, 222, C_CYAN, AL_C);
  }
}
const fc_scene_t SC_ZIWEI = {"ziwei", "紫微斗数", false, zw_enter, zw_key, zw_update, zw_draw};

/* ================= 奇门 ================= */
enum { Q_ASK, Q_ANIM, Q_READ };
static int qph, qsel, qat, qrt, hop, lockf[3];
static int qpage, qpages = 1;
static fc_qimen_t Q;
static fc_qimen_read_t RD;
static fc_time_t now;
static const int8_t LS[10][2] = {{0, 0}, {1, 2}, {2, 0}, {0, 1}, {0, 0}, {1, 1}, {2, 2}, {2, 1}, {0, 2}, {1, 0}};
static const int8_t URING[8] = {4, 9, 2, 7, 6, 1, 8, 3};      /* 屏幕上顺时针 */
static const char *const DOOR1[10] = {"", "休", "死", "伤", "杜", "", "开", "惊", "生", "景"};
static const char *const ASKHINT[5] = {"看生门", "看开门", "看六合", "看景门", "看值使门"};
static void cc(int n, int *x, int *y) { *x = 12 + LS[n][0] * 72; *y = 56 + LS[n][1] * 72; }
/* 外圈 8 宫之间插值：f 为千分之一格 */
static void rp(int f, int *x, int *y) {
  f = ((f % 8000) + 8000) % 8000;
  int i = f / 1000, u = f % 1000, ax, ay, bx, by;
  cc(URING[i], &ax, &ay); cc(URING[(i + 1) % 8], &bx, &by);
  *x = fx_lerp(ax, bx, u); *y = fx_lerp(ay, by, u);
}
static int uring_idx(int n) { for (int i = 0; i < 8; i++) if (URING[i] == n) return i; return 0; }
/* 天盘干：坤二宫的原干带中五干 */
static void heaven_str(int p, char *o) {
  strcpy(o, Q.heaven[p] >= 0 ? FT_GAN[Q.heaven[p]] : "");
  if (Q.star[p] == 2 && Q.heaven2_extra >= 0) strcat(o, FT_GAN[Q.heaven2_extra]);
}
static void qm_enter(void) { qph = Q_ASK; fc_now(&now); }
static void qm_start(void) {
  fc_qimen(now.y, now.m, now.d, now.hh, now.mm, &Q);
  fc_qimen_read(&Q, qsel, &RD);
  qph = Q_ANIM; qat = (int)VT; lockf[0] = lockf[1] = lockf[2] = 0; hop = 0;
}
static void qm_key(int k, bool d) {
  if (!d) return;
  if (qph == Q_ASK) {
    if (k == K_UP) { qsel = (qsel + 4) % 5; fc_sfx_play(SFX_TICK); }
    if (k == K_DOWN) { qsel = (qsel + 1) % 5; fc_sfx_play(SFX_TICK); }
    if (k == K_OK) { fc_sfx_play(SFX_ENTER); qm_start(); }
    return;
  }
  if (qph == Q_ANIM && k == K_OK && (int)VT - qat > 4200) { qph = Q_READ; qrt = (int)VT; qpage = 0; fc_sfx_play(SFX_WHOOSH); return; }
  if (qph == Q_READ) {
    if (k == K_OK) qm_enter();
    if (k == K_UP || k == K_DOWN) { qpage = (qpage + (k == K_DOWN ? 1 : qpages - 1)) % qpages; fc_sfx_play(SFX_TICK); }
  }
}
static void qm_update(int dt) {
  (void)dt;
  if (qph != Q_ANIM) return;
  int t = (int)VT - qat;
  for (int i = 1; i <= 9; i++) if (t > 500 + (i - 1) * 130 && hop < i) { hop = i; fc_sfx_play(SFX_TICK); }
  if (t > 2500 && !lockf[0]) { lockf[0] = 1; fc_sfx_play(SFX_LOCK); g_shake = 300; }
  if (t > 3400 && !lockf[1]) { lockf[1] = 1; fc_sfx_play(SFX_LOCK); g_shake = 300; }
  if (t > 4100 && !lockf[2]) { lockf[2] = 1; fc_sfx_play(SFX_REVEAL); }
}
static void qm_key_name(char *o) {
  if (qsel == 4) sprintf(o, "值使（%s）", FT_QM_DOOR[Q.zhifu_src]);
  else strcpy(o, FT_QM_KEY[qsel]);
}
static void qm_draw(void) {
  char b[160], kn[32];
  if (qph == Q_ASK) {
    G(&F_DISP24, "问什么事？", 120, 46, C_WHITE, C_MAG);
    for (int i = 0; i < 5; i++) {
      int y = 88 + i * 34; bool on = i == qsel;
      if (on) { fc_rect(30, y - 14, 180, 28, C_BG2, 255); fc_frame(30, y - 14, 180, 28, 1, C_MAG); }
      T(&F_KAI15, FT_QM_ASK[i], 60, y, on ? C_GOLD : C_WHITE, AL_L);
      T(&F_SANS9, ASKHINT[i], 196, y, on ? C_CYAN : C_DIM, AL_R);
    }
    TF(&F_KAI10, 120, 270, C_DIM, AL_C, "起局时间 %d.%02d.%02d %s时", now.y, now.m, now.d, FT_ZHI[fc_hour_branch(now.hh)]);
    char dg[8], hg[8]; gz_str(fc_day_gz(now.y, now.m, now.d), dg); gz_str(fc_hour_gz(now.y, now.m, now.d, now.hh), hg);
    TF(&F_KAI10, 120, 286, C_DIM, AL_C, "%s日 %s时 · 按此刻起局", dg, hg);
    T(&F_SANS9, "↑↓ 选择  OK 起局", 120, 306, C_HINT, AL_C);
    return;
  }
  qm_key_name(kn);
  int p = RD.palace;
  if (qph == Q_READ) {
    char body[2048] = "";
    char dg[8], hg[8]; gz_str(Q.day_gz, dg); gz_str(Q.hour_gz, hg);
    fc_body_add(body, sizeof body, "%s日 %s时\n%s遁%s局%s元\n", dg, hg, Q.yang ? "阳" : "阴", FT_CN[Q.ju], Q.yuan == 0 ? "上" : Q.yuan == 1 ? "中" : "下");
    char L3[3][96], S3[3][96];
    snprintf(L3[0], 96, "用神：%s", kn);
    snprintf(L3[1], 96, "落宫：%s%d宫（%s）", FT_QM_GUA[p], p, FT_QM_DIR[p]);
    snprintf(L3[2], 96, "方位：办这件事，可以多往%s走动。", FT_QM_DIR[p]);
    snprintf(S3[0], 96, "%s：%s", FT_QM_DOOR[Q.door[p]], FT_QM_DOOR_TEXT[Q.door[p]]);
    snprintf(S3[1], 96, "%s：%s", FT_QM_STAR[Q.star[p]], FT_QM_STAR_TEXT[Q.star[p]]);
    snprintf(S3[2], 96, "%s：%s", FT_QM_GODS[Q.god[p]], FT_QM_GOD_TEXT[Q.god[p]]);
    for (int i = 0; i < 3; i++) fc_body_add(body, sizeof body, "%s\n", L3[i]);
    for (int i = 0; i < 3; i++) fc_body_add(body, sizeof body, "%s\n", S3[i]);
    qpages = fc_read_card(FT_QM_ASK[qsel], FT_QM_VERDICT[RD.verdict], body, qpage);
    return;
  }
  int t = (int)VT - qat, gp = fx_prog(t, 500), L = 216 * gp / 1000;
  for (int i = 0; i <= 3; i++) { fc_rect(12, 56 + i * 72, L, 1, C_FRAME, 255); fc_rect(12 + i * 72, 56, 1, L, C_FRAME, 255); }
  for (int n = 1; n <= 9; n++) {
    if (t < 500 + (n - 1) * 130) continue;
    int x, y; cc(n, &x, &y);
    TF(&F_SANS9, x + 5, y + 64, C_DIM, AL_L, "%d%s", n, FT_QM_GUA[n]);
    if (Q.earth[n] >= 0) T(&F_KAI10, FT_GAN[Q.earth[n]], x + 66, y + 64, RGB(0xB9, 0xB0, 0xD8), AL_R);
  }
  if (t > 500 && t < 1700) {
    int f = (t - 500) * 1000 / 130, i = f / 1000 < 8 ? f / 1000 : 8, u = f - i * 1000, ax, ay, bx, by;
    cc(i + 1, &ax, &ay); cc(i + 2 <= 9 ? i + 2 : 9, &bx, &by);
    int x = fx_lerp(ax, bx, i < 8 ? u : 0) + 36, y = fx_lerp(ay, by, i < 8 ? u : 0) + 36;
    fc_disc(x, y, 5, C_GOLD, 255); fc_disc(x, y, 2, 0xFFFF, 255);
  }
  if (t > 1700) {
    int pr = ease_back(fx_prog(t - 1700, 800));
    for (int i = 0; i < 8; i++) {
      int x, y; rp(i * 1000 + Q.ring_shift * pr, &x, &y);
      T(&F_KAI13, FT_QM_STAR[URING[i]], x + 36, y + 30, C_CYAN, AL_C);
    }
    if (t > 2500) for (int i = 0; i < 8; i++) { int n = URING[i], x, y; cc(n, &x, &y); heaven_str(n, b); T(&F_KAI10, b, x + 66, y + 12, C_WHITE, AL_R); }
  }
  if (t > 2600) {
    int pr = ease_back(fx_prog(t - 2600, 800));
    for (int i = 0; i < 8; i++) {
      int x, y; rp(i * 1000 + Q.door_shift * pr, &x, &y);
      snprintf(b, sizeof b, "%s门", DOOR1[URING[i]]);
      T(&F_KAI13, b, x + 36, y + 48, C_GOLD, AL_C);
    }
  }
  if (t > 3500) {
    int gi = uring_idx(Q.zhifu_at);
    for (int j = 0; j < 8; j++) {
      if (t < 3500 + j * 60) break;
      int n = URING[((Q.yang ? gi + j : gi - j) % 8 + 8) % 8], x, y; cc(n, &x, &y);
      T(&F_SANS9, FT_QM_GODS[Q.god[n]], x + 5, y + 12, C_MAG, AL_L);
    }
  }
  int zx, zy; cc(5, &zx, &zy);
  if (t < 2600) nabo("N-CALC", 56, zx + 36, zy + 34, (int)VT); else T(&F_KAI13, "中五", zx + 36, zy + 34, C_DIM, AL_C);
  if (t > 4100) TF(&F_KAI13, 120, 40, C_WHITE, AL_C, "%s遁%s局 %s元 · %s", Q.yang ? "阳" : "阴", FT_CN[Q.ju], Q.yuan == 0 ? "上" : Q.yuan == 1 ? "中" : "下", FT_JIEQI[Q.jieqi]);
  else T(&F_KAI13, "起局中…", 120, 40, C_DIM, AL_C);
  if (t > 4100) {
    int ux, uy; cc(p, &ux, &uy);
    int pl = 500 + fx_sin((int)((int64_t)VT * 163 / 160)) * 500 / 16384, a = 400 + 600 * pl / 1000;
    fc_gfx_clip(ux + 1, uy + 1, 71, 71);
    fc_rect(ux + 1, uy + 1, 71, 2, C_GOLD, (uint8_t)(a * 255 / 1000)); fc_rect(ux + 1, uy + 70, 71, 2, C_GOLD, (uint8_t)(a * 255 / 1000));
    fc_rect(ux + 1, uy + 3, 2, 67, C_GOLD, (uint8_t)(a * 255 / 1000)); fc_rect(ux + 70, uy + 3, 2, 67, C_GOLD, (uint8_t)(a * 255 / 1000));
    fc_gfx_clip(0, 0, 0, 0);
    TF(&F_KAI13, 120, 288, C_GOLD, AL_C, "用神 %s 落%s%d宫 · %s", kn, FT_QM_GUA[p], p, FT_QM_DIR[p]);
    T(&F_SANS9, "OK 看解读", 120, 306, C_CYAN, AL_C);
  }
}
const fc_scene_t SC_QIMEN = {"qimen", "奇门遁甲", false, qm_enter, qm_key, qm_update, qm_draw};
