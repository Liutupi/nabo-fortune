/* NABO 赛博算命机 · 纯逻辑核心
 * 不依赖屏幕、声音、RTOS；全部整数运算；所有输入时间都是东八区墙上时间。
 * 标准答案：prototype/fortune-core.js + lunar-javascript / iztro / kinqimen，
 * 由 tests/fortune_host/record_golden.js 录制，tests/test_fortune.py 逐项对拍。 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/* ---------- 表（fortune_tables.c，自动生成） ---------- */
typedef struct { int32_t ny_day; uint16_t big; uint8_t leap; } fc_lunar_year_t;
extern const int32_t FC_LUNAR_YEAR0, FC_LUNAR_YEAR1;
extern const fc_lunar_year_t FC_LUNAR_YEARS[];
extern const int32_t FC_JIEQI_MIN[][24];

/* 支持范围：公历 1901-01-01 至 2100-12-31 */
#define FC_MIN_YEAR 1901
#define FC_MAX_YEAR 2100

/* ---------- 历法 ---------- */
typedef struct {
  int16_t year;      /* 农历年（按正月初一分界） */
  int8_t month;      /* 1..12 */
  bool leap;         /* 是否闰月 */
  int8_t day;        /* 1..30 */
  int8_t month_days; /* 当月天数 29/30 */
} fc_lunar_t;

typedef struct { int8_t y, m, d, h; } fc_bazi_t; /* 四柱，各为 0..59 六十甲子序号 */

int32_t fc_days_from_1900(int y, int m, int d);           /* 1900-01-01 = 0 */
void fc_date_from_days(int32_t days, int *y, int *m, int *d);
bool fc_solar_to_lunar(int y, int m, int d, fc_lunar_t *out);
int fc_lunar_year_gz(int lunar_year);                      /* 0..59 */
int fc_day_gz(int y, int m, int d);                        /* 0..59 */
int fc_hour_branch(int hour);                              /* 0..11，23 点为子 */
int fc_hour_gz(int y, int m, int d, int hour);             /* 23 点用次日日干起子时 */
int fc_prev_jieqi(int y, int m, int d, int hh, int mm);    /* 0..23（小寒=0），按精确到分钟 */
void fc_bazi(int y, int m, int d, int hh, int mm, fc_bazi_t *out); /* 年按立春、月按节 */
int fc_zodiac_sign(int m, int d);                          /* 0=白羊 … 11=双鱼 */

/* ---------- 称骨 ---------- */
typedef struct { uint8_t w_year, w_month, w_day, w_hour; uint8_t total; } fc_bone_t; /* 单位：钱 */
void fc_bone(const fc_lunar_t *lunar, int hour_branch, fc_bone_t *out);
int fc_bone_band(int total_qian);                          /* 0..8 白话分档 */

/* ---------- 六爻 ---------- */
/* vals[0]=初爻 … vals[5]=上爻，取值 6/7/8/9；返回本卦、变卦在 HEX[上][下] 表中的编号 up*8+lo */
void fc_liuyao(const uint8_t vals[6], int *ben, int *bian, bool *moving);
int fc_coin_line(const uint8_t backs[3]);
/* 断法（朱熹《易学启蒙》）：kind 0 卦辞 1 爻辞 2 用九/用六；pos 0=初爻；it[0] 为主 */
typedef struct { int8_t kind, hex, pos; } fc_yao_item_t;
typedef struct { int8_t ben, bian, moving, n, main; fc_yao_item_t it[2]; } fc_liuyao_read_t;
void fc_liuyao_read(const uint8_t vals[6], fc_liuyao_read_t *out);                  /* 三枚：1=背(3) 0=字(2) → 6..9 */

/* ---------- 随机 ---------- */
typedef struct { uint32_t a; } fc_rng_t;
uint32_t fc_rng_u32(fc_rng_t *r);                          /* mulberry32，与原型逐位一致 */
int fc_rng_int(fc_rng_t *r, int n);                        /* floor(next()*n) 的整数等价 */
uint32_t fc_daily_seed(int y, int m, int d, int py, int pm, int pd, int ph, int pg);

