// main/main.c — 《NABO 赛博算命机》AI Passport 独立固件：启动、按键、送屏、声音、时钟、档案、休眠。
//
// 开机直接进开机动画，不配网、不联网。整片 Flash 只装这一个程序。
//   上键 / 下键  选择；上键双击返回主转盘
//   OK          确认；摇卦、摇签要按住再松开
// 5 分钟无操作先存档再深睡，任意键唤醒（重新启动，回到开机动画）。
// 工牌没有时钟电池：上电 / 掉电复位后时间未知，开机提示“校准时间”；深睡和软件重启时间保持。
// 串口（USB）调试命令，每行一条：status、perf、go <场景>、key <u|d|o> <1|0>、time YYYY-MM-DD HH:MM、pf Y M D 时辰 性别
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_random.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "fortune/ui/fc_ui.h"

void fc_sound_fill(int16_t *pcm, int n);

static const char *TAG = "fortune";

#define SAMPLE_RATE 16000
#define CHUNK 320                        // 20 ms
#define VOLUME 80
#define IDLE_SLEEP_US (5LL * 60 * 1000 * 1000)
#define BACKLIGHT 85
#define FRAME_MS 33
#define NVS_NS "fortune"

static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_dma[2];
static int s_which;
static SemaphoreHandle_t s_free;         // 空闲的 DMA 条带缓冲个数
static QueueHandle_t s_keys;
static _Atomic int64_t s_last_input_us;
static atomic_bool s_sleep_now;
static bool s_audio_ok;

typedef struct { uint8_t key, down; } key_ev_t;

/* ---------- 送屏：fc_gfx_end 每合成好一条 8 行就回调一次 ---------- */

static bool on_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *ed, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_free, &woken);
    return woken == pdTRUE;
}

static void flush(int y0, int rows, const uint16_t *pix, void *ctx)
{
    xSemaphoreTake(s_free, portMAX_DELAY);
    uint16_t *b = s_dma[s_which];
    s_which ^= 1;
    int n = FC_W * rows;
    for (int i = 0; i < n; i++) b[i] = (uint16_t)(pix[i] << 8 | pix[i] >> 8);   // 面板要大端
    if (esp_lcd_panel_draw_bitmap(s_panel, 0, y0, FC_W, y0 + rows, b) != ESP_OK) xSemaphoreGive(s_free);
}

static void flush_wait(void)
{
    xSemaphoreTake(s_free, portMAX_DELAY);
    xSemaphoreTake(s_free, portMAX_DELAY);
    xSemaphoreGive(s_free);
    xSemaphoreGive(s_free);
}

/* ---------- 时钟：系统时间直接当作东八区墙上时间 ---------- */

static int32_t s_d1970;                  // 1970-01-01 距 1900-01-01 的天数

void fc_now(fc_time_t *t)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t s = tv.tv_sec < 0 ? 0 : tv.tv_sec;
    int y, m, d;
    fc_date_from_days(s_d1970 + (int32_t)(s / 86400), &y, &m, &d);
    t->y = (int16_t)y; t->m = (int8_t)m; t->d = (int8_t)d;
    t->hh = (int8_t)(s % 86400 / 3600); t->mm = (int8_t)(s % 3600 / 60);
    t->wday = (int8_t)fc_weekday(y, m, d);
}

static void save_clock(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    nvs_set_i64(h, "clock", tv.tv_sec);
    nvs_commit(h);
    nvs_close(h);
}

void fc_set_now(const fc_time_t *t)
{
    int64_t days = fc_days_from_1900(t->y, t->m, t->d) - s_d1970;
    struct timeval tv = {.tv_sec = (time_t)(days * 86400 + t->hh * 3600 + t->mm * 60), .tv_usec = 0};
    settimeofday(&tv, NULL);
    save_clock();
    ESP_LOGI(TAG, "clock set %04d-%02d-%02d %02d:%02d", t->y, t->m, t->d, t->hh, t->mm);
}

/* ---------- 生辰档案（NVS） ---------- */

void fc_profile_save(void)
{
    PF.demo = false;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_blob(h, "pf", &PF, sizeof PF);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "profile saved %d-%d-%d h%d g%d", PF.y, PF.m, PF.d, PF.h, PF.g);
}

