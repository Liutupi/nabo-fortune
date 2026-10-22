// main/fc_sound.c — 音效合成（16 kHz 单声道），音色照搬原型 sfx：方波/三角/正弦 + 带滤波的噪声，指数衰减。
// fc_sfx_play 可在任意任务调用（只置位），fc_sound_fill 在音频任务里每 20 ms 调一次。
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

#include "fortune/gfx/fc_fx.h"
#include "fortune/ui/fc_ui.h"

#define RATE 16000
#define VOICES 6

enum { W_SQ, W_TRI, W_SIN, W_NOISE_BP, W_NOISE_LP };
typedef struct {
    uint16_t hz, hz2;      /* hz2 ≠ 0：滑音到 hz2 */
    uint16_t ms, delay_ms;
    uint8_t wave;
    uint16_t vol;          /* 千分比，对应原型 gain */
} note_t;

static const note_t N_TICK[] = {{1100, 0, 45, 0, W_TRI, 150}};
static const note_t N_ENTER[] = {{523, 0, 90, 0, W_TRI, 200}, {784, 0, 120, 70, W_TRI, 200}};
static const note_t N_CLINK[] = {{1600, 0, 140, 0, W_SIN, 180}, {2400, 0, 80, 0, W_SIN, 80}};
static const note_t N_RATTLE[] = {{1800, 0, 45, 0, W_SIN, 160}};
static const note_t N_THUMP[] = {{110, 45, 180, 0, W_SIN, 250}};
static const note_t N_STAMP[] = {{90, 40, 250, 0, W_SIN, 220}, {600, 0, 90, 0, W_SIN, 100}};
static const note_t N_LINE[] = {{660, 0, 80, 0, W_TRI, 180}, {990, 0, 120, 60, W_TRI, 180}};
static const note_t N_REVEAL[] = {{523, 0, 250, 0, W_TRI, 180}, {659, 0, 250, 90, W_TRI, 180}, {784, 0, 250, 180, W_TRI, 180}, {1047, 0, 250, 270, W_TRI, 180}};
static const note_t N_PRINT[] = {{1100, 0, 20, 0, W_TRI, 100}};
static const note_t N_WHOOSH[] = {{1200, 300, 180, 0, W_SIN, 180}};
static const note_t N_LOCK[] = {{1200, 0, 45, 0, W_TRI, 170}, {1800, 0, 50, 0, W_SIN, 100}};

static const struct { const note_t *n; int count; } CUES[] = {
    [SFX_TICK] = {N_TICK, 1},     [SFX_ENTER] = {N_ENTER, 2},   [SFX_CLINK] = {N_CLINK, 2},   [SFX_RATTLE] = {N_RATTLE, 1},
    [SFX_THUMP] = {N_THUMP, 1},   [SFX_STAMP] = {N_STAMP, 2},   [SFX_LINE] = {N_LINE, 2},     [SFX_REVEAL] = {N_REVEAL, 4},
    [SFX_PRINT] = {N_PRINT, 1},   [SFX_WHOOSH] = {N_WHOOSH, 1}, [SFX_LOCK] = {N_LOCK, 2},
};
#define NCUE ((int)(sizeof CUES / sizeof CUES[0]))

typedef struct {
    uint32_t phase, step, step2;
    int32_t left, total, wait, amp;
    int32_t lp, bp_lo;     /* 噪声滤波状态 */
    int32_t k;             /* 滤波系数 Q15 */
    uint8_t wave;
} voice_t;

static atomic_uint s_pending;
static atomic_bool s_welcome;
static const uint8_t *s_voice;
static uint32_t s_voice_bytes, s_voice_pos;
static int s_voice_wait;
static voice_t s_v[VOICES];
static uint32_t s_lfsr = 0xACE1u;
static int s_master = 1000;         /* Cues have bounded gain plus a soft attack. */

void fc_sfx_play(int id) { if (id >= 0 && id < NCUE) atomic_fetch_or(&s_pending, 1u << id); }
void fc_sfx_welcome(void) { fc_sfx_play(SFX_ENTER); atomic_store(&s_welcome, true); }
void fc_sound_master(int permille) { s_master = permille; }

