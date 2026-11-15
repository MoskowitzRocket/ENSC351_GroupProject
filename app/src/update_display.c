#include "update_display.h"

#include <stdio.h>
#include <time.h>

#define PANE_PERIOD_MS    10000U   // how long each pane is shown
#define REFRESH_PERIOD_MS 250U    // min time between LCD writes

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000U + (uint64_t)ts.tv_nsec / 1000000U;
}

/* Two-line update. Lines are padded to 16 chars, so no clear/flicker. */
static void show(lcd_hal_t *lcd, const char *line1, const char *line2)
{
    char buf[LCD_HAL_COLUMNS + 1];

    snprintf(buf, sizeof buf, "%-16.16s", line1);
    lcd_hal_set_cursor(lcd, 0, 0);
    lcd_hal_write(lcd, buf);

    snprintf(buf, sizeof buf, "%-16.16s", line2);
    lcd_hal_set_cursor(lcd, 0, 1);
    lcd_hal_write(lcd, buf);
}

static const char *state_name(const settings_t *settings, const state_t *state)
{
    if (state->watering)
        return "Watering";
    if (settings->mode == MODE_AUTO && state->soaking)
        return "Soaking";
    return "Idle";
}

/* Time until next watering, formatted to fit 16 chars. */
static void format_countdown(char *out, size_t size, const settings_t *settings,
                             const state_t *state)
{
    uint32_t now = (uint32_t)time(NULL);
    uint32_t elapsed = now - state->lastWaterTime;
    uint32_t interval_s = settings_interval_ms(settings) / 1000U;
    uint32_t remaining = (interval_s > elapsed) ? interval_s - elapsed : 0;

    uint32_t d = remaining / 86400U;
    uint32_t h = (remaining % 86400U) / 3600U;
    uint32_t m = (remaining % 3600U) / 60U;
    uint32_t s = remaining % 60U;

    if (d > 0)
        snprintf(out, size, "Next %ud %02uh %02um", (unsigned)d, (unsigned)h,
                 (unsigned)m);
    else
        snprintf(out, size, "Next %02u:%02u:%02u", (unsigned)h, (unsigned)m,
                 (unsigned)s);
}

void update_display(lcd_hal_t *lcd, const settings_t *settings,
                    const state_t *state, int moisture)
{
    static uint64_t last_refresh = 0;

    uint64_t t = now_ms();
    if (last_refresh != 0 && t - last_refresh < REFRESH_PERIOD_MS)
        return;
    last_refresh = t;

    char line1[32];
    char line2[32];
    bool info_pane = (t / PANE_PERIOD_MS) % 2 == 1;

    if (!info_pane) {
        /* Pane 1: current time + mode */
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        strftime(line1, sizeof line1, "%H:%M:%S", &tm_now);
        snprintf(line2, sizeof line2, "Mode: %s",
                 settings->mode == MODE_AUTO ? "Auto" : "Timed");
    } else if (settings->mode == MODE_AUTO) {
        /* Pane 2 (auto): moisture + state */
        snprintf(line1, sizeof line1, "Moisture: %d", moisture);
        snprintf(line2, sizeof line2, "State: %s", state_name(settings, state));
    } else {
        /* Pane 2 (timed): countdown + state */
        format_countdown(line1, sizeof line1, settings, state);
        snprintf(line2, sizeof line2, "State: %s", state_name(settings, state));
    }

    show(lcd, line1, line2);
}