static void load_nvs(bool rtc_ok)
{
    int64_t last = 0;
    bool have = false;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        fc_profile_t p;
        size_t n = sizeof p;
        if (nvs_get_blob(h, "pf", &p, &n) == ESP_OK && n == sizeof p && p.y >= 1901 && p.y <= 2100 && p.m >= 1 && p.m <= 12 &&
            p.d >= 1 && p.d <= 31 && p.h >= 0 && p.h < 12 && p.g >= 0 && p.g <= 1) {
            PF = p; PF.demo = false;
        }
        have = nvs_get_i64(h, "clock", &last) == ESP_OK;
        nvs_close(h);
    }
    // 软件复位也可能来自首次安装前的另一套固件，不能只凭 reset reason 信任 RTC。
    struct timeval now;
    gettimeofday(&now, NULL);
    int64_t earliest = (int64_t)(fc_days_from_1900(2024, 1, 1) - s_d1970) * 86400;
    int64_t latest = (int64_t)(fc_days_from_1900(2100, 1, 1) - s_d1970) * 86400;
    rtc_ok = rtc_ok && now.tv_sec >= earliest && now.tv_sec < latest;
    have = have && last >= earliest && last < latest;
    g_time_unknown = !rtc_ok;
    if (!rtc_ok) {
        // 时间丢了：先恢复到上次记下的时间，开机提示去校准（校准页以它为初值）
        struct timeval tv = {.tv_sec = (time_t)(have ? last : (int64_t)(fc_days_from_1900(2026, 10, 22) - s_d1970) * 86400 + 12 * 3600)};
        settimeofday(&tv, NULL);
    }
}

/* ---------- 按键（esp_timer 任务里，只入队） ---------- */

static void on_button(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    if (ev != BSP_BTN_PRESS && ev != BSP_BTN_RELEASE) return;
    atomic_store(&s_last_input_us, esp_timer_get_time());
    key_ev_t e = {btn == BSP_BTN_UP ? K_UP : btn == BSP_BTN_DOWN ? K_DOWN : K_OK, ev == BSP_BTN_PRESS};
    xQueueSend(s_keys, &e, 0);
}

/* ---------- 深睡 ---------- */

static void warn(const char *what, esp_err_t err)
{
    if (err != ESP_OK) ESP_LOGW(TAG, "%s: %s", what, esp_err_to_name(err));
}

// 由音频任务调用（它独占编解码器）；游戏任务这时已经存档并停下
static void enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "idle, entering deep sleep");
    save_clock();
    bsp_display_backlight(0);
    int level = 0;
    if (bsp_button_prepare_deep_sleep(&level) != ESP_OK) esp_restart();
    warn("gpio wake", esp_deep_sleep_enable_gpio_wakeup(1ULL << BSP_BTN_GPIO, ESP_GPIO_WAKEUP_GPIO_LOW));
    warn("battery sleep", bsp_battery_sleep());
    if (s_audio_ok) {
        warn("audio sleep", bsp_audio_sleep());
        warn("i2s release", bsp_audio_prepare_deep_sleep());
    }
    warn("i2c release", bsp_i2c_prepare_deep_sleep());
    warn("display sleep", bsp_display_prepare_deep_sleep());
    esp_deep_sleep_start();
    esp_restart();
}

/* ---------- 音频 ---------- */

