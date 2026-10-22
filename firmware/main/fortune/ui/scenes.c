/* 场景表（顺序与原型 ORDER 一致） */
#include "fc_ui.h"
#include <stddef.h>

extern const fc_scene_t SC_BOOT, SC_MENU, SC_BIRTH, SC_DAILY, SC_SHAKE, SC_LOT, SC_TAROT, SC_BONE, SC_STAR, SC_ZIWEI, SC_QIMEN, SC_CLOCK;
const fc_scene_t *const FC_SCENES[] = {&SC_BOOT, &SC_MENU, &SC_BIRTH, &SC_DAILY, &SC_SHAKE, &SC_LOT, &SC_TAROT,
                                       &SC_BONE, &SC_STAR, &SC_ZIWEI, &SC_QIMEN, &SC_CLOCK, NULL};
