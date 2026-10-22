/* 紫微斗数排盘：命身宫、五行局、十四主星、亮度、四化、十四辅星、命主身主。
 * 规则与数据移植自 iztro 2.6.1（MIT，SylarLong），默认配置：fixLeap=true、年按正月初一。
 * 宫索引 0 = 寅宫，顺时针递增。 */
#include "fortune_core.h"

static int fix12(int i) { i %= 12; return i < 0 ? i + 12 : i; }

/* 亮度：0 无 1庙 2旺 3得 4利 5平 6不 7陷；按宫索引（寅起） */
static const char *const BRIGHT_SRC[ZW_MAJOR_COUNT] = {
  "旺旺得旺庙庙旺旺得旺平庙", "得旺利平庙陷得旺利平庙陷", "旺庙旺旺旺得得平不陷陷不", "得利庙平旺庙得利庙平旺庙",
  "利平平庙陷不旺平平庙旺不", "庙平利陷平利庙平利陷平利", "庙得庙得旺庙得旺庙得庙庙", "旺陷陷陷不不利旺旺庙庙庙",
  "平利庙陷旺庙平利庙陷旺庙", "庙庙陷旺旺不庙庙陷旺旺不", "庙陷得得庙得庙陷得得庙庙", "庙庙庙陷庙旺陷得庙陷庙旺",
  "庙旺庙平旺庙庙旺庙平旺庙", "得陷旺平庙旺得陷旺平庙旺"};

static int bright_code(const char *s, int idx) {
  /* UTF-8 每个汉字 3 字节 */
  const unsigned char *p = (const unsigned char *)s + idx * 3;
  static const char *const K = "庙旺得利平不陷";
  for (int k = 0; k < 7; k++) {
    const unsigned char *q = (const unsigned char *)K + k * 3;
    if (p[0] == q[0] && p[1] == q[1] && p[2] == q[2]) return k + 1;
  }
  return 0;
}

/* 四化（禄权科忌）：主星 0..13；辅星记为 100 + ZM_* */
static const int16_t MUT[10][4] = {
  {ZW_LIANZHEN, ZW_POJUN, ZW_WUQU, ZW_TAIYANG},          /* 甲 */
  {ZW_TIANJI, ZW_TIANLIANG, ZW_ZIWEI, ZW_TAIYIN},        /* 乙 */
  {ZW_TIANTONG, ZW_TIANJI, 100 + ZM_WENCHANG, ZW_LIANZHEN}, /* 丙 */
  {ZW_TAIYIN, ZW_TIANTONG, ZW_TIANJI, ZW_JUMEN},         /* 丁 */
  {ZW_TANLANG, ZW_TAIYIN, 100 + ZM_YOUBI, ZW_TIANJI},    /* 戊 */
  {ZW_WUQU, ZW_TANLANG, ZW_TIANLIANG, 100 + ZM_WENQU},   /* 己 */
  {ZW_TAIYANG, ZW_WUQU, ZW_TAIYIN, ZW_TIANTONG},         /* 庚 */
  {ZW_JUMEN, ZW_TAIYANG, 100 + ZM_WENQU, 100 + ZM_WENCHANG}, /* 辛 */
  {ZW_TIANLIANG, ZW_ZIWEI, 100 + ZM_ZUOFU, ZW_WUQU},     /* 壬 */
  {ZW_POJUN, ZW_JUMEN, ZW_TAIYIN, ZW_TANLANG}};          /* 癸 */

/* 命主（按命宫地支）、身主（按年支），子..亥 */
static const int16_t SOUL_STAR[12] = {ZW_TANLANG, ZW_JUMEN, 100 + ZM_LUCUN, 100 + ZM_WENQU, ZW_LIANZHEN, ZW_WUQU,
                                      ZW_POJUN, ZW_WUQU, ZW_LIANZHEN, 100 + ZM_WENQU, 100 + ZM_LUCUN, ZW_JUMEN};
static const int16_t BODY_STAR[12] = {100 + ZM_HUOXING, ZW_TIANXIANG, ZW_TIANLIANG, ZW_TIANTONG, 100 + ZM_WENCHANG, ZW_TIANJI,
                                      100 + ZM_HUOXING, ZW_TIANXIANG, ZW_TIANLIANG, ZW_TIANTONG, 100 + ZM_WENCHANG, ZW_TIANJI};

