/* 界面与解读文案。fortune_text.c 由 tools/gen_fortune_text.js 从原型生成；fortune_names.c 为固定名称表。 */
#pragma once
#include <stdint.h>

extern const char *const FT_BONE_POEM[52];          /* 2.1..7.2 两，下标 = 钱数 - 21 */
extern const uint8_t FT_BONE_BAND_LO[9], FT_BONE_BAND_HI[9];
extern const char *const FT_BONE_TYPE[9], *const FT_BONE_TEXT[9];
extern const char *const FT_TAROT_NUM[22], *const FT_TAROT_NAME[22], *const FT_TAROT_KEY[22];
extern const char *const FT_SIGN_NAME[12], *const FT_SIGN_DATE[12];
extern const char *const FT_HEX_NAME[64];           /* 上卦*8+下卦 */
extern const char *const FT_YI[6], *const FT_JI[5];
extern const char *const FT_ZW_TEXT[14 * 4];        /* 主星 × 主题（性格/财/事业/感情） */
extern const int8_t FT_ZW_FIELD_IDX[12];
extern const char *const FT_ZW_FIELD_NAME[12], *const FT_ZW_BRIGHT[8], *const FT_ZW_MUT[4];
extern const char *const FT_QM_DOOR_TEXT[10], *const FT_QM_STAR_TEXT[10], *const FT_QM_GOD_TEXT[8];
extern const char *const FT_QM_ASK[5], *const FT_QM_KEY[5], *const FT_QM_DIR[10];

/* fortune_names.c */
extern const char *const FT_GAN[10], *const FT_ZHI[12], *const FT_SHENGXIAO[12], *const FT_JIEQI[24];
extern const char *const FT_LMONTH[12], *const FT_LDAY[30], *const FT_CN[10];
extern const char *const FT_ZW_MAJOR[14], *const FT_ZW_MINOR[14], *const FT_ZW_PALACE[12];
extern const char *const FT_ZW_BRIGHT_NAME[8], *const FT_ZW_MUT_NAME[4], *const FT_ZW_FIVE[7];
extern const char *const FT_QM_STAR[10], *const FT_QM_DOOR[10], *const FT_QM_GODS[8], *const FT_QM_GUA[10], *const FT_QM_VERDICT[4];
