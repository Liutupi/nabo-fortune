/* Daily SSQ v1; field order matches prototype/fortune-core.js.
 * Numeric fields only: never hash C padding, pointers or translated text. */
#include "fortune_core.h"
#include <string.h>

static bool valid_date(int y, int m, int d) {
  if (y < FC_MIN_YEAR || y > FC_MAX_YEAR || m < 1 || m > 12 || d < 1 || d > 31) return false;
  int yy, mm, dd;
  fc_date_from_days(fc_days_from_1900(y, m, d), &yy, &mm, &dd);
  return yy == y && mm == m && dd == d;
}
static void mix(uint32_t *s, uint32_t v) { *s = (*s ^ v) * 16777619u; }
static void mix_lunar(uint32_t *s, const fc_lunar_t *l) {
  mix(s, l->year); mix(s, l->month); mix(s, l->day); mix(s, l->leap);
}
static void mix_bazi(uint32_t *s, const fc_bazi_t *b) {
  mix(s, b->y); mix(s, b->m); mix(s, b->d); mix(s, b->h);
}
/* Rejection removes modulo bias. This stream is private to SSQ. */
static unsigned bounded(fc_rng_t *r, unsigned n) {
  uint32_t u, threshold = (0u - (uint32_t)n) % n;
  do { u = fc_rng_u32(r); } while (u < threshold);
  return u % n;
}
bool fc_daily_ssq(int y, int m, int d, int py, int pm, int pd, int ph, int pg, fc_ssq_t *o) {
  if (!o || !valid_date(y, m, d) || !valid_date(py, pm, pd) || ph < 0 || ph > 11 || pg < 0 || pg > 1) return false;
  fc_lunar_t pl, l;
  fc_bazi_t pb, b;
  fc_bone_t bone;
  fc_ziwei_t z;
  fc_qimen_t q;
  fc_qimen_read_t qr;
  if (!fc_solar_to_lunar(py, pm, pd, &pl) || !fc_solar_to_lunar(y, m, d, &l) || !fc_ziwei(py, pm, pd, ph, &z)) return false;
  fc_bazi(py, pm, pd, ph == 0 ? 0 : ph * 2, 30, &pb);
  fc_bazi(y, m, d, 12, 0, &b);
  fc_bone(&pl, ph, &bone);
  fc_qimen(y, m, d, 12, 0, &q); fc_qimen_read(&q, 0, &qr);
  memset(o, 0, sizeof *o);
  uint32_t base = fc_daily_seed(y, m, d, py, pm, pd, ph, pg);
  fc_daily_ticket(base, fc_zodiac_sign(pm, pd), &o->ticket);
  fc_rng_t luck = {base ^ 0x4C55434Bu};
  o->lucky = (uint8_t)(1 + fc_rng_int(&luck, 9));
  uint32_t s = 2166136261u;
  mix(&s, 0x53535101u); mix(&s, base);
  mix_lunar(&s, &pl); mix_bazi(&s, &pb); mix_lunar(&s, &l); mix_bazi(&s, &b);
  const fc_ticket_t *t = &o->ticket;
  mix(&s, t->sign); mix(&s, t->sign_stars); mix(&s, o->lucky);
  mix(&s, t->tarot); mix(&s, t->tarot_rev); mix(&s, t->lot_no); mix(&s, t->lot_level);
  mix(&s, t->hex_up); mix(&s, t->hex_lo);
  for (int i = 0; i < 3; i++) mix(&s, t->yi[i]);
  for (int i = 0; i < 2; i++) mix(&s, t->ji[i]);
  mix(&s, bone.w_year); mix(&s, bone.w_month); mix(&s, bone.w_day); mix(&s, bone.w_hour); mix(&s, bone.total);
  mix(&s, z.soul); mix(&s, z.body); mix(&s, z.five); mix(&s, z.soul_star); mix(&s, z.body_star);
  for (int i = 0; i < 12; i++) { mix(&s, z.palace_stem[i]); mix(&s, z.palace_name[i]); }
  for (int i = 0; i < 14; i++) {
    mix(&s, z.major_at[i]); mix(&s, z.major_bright[i]); mix(&s, z.major_mut[i]); mix(&s, z.minor_at[i]); mix(&s, z.minor_mut[i]);
  }
  mix(&s, q.yang); mix(&s, q.yuan); mix(&s, q.ju); mix(&s, q.day_gz); mix(&s, q.hour_gz); mix(&s, q.zhifu_at); mix(&s, q.zhishi_at);
  for (int p = 1; p <= 9; p++) {
    mix(&s, q.earth[p]);
    if (p != 5) { mix(&s, q.heaven[p]); mix(&s, q.star[p]); mix(&s, q.door[p]); mix(&s, q.god[p]); }
  }
  o->seed = s;
  fc_rng_t r = {s};
  uint8_t pool[33];
  for (int i = 0; i < 33; i++) pool[i] = (uint8_t)(i + 1);
  for (int i = 0; i < 6; i++) {
    unsigned at = bounded(&r, (unsigned)(33 - i));
    o->red[i] = pool[at];
    for (unsigned j = at; j < (unsigned)(32 - i); j++) pool[j] = pool[j + 1];
  }
  for (int i = 1; i < 6; i++) for (int j = i; j > 0 && o->red[j] < o->red[j - 1]; j--) {
    uint8_t tmp = o->red[j]; o->red[j] = o->red[j - 1]; o->red[j - 1] = tmp;
  }
  o->blue = (uint8_t)(1 + bounded(&r, 16));
  o->lunar = l; o->day_gz = (int8_t)fc_day_gz(y, m, d); o->bone_total = bone.total;
  o->soul = z.soul; o->body = z.body; o->five = z.five;
  o->q_yang = q.yang; o->q_yuan = q.yuan; o->q_ju = q.ju; o->q_palace = qr.palace;
  return true;
}