static void start(const note_t *n) {
    int k = 0;
    for (int i = 1; i < VOICES; i++)
        if (s_v[i].left + s_v[i].wait < s_v[k].left + s_v[k].wait) k = i;
    voice_t *v = &s_v[k];
    memset(v, 0, sizeof *v);
    v->step = (uint32_t)(((uint64_t)n->hz << 32) / RATE);
    v->step2 = n->hz2 ? (uint32_t)(((uint64_t)n->hz2 << 32) / RATE) : v->step;
    v->total = v->left = n->ms * RATE / 1000;
    v->wait = n->delay_ms * RATE / 1000;
    v->wave = n->wave;
    v->amp = (int32_t)n->vol * 32767 / 1000 * s_master / 1000;
    /* 一阶滤波系数 ≈ 2π·f/fs（Q15），带通 = 低通(f·1.5) − 低通(f/1.5) */
    v->k = (int32_t)((int64_t)n->hz * 205887 / RATE);           /* 2π·32768 ≈ 205887 */
    if (v->k > 32000) v->k = 32000;
}

static inline int32_t noise(void) {
    s_lfsr ^= s_lfsr << 13; s_lfsr ^= s_lfsr >> 17; s_lfsr ^= s_lfsr << 5;
    return (int32_t)(s_lfsr & 0xFFFF) - 32768;
}

void fc_sound_fill(int16_t *pcm, int n) {
    if (atomic_exchange(&s_welcome, false)) {
        s_voice = fc_res("snd/boot-welcome", &s_voice_bytes);
        s_voice_pos = 0; s_voice_wait = RATE * 1400 / 1000;
    }
    unsigned p = atomic_exchange(&s_pending, 0);
    for (int c = 0; c < NCUE; c++)
        if (p & (1u << c))
            for (int i = 0; i < CUES[c].count; i++) start(&CUES[c].n[i]);
    for (int i = 0; i < n; i++) {
        int32_t acc = pcm[i];
        for (int j = 0; j < VOICES; j++) {
            voice_t *v = &s_v[j];
            if (v->wait > 0) { v->wait--; continue; }
            if (v->left <= 0) continue;
            int32_t ph = (int32_t)(v->phase >> 16), s;
            switch (v->wave) {
                case W_SQ: s = ph < 32768 ? 32767 : -32767; break;
                case W_TRI: s = ph < 32768 ? ph * 2 - 32767 : 32767 - (ph - 32768) * 2; break;
                case W_SIN: s = fx_sin(ph >> 6) * 2; break;
                case W_NOISE_LP: v->lp += (int32_t)((int64_t)(noise() - v->lp) * v->k >> 15); s = v->lp * 2; break;
                default: {      /* 带通 */
                    int32_t x = noise();
                    v->lp += (int32_t)((int64_t)(x - v->lp) * v->k >> 15);
                    v->bp_lo += (int32_t)((int64_t)(v->lp - v->bp_lo) * (v->k / 3) >> 15);
                    s = (v->lp - v->bp_lo) * 2;
                }
            }
            if (s > 32767) s = 32767;
            if (s < -32767) s = -32767;
            /* 指数近似：剩余比例的平方；噪声按原型是线性衰减 */
            int32_t e = (int32_t)((int64_t)v->left * 256 / v->total);
            int32_t env = v->wave >= W_NOISE_BP ? e : e * e >> 8;
            int elapsed = v->total - v->left;
            if (elapsed < 96) env = env * elapsed / 96;  /* 6ms attack prevents hard-start clicks. */
            acc += (int32_t)((int64_t)s * v->amp / 32767 * env >> 8);
            uint32_t st = v->step2 == v->step ? v->step : v->step2 + (uint32_t)((int64_t)((int64_t)v->step - v->step2) * v->left / v->total);
            v->phase += st;
            v->left--;
        }
        if (s_voice_wait > 0) s_voice_wait--;
        else if (s_voice && s_voice_pos + 1 < s_voice_bytes) {
            int16_t sample = (int16_t)(s_voice[s_voice_pos] | s_voice[s_voice_pos + 1] << 8);
            s_voice_pos += 2;
            acc = sample + acc / 4;
        }
        pcm[i] = (int16_t)(acc > 32767 ? 32767 : acc < -32768 ? -32768 : acc);
    }
}