static void audio_task(void *arg)
{
    static int16_t pcm[CHUNK];
    if (s_audio_ok) bsp_audio_set_volume(VOLUME);
    for (;;) {
        if (atomic_load(&s_sleep_now)) enter_deep_sleep();
        memset(pcm, 0, sizeof(pcm));
        fc_sound_fill(pcm, CHUNK);
        if (!s_audio_ok || bsp_audio_write(pcm, sizeof(pcm)) != ESP_OK) vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ---------- 串口调试 ---------- */

static uint32_t s_frames, s_frame_slow, s_strips;
static uint64_t s_frame_us, s_frame_max;
static int s_force;                      // 下一帧整屏重画
static bool s_restart;

static bool valid_date(int y, int m, int d)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (y < 1901 || y > 2100 || m < 1 || m > 12 || d < 1) return false;
    int n = days[m - 1] + (m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    return d <= n;
}

static void command(char *cmd, char *out, size_t n)
{
    if (!strcmp(cmd, "restart")) {
        save_clock();
        s_restart = true;
        snprintf(out, n, "restarting");
    } else if (!strcmp(cmd, "perf")) {
        snprintf(out, n, "frames=%u avg_ms=%u max_ms=%u over_33ms=%u strips/frame=%u.%u ops=%d cache=%u image_misses=%u", (unsigned)s_frames,
                 (unsigned)(s_frames ? s_frame_us / s_frames / 1000 : 0), (unsigned)(s_frame_max / 1000), (unsigned)s_frame_slow,
                 (unsigned)(s_frames ? s_strips / s_frames : 0), (unsigned)(s_frames ? s_strips * 10 / s_frames % 10 : 0), fc_gfx_ops(),
                 (unsigned)fc_img_cache_bytes(), fc_ui_image_misses());
        s_frames = s_frame_us = s_frame_max = s_frame_slow = s_strips = 0;
    } else if (!strncmp(cmd, "go ", 3)) {
        fc_ui_go(cmd + 3); s_force = 1;
        snprintf(out, n, "scene=%s", fc_ui_scene_id());
    } else if (!strncmp(cmd, "key ", 4) && strlen(cmd) >= 7) {
        int k = cmd[4] == 'u' ? K_UP : cmd[4] == 'd' ? K_DOWN : K_OK;
        fc_ui_key(k, cmd[6] == '1');
        snprintf(out, n, "key %d %c", k, cmd[6]);
    } else if (!strncmp(cmd, "time ", 5)) {
        fc_time_t t = {0};
        int y, m, d, hh, mm;
        if (sscanf(cmd + 5, "%d-%d-%d %d:%d", &y, &m, &d, &hh, &mm) == 5 && valid_date(y, m, d) &&
            y >= 2024 && y < 2100 && hh >= 0 && hh < 24 && mm >= 0 && mm < 60) {
            t.y = (int16_t)y; t.m = (int8_t)m; t.d = (int8_t)d; t.hh = (int8_t)hh; t.mm = (int8_t)mm;
            fc_set_now(&t); g_time_unknown = false;
        } else { snprintf(out, n, "error=invalid_time"); return; }
        fc_now(&t);
        snprintf(out, n, "time=%04d-%02d-%02d %02d:%02d wday=%d", t.y, t.m, t.d, t.hh, t.mm, t.wday);
    } else if (!strcmp(cmd, "pf clear")) {
        nvs_handle_t h;
        esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
        if (err == ESP_OK) {
            err = nvs_erase_key(h, "pf");
            if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
            if (err == ESP_OK) err = nvs_commit(h);
            nvs_close(h);
        }
        if (err == ESP_OK) PF = (fc_profile_t){1985, 6, 15, 4, 0, true};
        snprintf(out, n, "profile_clear=%s", esp_err_to_name(err));
    } else if (!strncmp(cmd, "pf ", 3)) {
        int y, m, d, h, g;
        if (sscanf(cmd + 3, "%d %d %d %d %d", &y, &m, &d, &h, &g) == 5 && valid_date(y, m, d) && h >= 0 && h < 12 && g >= 0 && g <= 1) {
            PF.y = (int16_t)y; PF.m = (int8_t)m; PF.d = (int8_t)d; PF.h = (int8_t)h; PF.g = (int8_t)g;
            fc_profile_save();
        } else { snprintf(out, n, "error=invalid_profile"); return; }
        snprintf(out, n, "pf=%d-%d-%d h=%d g=%d", PF.y, PF.m, PF.d, PF.h, PF.g);
    } else if (!strncmp(cmd, "sfx ", 4)) {
        int id;
        if (sscanf(cmd + 4, "%d", &id) != 1 || id < SFX_TICK || id > SFX_LOCK) { snprintf(out, n, "error=invalid_sfx"); return; }
        fc_sfx_play(id);
        snprintf(out, n, "sfx=%d", id);
    } else {
        fc_time_t t; fc_now(&t);
        snprintf(out, n, "scene=%s version=%s time=%04d-%02d-%02d %02d:%02d%s pf=%d-%d-%d h=%d g=%d%s batt=%dmV", fc_ui_scene_id(), CONFIG_APP_PROJECT_VER, t.y, t.m, t.d,
                 t.hh, t.mm, g_time_unknown ? "(未校准)" : "", PF.y, PF.m, PF.d, PF.h, PF.g, PF.demo ? "(示例)" : "", bsp_battery_mv());
    }
}

static void console_poll(void)
{
    static char line[96];
    static int len;
    uint8_t buf[32];
    int n = usb_serial_jtag_read_bytes(buf, sizeof(buf), 0);
    for (int i = 0; i < n; i++) {
        char c = (char)buf[i];
        if (c == '\r' || c == '\n') {
            if (len) {
                line[len] = 0;
                static char out[256];
                char *cmd = line[0] == '>' ? line + 1 : line;
                if (!strncmp(cmd, "nf ", 3)) cmd += 3;
                command(cmd, out, sizeof out);
                printf("NF %s heap=%u largest=%u uptime_ms=%u\n", out, (unsigned)esp_get_free_heap_size(),
                       (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), (unsigned)(esp_timer_get_time() / 1000));
                atomic_store(&s_last_input_us, esp_timer_get_time());
                if (s_restart) {
                    fflush(stdout);
                    usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(200));
                    esp_restart();
                }
            }
            len = 0;
        } else if (len < (int)sizeof(line) - 1) {
            line[len++] = c;
        }
    }
}

/* ---------- 主循环 ---------- */

static int s_strip_count;
static void flush_count(int y0, int rows, const uint16_t *pix, void *ctx) { s_strip_count++; flush(y0, rows, pix, ctx); }

static void game_task(void *arg)
{
    TickType_t wake = xTaskGetTickCount();
    int64_t last = esp_timer_get_time();
    for (;;) {
        key_ev_t ev;
        while (xQueueReceive(s_keys, &ev, 0) == pdTRUE) {
            ESP_LOGI(TAG, "button key=%u down=%u", ev.key, ev.down);
            fc_ui_key(ev.key, ev.down);
        }
        console_poll();
        int64_t t0 = esp_timer_get_time();
        int dt = (int)((t0 - last) / 1000);
        if (dt > 50) dt = 50;                // 与原型一致：一帧最多推进 50 ms
        if (dt < 1) dt = 1;
        last += (int64_t)dt * 1000;
        if (t0 - last > 50000) last = t0;
        fc_ui_step(dt);
        s_strip_count = 0;
        fc_ui_render(flush_count, NULL, s_force);
        s_force = 0;
        uint64_t used = (uint64_t)(esp_timer_get_time() - t0);
        s_frames++; s_frame_us += used; s_strips += (uint32_t)s_strip_count;
        if (used > s_frame_max) s_frame_max = used;
        if (used > FRAME_MS * 1000) s_frame_slow++;
        if (esp_timer_get_time() - atomic_load(&s_last_input_us) > IDLE_SLEEP_US) {
            flush_wait();
            atomic_store(&s_sleep_now, true);
            vTaskSuspend(NULL);
        }
        // 复杂画面超过帧预算时也必须让出 CPU，避免 IDLE0 看门狗饥饿。
        if (xTaskDelayUntil(&wake, pdMS_TO_TICKS(FRAME_MS)) == pdFALSE) {
            wake = xTaskGetTickCount();
            vTaskDelay(1);
        }
    }
}

/* ---------- 启动 ---------- */

static const uint8_t *map_assets(uint32_t *size)
{
    const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x41, "assets");
    if (!p) return NULL;
    uint8_t head[8];
    if (esp_partition_read(p, 0, head, sizeof head) != ESP_OK || memcmp(head, "FNAS", 4)) return NULL;
    uint32_t n = head[6] | head[7] << 8, end = 0;
    for (uint32_t i = 0; i < n && i < 512; i++) {
        uint8_t e[8];
        if (esp_partition_read(p, 8 + 40 * i + 32, e, sizeof e) != ESP_OK) return NULL;
        uint32_t off = e[0] | e[1] << 8 | e[2] << 16 | (uint32_t)e[3] << 24, len = e[4] | e[5] << 8 | e[6] << 16 | (uint32_t)e[7] << 24;
        if (off + len > end) end = off + len;
    }
    if (!end || end > p->size) return NULL;
    const void *ptr = NULL;
    esp_partition_mmap_handle_t h;
    uint32_t len = (end + 0xFFFF) & ~0xFFFFu;
    if (len > p->size) len = p->size;
    if (esp_partition_mmap(p, 0, len, ESP_PARTITION_MMAP_DATA, &ptr, &h) != ESP_OK) return NULL;
    ESP_LOGI(TAG, "assets %u bytes mapped (%u entries)", (unsigned)end, (unsigned)n);
    *size = end;
    return ptr;
}