typedef struct {
  int8_t sign, sign_stars, tarot, tarot_rev, lot_level, hex_up, hex_lo;
  int16_t lot_no;
  int8_t yi[3], ji[2];
  int16_t serial;
} fc_ticket_t;
void fc_daily_ticket(uint32_t seed, int sign, fc_ticket_t *out);
void fc_tarot_hand(fc_rng_t *r, int8_t hand[7], bool *reversed);
#define FC_LOT_COUNT 100
int fc_lot_level(int lot_no);                              /* 观音灵签：0 上签 1 中签 2 下签；签号 1..100 */

/* ---------- 奇门遁甲（时家转盘拆补法） ---------- */
typedef struct {
  int8_t jieqi;          /* 0..23 */
  bool yang;             /* 阳遁 */
  int8_t yuan;           /* 0 上 1 中 2 下 */
  int8_t ju;             /* 1..9 */
  int8_t day_gz, hour_gz;
  int8_t earth[10];      /* 地盘干：宫 1..9 → 天干 0..9 */
  int8_t heaven[10];     /* 天盘干（坤二宫另带中五干：heaven2_extra） */
  int8_t heaven2_extra;  /* -1 表示无 */
  int8_t star[10];       /* 宫 → 原宫号（九星以原宫号表示：1蓬 2芮 3冲 4辅 5禽 6心 7柱 8任 9英） */
  int8_t door[10];       /* 宫 → 原宫号（1休 2死 3伤 4杜 6开 7惊 8生 9景），5 宫为 0 */
  int8_t god[10];        /* 宫 → 八神 0..7（值符…九天），5 宫为 -1 */
  int8_t zhifu_src, zhifu_at, zhishi_at;
  int8_t ring_shift, door_shift;
} fc_qimen_t;
void fc_qimen(int y, int m, int d, int hh, int mm, fc_qimen_t *out); /* 23 点按次日 */
/* 问事：0求财(生门) 1事业(开门) 2感情(六合) 3考试(景门) 4出行(值使) */
typedef struct { int8_t palace; int8_t score; int8_t verdict; } fc_qimen_read_t; /* verdict 0大顺 1顺 2平 3宜缓 */
void fc_qimen_read(const fc_qimen_t *q, int ask, fc_qimen_read_t *out);

/* ---------- 紫微斗数（与 iztro 2.6.1 默认配置一致：fixLeap=true，年按正月初一） ---------- */
enum { ZW_ZIWEI, ZW_TIANJI, ZW_TAIYANG, ZW_WUQU, ZW_TIANTONG, ZW_LIANZHEN, ZW_TIANFU, ZW_TAIYIN,
       ZW_TANLANG, ZW_JUMEN, ZW_TIANXIANG, ZW_TIANLIANG, ZW_QISHA, ZW_POJUN, ZW_MAJOR_COUNT };
enum { ZM_ZUOFU, ZM_YOUBI, ZM_WENCHANG, ZM_WENQU, ZM_TIANKUI, ZM_TIANYUE, ZM_LUCUN, ZM_TIANMA,
       ZM_DIKONG, ZM_DIJIE, ZM_HUOXING, ZM_LINGXING, ZM_QINGYANG, ZM_TUOLUO, ZM_MINOR_COUNT };
typedef struct {
  int8_t soul, body;            /* 宫索引（0=寅） */
  int8_t five;                  /* 五行局数 2..6 */
  int8_t year_stem, year_branch;
  int8_t palace_stem[12];       /* 0..9 */
  int8_t palace_name[12];       /* 0=命宫,1父母,2福德,3田宅,4官禄,5仆役,6迁移,7疾厄,8财帛,9子女,10夫妻,11兄弟 */
  int8_t major_at[ZW_MAJOR_COUNT];
  int8_t minor_at[ZM_MINOR_COUNT];
  int8_t major_bright[ZW_MAJOR_COUNT]; /* 0 无 1庙 2旺 3得 4利 5平 6不 7陷 */
  int8_t major_mut[ZW_MAJOR_COUNT];    /* -1 无，0禄 1权 2科 3忌 */
  int8_t minor_mut[ZM_MINOR_COUNT];
  int8_t soul_star, body_star;  /* 命主、身主：主星或辅星编号见 fortune_text */
} fc_ziwei_t;
bool fc_ziwei(int y, int m, int d, int time_index, fc_ziwei_t *out); /* time_index 0..11 */

#ifdef __cplusplus
}
#endif
