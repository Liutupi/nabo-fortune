/* 奇门遁甲 · 时家转盘拆补法（对拍 kinqimen 与 prototype/fortune-core.js） */
#include "fortune_core.h"

/* 节气序号（小寒=0）→ 阳遁? 局数[上中下] */
static const int8_t JU[24][3] = {
  {2,8,5},{3,9,6},{8,5,2},{9,6,3},{1,7,4},{3,9,6},{4,1,7},{5,2,8},{4,1,7},{5,2,8},{6,3,9},{9,3,6},
  {8,2,5},{7,1,4},{2,5,8},{1,4,7},{9,3,6},{7,1,4},{6,9,3},{5,8,2},{6,9,3},{5,8,2},{4,7,1},{1,7,4}};
/* 小寒..芒种 阳遁(0..10)，夏至..大雪 阴遁(11..22)，冬至(23) 阳遁 */
static bool is_yang(int jq) { return jq <= 10 || jq == 23; }

static const int8_t QIYI[9] = {4,5,6,7,8,9,3,2,1};      /* 戊己庚辛壬癸丁丙乙 → 天干序号 */
static const int8_t RING[8] = {1,8,3,4,9,2,7,6};
static const int8_t XUN_YI[6] = {4,5,6,7,8,9};           /* 甲子戊 甲戌己 甲申庚 甲午辛 甲辰壬 甲寅癸 */

static int ring_idx(int p) { for (int i = 0; i < 8; i++) if (RING[i] == p) return i; return -1; }
static int g5(int p) { return p == 5 ? 2 : p; }
static int mod(int a, int n) { int r = a % n; return r < 0 ? r + n : r; }

void fc_qimen(int y, int m, int d, int hh, int mm, fc_qimen_t *q) {
  q->jieqi = (int8_t)fc_prev_jieqi(y, m, d, hh, mm);   /* 节气按真实时刻 */
  int dy = y, dm = m, dd = d, h = hh;
  if (hh == 23) { fc_date_from_days(fc_days_from_1900(y, m, d) + 1, &dy, &dm, &dd); h = 0; }
  q->yang = is_yang(q->jieqi);
  int di = fc_day_gz(dy, dm, dd), fh = di - (di % 10) % 5, fz = fh % 12;
  q->yuan = (fz == 0 || fz == 6 || fz == 3 || fz == 9) ? 0 : (fz == 2 || fz == 8 || fz == 5 || fz == 11) ? 1 : 2;
  q->ju = JU[q->jieqi][q->yuan];
  q->day_gz = (int8_t)di;
  int ti = fc_hour_gz(dy, dm, dd, h);
  q->hour_gz = (int8_t)ti;

  int8_t where[10];                                     /* 天干 → 地盘宫 */
  for (int i = 0; i < 10; i++) where[i] = 0;
  for (int i = 0; i < 9; i++) {
    int p = q->yang ? mod(q->ju - 1 + i, 9) + 1 : mod(q->ju - 1 - i, 9) + 1;
    q->earth[p] = QIYI[i]; where[QIYI[i]] = (int8_t)p;
  }
  q->earth[0] = -1;
  int xun = ti / 10, steps = ti % 10, xun_yi = XUN_YI[xun];
  int src_real = where[xun_yi], src = g5(src_real);
  int tg = (ti % 10 == 0) ? xun_yi : ti % 10;
  int dst = g5(where[tg]);
  int shift = mod(ring_idx(dst) - ring_idx(src), 8);
  for (int i = 0; i < 10; i++) { q->star[i] = 0; q->door[i] = 0; q->god[i] = -1; q->heaven[i] = -1; }
  q->heaven2_extra = -1;
  for (int i = 0; i < 8; i++) {
    int p = RING[i], from = RING[mod(i - shift, 8)];
    q->star[p] = (int8_t)from;
    q->heaven[p] = q->earth[from];
    if (from == 2) q->heaven2_extra = q->earth[5];   /* 天禽随天芮：中五干落在天芮所到之宫 */
  }
  q->star[5] = 5;
  int door_dst = g5(q->yang ? mod(src_real - 1 + steps, 9) + 1 : mod(src_real - 1 - steps, 9) + 1);
  int dshift = mod(ring_idx(door_dst) - ring_idx(src), 8);
  for (int i = 0; i < 8; i++) q->door[RING[i]] = RING[mod(i - dshift, 8)];
  int gi = ring_idx(dst);
  for (int k = 0; k < 8; k++) q->god[RING[mod(q->yang ? gi + k : gi - k, 8)]] = (int8_t)k;
  q->zhifu_src = (int8_t)src; q->zhifu_at = (int8_t)dst; q->zhishi_at = (int8_t)door_dst;
  q->ring_shift = (int8_t)shift; q->door_shift = (int8_t)dshift;
}

/* 吉凶分：门（按原宫号）、星（按原宫号）、神（0..7） */
static const int8_t DOOR_SC[10] = {0,2,0,0,1,0,2,0,2,1};  /* 1休2 2死0 3伤0 4杜1 6开2 7惊0 8生2 9景1 */
static const int8_t STAR_SC[10] = {0,0,0,1,2,2,2,0,2,1};  /* 1蓬0 2芮0 3冲1 4辅2 5禽2 6心2 7柱0 8任2 9英1 */
static const int8_t GOD_SC[8] = {2,0,2,2,0,0,1,2};

void fc_qimen_read(const fc_qimen_t *q, int ask, fc_qimen_read_t *o) {
  int p = 0;
  if (ask == 0 || ask == 1 || ask == 3) {
    int want = ask == 0 ? 8 : ask == 1 ? 6 : 9;            /* 生门 开门 景门（原宫号） */
    for (int i = 1; i <= 9; i++) if (i != 5 && q->door[i] == want) p = i;
  } else if (ask == 2) {
    for (int i = 1; i <= 9; i++) if (i != 5 && q->god[i] == 3) p = i; /* 六合 */
  } else p = q->zhishi_at;
  int sc = DOOR_SC[q->door[p]] + STAR_SC[q->star[p]] + GOD_SC[q->god[p]];
  o->palace = (int8_t)p; o->score = (int8_t)sc;
  o->verdict = (int8_t)(sc >= 5 ? 0 : sc >= 3 ? 1 : sc >= 2 ? 2 : 3);
}