static void fatal_screen(void)
{
    // 素材没刷进去：整屏红色，背光亮起来让人知道
    uint16_t *b = s_dma[0];
    for (int i = 0; i < FC_W * FC_STRIP; i++) b[i] = 0x00F8;   // 红（大端）
    for (int y = 0; y < FC_H; y += FC_STRIP) {
        xSemaphoreTake(s_free, portMAX_DELAY);
        if (esp_lcd_panel_draw_bitmap(s_panel, 0, y, FC_W, y + FC_STRIP, b) != ESP_OK) xSemaphoreGive(s_free);
    }
    bsp_display_backlight(BACKLIGHT);
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    warn("nvs", err);

    usb_serial_jtag_driver_config_t uc = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    if (usb_serial_jtag_driver_install(&uc) == ESP_OK) usb_serial_jtag_vfs_use_driver();

    ESP_ERROR_CHECK(bsp_display_init());
    s_panel = bsp_display_panel();
    s_free = xSemaphoreCreateCounting(2, 2);
    for (int i = 0; i < 2; i++) {
        s_dma[i] = heap_caps_malloc(FC_W * FC_STRIP * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        ESP_ERROR_CHECK(s_dma[i] ? ESP_OK : ESP_ERR_NO_MEM);
    }
    esp_lcd_panel_io_callbacks_t cbs = {.on_color_trans_done = on_trans_done};
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(bsp_display_io(), &cbs, NULL));

    warn("battery", bsp_battery_init());
    s_audio_ok = bsp_audio_init() == ESP_OK && bsp_audio_set_format(SAMPLE_RATE, 16, 1) == ESP_OK;
    if (!s_audio_ok) ESP_LOGE(TAG, "audio unavailable, running silent");

    s_d1970 = fc_days_from_1900(1970, 1, 1);
    esp_reset_reason_t why = esp_reset_reason();
    // 深睡、软件重启、看门狗都不断电，RTC 时间可信；上电、掉电复位后时间从 0 开始
    bool rtc_ok = why == ESP_RST_DEEPSLEEP || why == ESP_RST_SW || why == ESP_RST_PANIC || why == ESP_RST_INT_WDT ||
                  why == ESP_RST_TASK_WDT || why == ESP_RST_WDT;
    load_nvs(rtc_ok);
    ESP_LOGI(TAG, "reset reason %d, clock %s", (int)why, g_time_unknown ? "unknown" : "kept");

    uint32_t size = 0;
    const uint8_t *blob = map_assets(&size);
    if (!blob || !fc_ui_init(blob, size, esp_random())) {
        ESP_LOGE(TAG, "assets missing or broken: flash fortune_assets.bin to the assets partition");
        fatal_screen();
        for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    fc_ui_go("boot");
    atomic_store(&s_last_input_us, esp_timer_get_time());
    s_keys = xQueueCreate(16, sizeof(key_ev_t));
    fc_ui_render(flush, NULL, true);
    flush_wait();
    bsp_display_backlight(BACKLIGHT);

    xTaskCreate(game_task, "nf_game", 8192, NULL, 6, NULL);
    xTaskCreate(audio_task, "nf_audio", 4096, NULL, 8, NULL);
    ESP_ERROR_CHECK(bsp_button_init(on_button, NULL));
    ESP_LOGI(TAG, "NABO fortune ready, free heap %u, largest %u", (unsigned)esp_get_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}
