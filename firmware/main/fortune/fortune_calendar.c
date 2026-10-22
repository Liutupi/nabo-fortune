/* 历法：公历↔天数、农历、干支、节气、四柱、星座。整数实现。 */
#include "fortune_core.h"

static int32_t jdn(int y, int m, int d) {
  int a = (14 - m) / 12, yy = y + 4800 - a, mm = m + 12 * a - 3;
  return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
}
#define JDN_1900 2415021

int32_t fc_days_from_1900(int y, int m, int d) { return jdn(y, m, d) - JDN_1900; }

void fc_date_from_days(int32_t days, int *y, int *m, int *d) {
  int32_t a = days + JDN_1900 + 32044, b = (4 * a + 3) / 146097, c = a - 146097 * b / 4;
  int32_t dd = (4 * c + 3) / 1461, e = c - 1461 * dd / 4, mm = (5 * e + 2) / 153;
  *d = e - (153 * mm + 2) / 5 + 1;
  *m = mm + 3 - 12 * (mm / 10);
  *y = 100 * b + dd - 4800 + mm / 10;
}

static int mod(int a, int n) { int r = a % n; return r < 0 ? r + n : r; }

bool fc_solar_to_lunar(int y, int m, int d, fc_lunar_t *out) {
  if (y < FC_MIN_YEAR || y > FC_MAX_YEAR) return false;
  int32_t day = fc_days_from_1900(y, m, d);
  int i = y - FC_LUNAR_YEAR0;                       /* 农历年 = 公历年 或 公历年-1 */
  if (day < FC_LUNAR_YEARS[i].ny_day) i--;
  const fc_lunar_year_t *ly = &FC_LUNAR_YEARS[i];
  int32_t off = day - ly->ny_day;
  int count = ly->leap ? 13 : 12;
  for (int k = 0; k < count; k++) {
    int len = (ly->big >> k) & 1 ? 30 : 29;
    if (off < len) {
      int month = k + 1; bool leap = false;
      if (ly->leap) {
        if (k == ly->leap) { month = ly->leap; leap = true; }
        else if (k > ly->leap) month = k;
      }
      out->year = (int16_t)(FC_LUNAR_YEAR0 + i);
      out->month = (int8_t)month; out->leap = leap;
      out->day = (int8_t)(off + 1); out->month_days = (int8_t)len;
      return true;
    }
    off -= len;
  }
  return false;
}

int fc_lunar_year_gz(int lunar_year) { return mod(lunar_year - 4, 60); }
int fc_day_gz(int y, int m, int d) { return mod(fc_days_from_1900(y, m, d) + 10, 60); }
int fc_hour_branch(int hour) { return ((hour + 1) / 2) % 12; }

int fc_hour_gz(int y, int m, int d, int hour) {
  int dg = fc_day_gz(y, m, d);
  if (hour == 23) dg = (dg + 1) % 60;
  int br = fc_hour_branch(hour), stem = ((dg % 10) % 5 * 2 + br) % 10;
  /* 干支序号：满足 i%10==stem 且 i%12==br */
  for (int i = 0; i < 60; i++) if (i % 10 == stem && i % 12 == br) return i;
  return 0;
}

static int32_t minute_of(int y, int m, int d, int hh, int mm) {
  return fc_days_from_1900(y, m, d) * 1440 + hh * 60 + mm;
}

/* 返回最近一个 <= t 的节气：*year_out 为所在公历年，返回 0..23 */
static int prev_term(int y, int32_t t, int *year_out) {
  for (int yy = y; yy >= y - 1; yy--) {
    const int32_t *row = FC_JIEQI_MIN[yy - FC_LUNAR_YEAR0];
    for (int k = 23; k >= 0; k--) if (row[k] <= t) { *year_out = yy; return k; }
  }
  *year_out = y - 1; return 23;
}

int fc_prev_jieqi(int y, int m, int d, int hh, int mm) {
  int yy; return prev_term(y, minute_of(y, m, d, hh, mm), &yy);
}

void fc_bazi(int y, int m, int d, int hh, int mm, fc_bazi_t *out) {
  int32_t t = minute_of(y, m, d, hh, mm);
  int by = (t >= FC_JIEQI_MIN[y - FC_LUNAR_YEAR0][2]) ? y : y - 1;   /* 立春 */
  int ygz = mod(by - 4, 60);
  /* 月：最近的“节”（偶数序号） */
  int ty, k = prev_term(y, t, &ty);
  if (k % 2) k--;
  int br = (k / 2 + 1) % 12;
  int yin_stem = ((ygz % 10) % 5 * 2 + 2) % 10;
  int ms = (yin_stem + mod(br - 2, 12)) % 10, mgz = 0;
  for (int i = 0; i < 60; i++) if (i % 10 == ms && i % 12 == br) { mgz = i; break; }
  out->y = (int8_t)ygz; out->m = (int8_t)mgz;
  out->d = (int8_t)fc_day_gz(y, m, d);
  out->h = (int8_t)fc_hour_gz(y, m, d, hh);
}

int fc_zodiac_sign(int m, int d) {
  static const int16_t st[12][2] = {{120,10},{219,11},{321,0},{420,1},{521,2},{622,3},{723,4},{823,5},{923,6},{1024,7},{1123,8},{1222,9}};
  int v = m * 100 + d, r = 9;
  for (int i = 0; i < 12; i++) if (v >= st[i][0]) r = st[i][1];
  return r;
}
