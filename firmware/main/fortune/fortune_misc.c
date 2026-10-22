/* 称骨、六爻、随机、今日小票、塔罗手牌 */
#include "fortune_core.h"

/* ---- 称骨（单位：钱）。三源校对见 prototype/fortune-core.js 注释 ---- */
static const uint8_t BONE_Y[60] = {12,9,6,7,12,5,9,8,7,8,15,9,16,8,8,19,12,6,8,7,5,15,6,16,15,7,9,12,10,7,
                                   15,6,5,14,14,9,7,7,9,12,8,7,13,5,14,5,9,17,5,7,12,8,8,6,19,6,8,16,10,6};
static const uint8_t BONE_M[12] = {6,7,18,9,5,16,9,15,18,8,9,5};
static const uint8_t BONE_D[30] = {5,10,8,15,16,15,8,16,8,16,9,17,8,17,10,8,9,18,5,15,10,9,8,9,15,18,7,8,16,6};
static const uint8_t BONE_H[12] = {16,6,7,10,9,16,10,8,8,9,6,6};

void fc_bone(const fc_lunar_t *l, int hour_branch, fc_bone_t *o) {
  o->w_year = BONE_Y[fc_lunar_year_gz(l->year)];
  o->w_month = BONE_M[l->month - 1];        /* 闰月按本月 */
  o->w_day = BONE_D[l->day - 1];
  o->w_hour = BONE_H[hour_branch];
  o->total = (uint8_t)(o->w_year + o->w_month + o->w_day + o->w_hour);
}

int fc_bone_band(int t) {
  static const uint8_t lo[9] = {21,30,36,40,46,50,56,60,66};
  int b = 0;
  for (int i = 0; i < 9; i++) if (t >= lo[i]) b = i;
  return b;
}

/* ---- 六爻 ---- */
static int tri(int b0, int b1, int b2) {      /* 下→上三爻，1=阳 → 乾兑离震巽坎艮坤 0..7 */
  static const int8_t T[8] = {7, 3, 5, 1, 6, 2, 4, 0}; /* 索引 b0 | b1<<1 | b2<<2 */
  return T[b0 | (b1 << 1) | (b2 << 2)];
}
int fc_coin_line(const uint8_t backs[3]) { return 6 + backs[0] + backs[1] + backs[2]; }

void fc_liuyao(const uint8_t v[6], int *ben, int *bian, bool *moving) {
  int y[6], c[6]; bool mv = false;
  for (int i = 0; i < 6; i++) {
    y[i] = (v[i] == 7 || v[i] == 9);
    c[i] = v[i] == 6 ? 1 : v[i] == 9 ? 0 : y[i];
    if (v[i] == 6 || v[i] == 9) mv = true;
  }
  *ben = tri(y[3], y[4], y[5]) * 8 + tri(y[0], y[1], y[2]);
  *bian = tri(c[3], c[4], c[5]) * 8 + tri(c[0], c[1], c[2]);
  *moving = mv;
}

void fc_liuyao_read(const uint8_t v[6], fc_liuyao_read_t *o) {
  int ben, bian; bool mvf; int mv[6], st[6], k = 0, s = 0;
  fc_liuyao(v, &ben, &bian, &mvf);
  for (int i = 0; i < 6; i++) { if (v[i] == 6 || v[i] == 9) mv[k++] = i; else st[s++] = i; }
  o->ben = (int8_t)ben; o->bian = (int8_t)bian; o->moving = (int8_t)k;
  fc_yao_item_t *it = o->it;
#define SET(j, kd, hx, ps) (it[j].kind = (int8_t)(kd), it[j].hex = (int8_t)(hx), it[j].pos = (int8_t)(ps))
  switch (k) {
    case 0: o->n = 1; SET(0, 0, ben, -1); break;
    case 1: o->n = 1; SET(0, 1, ben, mv[0]); break;
    case 2: o->n = 2; SET(0, 1, ben, mv[1]); SET(1, 1, ben, mv[0]); break;   /* 以上爻为主 */
    case 3: o->n = 2; SET(0, 0, ben, -1); SET(1, 0, bian, -1); break;       /* 以本卦为主 */
    case 4: o->n = 2; SET(0, 1, bian, st[0]); SET(1, 1, bian, st[1]); break; /* 以下爻为主 */
    case 5: o->n = 1; SET(0, 1, bian, st[0]); break;
    default:
      o->n = 1;
      if (ben == 0 || ben == 63) SET(0, 2, ben, -1); else SET(0, 0, bian, -1);
  }
#undef SET
  o->main = it[0].hex;
}

