/* 宿主机驱动：把固件 UI 编成共享库，Python 用 ctypes 驱动按键/时间并取帧缓冲。
 * 每次 host_frame 先按脏条带增量合成到 fb，再强制整屏合成到 fb_full，两者应逐像素相同。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../firmware/main/fortune/ui/fc_ui.h"

static uint8_t *g_blob;
static uint16_t fb[FC_W * FC_H], fb_full[FC_W * FC_H];
static int g_strips;
static void to_fb(int y0, int rows, const uint16_t *pix, void *ctx) {
  memcpy((uint16_t *)ctx + y0 * FC_W, pix, (size_t)rows * FC_W * 2); if (ctx == fb) g_strips++;
}
int host_init(const char *path, unsigned seed) {
  FILE *f = fopen(path, "rb"); if (!f) return -1;
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  free(g_blob); g_blob = malloc((size_t)n);
  if (fread(g_blob, 1, (size_t)n, f) != (size_t)n) { fclose(f); return -2; }
  fclose(f);
  if (!fc_ui_init(g_blob, (uint32_t)n, seed)) return -3;
  fc_ui_go("boot");
  return 0;
}
void host_go(const char *id) { fc_ui_go(id); }
void host_key(int k, int down) { fc_ui_key(k, down != 0); }
void host_step(int dt) { fc_ui_step(dt); }
extern int fc_lot_force;
void host_set_lot(int no) { fc_lot_force = no; }
/* 返回：增量条带数；*mismatch 为增量与整屏不同的像素数 */
int host_frame(int force, int *mismatch, int *ops) {
  g_strips = 0;
  fc_ui_render(to_fb, fb, force != 0);
  fc_gfx_end(to_fb, fb_full, true);
  int bad = 0; for (int i = 0; i < FC_W * FC_H; i++) bad += fb[i] != fb_full[i];
  if (mismatch) *mismatch = bad;
  if (ops) *ops = fc_gfx_ops() | (fc_gfx_overflow() ? 1 << 30 : 0);
  return g_strips;
}
const uint16_t *host_fb(void) { return fb; }
unsigned host_cache_bytes(void) { return fc_img_cache_bytes(); }
const char *host_scene(void) { return fc_ui_scene_id(); }
