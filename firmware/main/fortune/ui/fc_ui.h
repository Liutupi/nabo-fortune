/* 场景框架：时间、按键、场景切换、顶栏、NABO 精灵、粒子、音效钩子、绘图小工具。
 * 与原型 prototype/index.html 一一对应（VT、go、nabo、naboThen、burst、T、G、glitch、tri）。 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "../gfx/fc_fx.h"
#include "../gfx/fc_gfx.h"
#include "../gfx/fc_res.h"
#include "../fortune_core.h"
#include "../fortune_text.h"

/* 调色板（原型 C 对象） */
#define C_BG RGB(0x0D, 0x08, 0x20)
#define C_BG2 RGB(0x1A, 0x10, 0x35)
#define C_GRID RGB(0x23, 0x1A, 0x46)
#define C_MAG RGB(0xFF, 0x3D, 0xA6)
#define C_CYAN RGB(0x35, 0xF0, 0xDC)
#define C_GOLD RGB(0xFF, 0xC9, 0x4A)
#define C_RED RGB(0xE0, 0x40, 0x2C)
#define C_PAPER RGB(0xF2, 0xE4, 0xC0)
#define C_INK RGB(0x2B, 0x1A, 0x0E)
#define C_WHITE RGB(0xF4, 0xF0, 0xFF)
#define C_DIM RGB(0x7B, 0x6A, 0xA8)
#define C_PURPLE RGB(0x4B, 0x2A, 0x9A)
#define C_HINT RGB(0x5C, 0x4F, 0x8A)
#define C_BLACK 0

enum { K_UP = 0, K_DOWN = 1, K_OK = 2 };

typedef struct fc_scene {
  const char *id, *name;
  bool no_double;                       /* 双击 ↑ 不返回主菜单（生辰输入） */
  void (*enter)(void);
  void (*key)(int k, bool down);
  void (*update)(int dt);
  void (*draw)(void);
} fc_scene_t;

/* 字体（启动时从总包打开） */
extern fc_font_t F_KAI15, F_KAI13, F_KAI10, F_SANS11, F_SANS9, F_DISP24, F_DISP13, F_MONO10;

extern uint32_t VT;                     /* 虚拟毫秒 */
extern int g_shake;                     /* 震屏幅度 ×100 */
extern fc_rng_t g_rng;
static inline int R1000(void) { return fc_rng_int(&g_rng, 1000); }   /* 原型 R()*1000 */
static inline int RI(int n) { return fc_rng_int(&g_rng, n); }
/* 原型 (R()*2-1)*a，结果取整 */
static inline int RS(int a) { return (R1000() * 2 - 1000) * a / 1000; }

bool fc_ui_init(const uint8_t *blob, uint32_t size, uint32_t seed);
void fc_ui_go(const char *id);
void fc_ui_back(void);
const char *fc_ui_scene_id(void);
unsigned fc_ui_image_misses(void);
void fc_ui_key(int k, bool down);
void fc_ui_step(int dt);                 /* 推进时间并更新场景 */
void fc_ui_render(fc_flush_fn flush, void *ctx, bool force);
uint32_t fc_ui_scene_t0(void);
static inline int EL(void) { return (int)(VT - fc_ui_scene_t0()); }
bool fc_ui_held(int k);

/* 音效：设备端实现 fc_sfx_play；宿主机默认空实现 */
enum { SFX_TICK, SFX_ENTER, SFX_CLINK, SFX_RATTLE, SFX_THUMP, SFX_STAMP, SFX_LINE, SFX_REVEAL, SFX_PRINT, SFX_WHOOSH, SFX_LOCK };
void fc_sfx_play(int id);
void fc_sfx_welcome(void);

/* NABO 精灵：12 fps，(cx,cy) 为中心 */
int spr_ms(const char *id);
int spr_frames(const char *id);
void nabo(const char *id, int size, int cx, int cy, int t);
void nabo_frame(const char *id, int size, int cx, int cy, int frame);
void nabo_then(const char *id, int size, int cx, int cy, int t, const char *idle);
/* 道具（prop/…），失败返回 NULL */
const fc_img_t *prop(const char *name);

/* 粒子 */
void burst(int x, int y, int n, uint16_t col);   /* col=0：金/浅金随机 */
void particles_clear(void);

/* 文字 */
void T(const fc_font_t *f, const char *s, int x, int y, uint16_t c, int al);
void G(const fc_font_t *f, const char *s, int x, int y, uint16_t c, uint16_t glow);
void glitch(const fc_font_t *f, const char *s, int x, int y, uint16_t c, int k);
void TF(const fc_font_t *f, int x, int y, uint16_t c, int al, const char *fmt, ...);
/* 按字数折行（与原型 wrap 一致：行尾标点并入上一行）。返回行数，lines[i] 指向 s 内，lens[i] 为字节数 */
int wrap(const char *s, int n, const char **lines, int *lens, int max);
int wrap_px(const fc_font_t *f, const char *s, int width, const char **lines, int *lens, int max);
void fc_body_add(char *body, int capacity, const char *fmt, ...);
int fc_read_card(const char *title, const char *subtitle, const char *body, int page);
int utf8_len(const char *s);
const char *utf8_at(const char *s, int i);        /* 第 i 个字的位置 */
void cn_num(int n, char *out);                     /* 1..100 → 中文数字 */

/* 三爻：bits[0] 为下爻；(x,y) 中心 */
void tri3(int x, int y, const uint8_t bits[3], int w, uint16_t col, int gap, int th);
/* 旋转后的三爻（ang：1/1024 圈，绕 (cx,cy)，三爻中心在距中心 r 处） */
void tri3_rot(int cx, int cy, int r, int ang, const uint8_t bits[3], int w, uint16_t col, int gap, int th);

/* 生辰档案：设备上存 NVS（fc_profile_save 由平台实现），宿主机只在内存 */
typedef struct { int16_t y; int8_t m, d, h, g; bool demo; } fc_profile_t;   /* h：时辰 0..11；g：0 男 1 女 */
extern fc_profile_t PF;
void fc_profile_save(void);
extern const char *g_birth_ret;           /* 生辰页保存后回到哪个场景 */
/* 当前时间（东八区墙上时间）：设备读 RTC，宿主机可设 */
typedef struct { int16_t y; int8_t m, d, hh, mm, wday; } fc_time_t;
void fc_now(fc_time_t *t);

/* 五角星（原型 star5）、旋转矩形（中心 cx,cy，ang 1/1024 圈） */
void star5(int x, int y, int r10, uint16_t col);   /* r10：半径 ×10 */
void rect_rot(int cx, int cy, int w, int h, int ang, uint16_t col);
void frame_rot(int cx, int cy, int w, int h, int ang, int th, uint16_t col);

/* 生辰卡（scene_birth.c）：原型 drawPF / drawConfirm */
void pf_lunar(const fc_profile_t *p, fc_lunar_t *l);
void draw_pf(const fc_profile_t *p, int y0, bool demo);
void draw_confirm(const char *title);
int gz_str(int gz, char *out);            /* 六十甲子 → "甲子"，返回字节数 */

/* 时钟：断电后时间未知（设备上由 main.c 判定），开机提示去校准；fc_set_now 由平台实现 */
extern bool g_time_unknown;
void fc_set_now(const fc_time_t *t);
int fc_weekday(int y, int m, int d);      /* 0 = 周日 */