bool fc_ziwei(int y, int m, int d, int t, fc_ziwei_t *z) {
  fc_lunar_t l;
  if (t < 0 || t > 11 || !fc_solar_to_lunar(y, m, d, &l)) return false;
  int ygz = fc_lunar_year_gz(l.year), ys = ygz % 10, yb = ygz % 12;
  z->year_stem = (int8_t)ys; z->year_branch = (int8_t)yb;
  int mi = fix12(l.month - 1 + ((l.leap && l.day > 15) ? 1 : 0));
  int soul = fix12(mi - t), body = fix12(mi + t);
  z->soul = (int8_t)soul; z->body = (int8_t)body;
  int start = ((ys % 5) * 2 + 2) % 10;                    /* 五虎遁：寅宫天干 */
  for (int i = 0; i < 12; i++) {
    z->palace_stem[i] = (int8_t)((start + i) % 10);
    z->palace_name[i] = (int8_t)fix12(i - soul);
  }
  /* 五行局：命宫干支纳音 */
  int ss = z->palace_stem[soul], sb = (soul + 2) % 12;
  int idx = ss / 2 + 1 + (sb % 6) / 2 + 1;
  while (idx > 5) idx -= 5;
  static const int8_t FIVE[5] = {3, 4, 2, 6, 5};
  int five = FIVE[idx - 1];
  z->five = (int8_t)five;
  /* 起紫微 */
  int off = -1, q, r;
  do { off++; q = (l.day + off) / five; r = (l.day + off) % five; } while (r);
  q %= 12;
  int zi = q - 1;
  zi = (off % 2 == 0) ? zi + off : zi - off;
  zi = fix12(zi);
  int tf = fix12(12 - zi);
  static const int8_t ZG[6][2] = {{ZW_ZIWEI,0},{ZW_TIANJI,1},{ZW_TAIYANG,3},{ZW_WUQU,4},{ZW_TIANTONG,5},{ZW_LIANZHEN,8}};
  static const int8_t FG[8][2] = {{ZW_TIANFU,0},{ZW_TAIYIN,1},{ZW_TANLANG,2},{ZW_JUMEN,3},{ZW_TIANXIANG,4},{ZW_TIANLIANG,5},{ZW_QISHA,6},{ZW_POJUN,10}};
  for (int i = 0; i < 6; i++) z->major_at[ZG[i][0]] = (int8_t)fix12(zi - ZG[i][1]);
  for (int i = 0; i < 8; i++) z->major_at[FG[i][0]] = (int8_t)fix12(tf + FG[i][1]);
  for (int s = 0; s < ZW_MAJOR_COUNT; s++) { z->major_bright[s] = (int8_t)bright_code(BRIGHT_SRC[s], z->major_at[s]); z->major_mut[s] = -1; }
  /* 辅星 */
  int8_t *mn = z->minor_at;
  mn[ZM_ZUOFU] = (int8_t)fix12(2 + mi);  mn[ZM_YOUBI] = (int8_t)fix12(8 - mi);
  mn[ZM_WENCHANG] = (int8_t)fix12(8 - t); mn[ZM_WENQU] = (int8_t)fix12(2 + t);
  static const int8_t KUI[10] = {11,10,9,9,11,10,11,4,1,1}, YUE[10] = {5,6,7,7,5,6,5,0,3,3};
  mn[ZM_TIANKUI] = KUI[ys]; mn[ZM_TIANYUE] = YUE[ys];
  static const int8_t LU[10] = {0,1,3,4,3,4,6,7,9,10};
  mn[ZM_LUCUN] = LU[ys]; mn[ZM_QINGYANG] = (int8_t)fix12(LU[ys] + 1); mn[ZM_TUOLUO] = (int8_t)fix12(LU[ys] - 1);
  /* 年支三合：申子辰 0，巳酉丑 1，寅午戌 2，亥卯未 3 */
  static const int8_t TRIO[12] = {0,1,2,3,0,1,2,3,0,1,2,3};
  static const int8_t MA[4] = {0, 9, 6, 3}, HUO[4] = {0, 1, 11, 7}, LING[4] = {8, 8, 1, 8};
  int g = TRIO[yb];
  mn[ZM_TIANMA] = MA[g];
  mn[ZM_DIKONG] = (int8_t)fix12(9 - t); mn[ZM_DIJIE] = (int8_t)fix12(9 + t);
  mn[ZM_HUOXING] = (int8_t)fix12(HUO[g] + t); mn[ZM_LINGXING] = (int8_t)fix12(LING[g] + t);
  for (int s = 0; s < ZM_MINOR_COUNT; s++) z->minor_mut[s] = -1;
  for (int k = 0; k < 4; k++) {
    int s = MUT[ys][k];
    if (s >= 100) z->minor_mut[s - 100] = (int8_t)k; else z->major_mut[s] = (int8_t)k;
  }
  z->soul_star = (int8_t)SOUL_STAR[sb];
  z->body_star = (int8_t)BODY_STAR[yb];
  return true;
}