/* ---- 随机：mulberry32 ---- */
uint32_t fc_rng_u32(fc_rng_t *r) {
  r->a += 0x6D2B79F5u;
  uint32_t t = (r->a ^ (r->a >> 15)) * (1u | r->a);
  t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
  return t ^ (t >> 14);
}
int fc_rng_int(fc_rng_t *r, int n) { return (int)(((uint64_t)fc_rng_u32(r) * (uint32_t)n) >> 32); }

uint32_t fc_daily_seed(int y, int m, int d, int py, int pm, int pd, int ph, int pg) {
  uint32_t a = (uint32_t)(y * 10000 + m * 100 + d) * 0x9E3779B1u
             ^ (uint32_t)(py * 10000 + pm * 100 + pd) * 0x85EBCA77u
             ^ (uint32_t)(ph * 2 + pg + 1) * 0xC2B2AE3Du;
  a ^= a >> 16; a *= 0x7FEB352Du; a ^= a >> 15;
  return a;
}

static void pick_n(fc_rng_t *r, int n, int k, int8_t *out) {
  int8_t pool[32];
  for (int i = 0; i < n; i++) pool[i] = (int8_t)i;
  for (int j = 0; j < k; j++) {
    int idx = fc_rng_int(r, n - j);
    out[j] = pool[idx];
    for (int i = idx; i < n - j - 1; i++) pool[i] = pool[i + 1];
  }
}

/* 观音灵签签级 0上 1中 2下（由 guanyin_lots.json 生成） */
static const uint8_t LOT_LEVEL[FC_LOT_COUNT] = {0,2,2,0,1,1,2,0,1,1,0,0,1,1,1,1,1,0,1,1,0,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,1,0,0,1,0,1,0,1,1,1,0,1,1,2,1,1,1,1,1,2,1,1,1,2,2,2,0,1,1,2,1,1,0,2,1,1,1,0,1,0,0,1,1,2,1,0,1,1,0,0,1,0,1,2,1,0,1,2,2,2};
int fc_lot_level(int no) { return (no >= 1 && no <= FC_LOT_COUNT) ? LOT_LEVEL[no - 1] : -1; }

#define FC_YI_COUNT 6
#define FC_JI_COUNT 5
void fc_daily_ticket(uint32_t seed, int sign, fc_ticket_t *o) {
  fc_rng_t r = {seed};
  o->sign = (int8_t)sign;
  o->sign_stars = (int8_t)(2 + fc_rng_int(&r, 4));
  o->tarot = (int8_t)fc_rng_int(&r, 22);
  o->tarot_rev = fc_rng_int(&r, 10) < 3;
  o->lot_no = (int16_t)(1 + fc_rng_int(&r, FC_LOT_COUNT));
  o->hex_up = (int8_t)fc_rng_int(&r, 8);
  o->hex_lo = (int8_t)fc_rng_int(&r, 8);
  pick_n(&r, FC_YI_COUNT, 3, o->yi);
  pick_n(&r, FC_JI_COUNT, 2, o->ji);
  o->serial = (int16_t)(1 + fc_rng_int(&r, 9999));
  o->lot_level = (int8_t)fc_lot_level(o->lot_no);
}

void fc_tarot_hand(fc_rng_t *r, int8_t hand[7], bool *rev) {
  pick_n(r, 22, 7, hand);
  *rev = fc_rng_int(r, 10) < 3;
}
