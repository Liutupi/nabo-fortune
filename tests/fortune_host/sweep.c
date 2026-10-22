/* 全量扫描：1901-01-01 … 2100-12-31 每天每个时辰，跑一遍所有入口（配合 -fsanitize 使用） */
#include <stdio.h>
#include "../../firmware/main/fortune/fortune_core.h"
int main(void) {
  long n = 0; unsigned acc = 0;
  for (int32_t day = fc_days_from_1900(1901,1,1); day <= fc_days_from_1900(2100,12,31); day++) {
    int y, m, d; fc_date_from_days(day, &y, &m, &d);
    if (fc_days_from_1900(y, m, d) != day) { printf("date roundtrip %d\n", day); return 1; }
    fc_lunar_t l; if (!fc_solar_to_lunar(y, m, d, &l)) { printf("lunar fail %d-%d-%d\n", y, m, d); return 1; }
    for (int h = 0; h < 24; h++) {
      fc_bazi_t b; fc_bazi(y, m, d, h, 30, &b); acc += b.m;
      if (y < 2100 || m < 12 || d < 31 || h < 23) { fc_qimen_t q; fc_qimen(y, m, d, h, 30, &q); for (int a = 0; a < 5; a++) { fc_qimen_read_t r; fc_qimen_read(&q, a, &r); acc += r.palace; } }
      if (h % 2 == 0) { int t = fc_hour_branch(h); fc_bone_t o; fc_bone(&l, t, &o); acc += o.total; fc_ziwei_t z; if (!fc_ziwei(y, m, d, t, &z)) { printf("ziwei fail\n"); return 1; } acc += z.soul; }
      n++;
    }
    acc += fc_zodiac_sign(m, d);
  }
  printf("ok %ld 个时间点 校验和 %u\n", n, acc);
  return 0;
}
