/* 直绘：每帧把绘制操作记进显示列表，按 8 行条带合成 RGB565 后送屏。
 * 每个条带对落在其中的操作做哈希，和上一帧相同就跳过（脏条带）。全部整数运算。 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "fc_res.h"

#define FC_W 240
#define FC_H 320
#define FC_STRIP 8

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

typedef void (*fc_flush_fn)(int y0, int rows, const uint16_t *pix, void *ctx);

enum { AL_L = 0, AL_C = 1, AL_R = 2 };

void fc_gfx_begin(void);
void fc_gfx_offset(int dx, int dy);              /* 震屏：之后的操作整体平移 */
void fc_gfx_clip(int x, int y, int w, int h);    /* 之后的操作裁剪到此矩形；w<=0 取消 */
void fc_gfx_scanlines(bool on);

void fc_rect(int x, int y, int w, int h, uint16_t c, uint8_t alpha);
void fc_frame(int x, int y, int w, int h, int th, uint16_t c);
void fc_line(int x0, int y0, int x1, int y1, int w, uint16_t c);
void fc_disc(int cx, int cy, int r, uint16_t c, uint8_t alpha);
void fc_ellipse(int cx, int cy, int rx, int ry, uint16_t c, uint8_t alpha);
void fc_ring(int cx, int cy, int r, int th, uint16_t c);
void fc_tri(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c);
/* 多边形：xy 为 n 对顶点，单位 1/16 像素 */
void fc_poly(const int16_t *xy, int n, uint16_t c, uint8_t alpha);
void fc_taiji(int cx, int cy, int r, int ang, uint16_t a, uint16_t b);
/* 图像：(x,y) 左上角；sx,sy,sw,sh 源矩形（sw<=0 取整幅）；flip: 1 水平 2 垂直 */
void fc_img(const fc_img_t *im, int frame, int x, int y, uint8_t alpha);
void fc_img_part(const fc_img_t *im, int frame, int sx, int sy, int sw, int sh, int x, int y);
/* 仿射：源锚点 (ax,ay) 放到屏幕 (cx,cy)，绕它旋转 ang（1/1024 圈），缩放 kx,ky（Q8，256 = 1 倍） */
void fc_img_aff(const fc_img_t *im, int frame, int cx, int cy, int ax, int ay, int ang, int kx, int ky, uint8_t alpha);
/* 文字：y 为行中线（与原型 textBaseline=middle 一致） */
void fc_text(const fc_font_t *f, int x, int y, const char *s, uint16_t c, int align);
void fc_text_n(const fc_font_t *f, int x, int y, const char *s, int nbytes, uint16_t c, int align);

/* 合成并送屏；force=true 时整屏重画。返回本帧重画的条带数。 */
int fc_gfx_end(fc_flush_fn flush, void *ctx, bool force);
int fc_gfx_ops(void);
bool fc_gfx_overflow(void);
