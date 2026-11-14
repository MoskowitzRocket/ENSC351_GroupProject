#include "setup_mode.h"

#include <stdio.h>
#include <unistd.h>

#include "hal/button.h"

typedef enum {
    EV_NONE,
    EV_UP,
    EV_DOWN,
    EV_LEFT,
    EV_RIGHT,
    EV_SELECT
} ui_event_t;

static const char *event_name(ui_event_t event)
{
    switch (event) {
    case EV_UP:     return "UP";
    case EV_DOWN:   return "DOWN";
    case EV_LEFT:   return "LEFT";
    case EV_RIGHT:  return "RIGHT";
    case EV_SELECT: return "SELECT";
    case EV_NONE:   return "NONE";
    }
    return "UNKNOWN";
}

static void debug_print_settings(const char *label, const settings_t *settings)
{
    printf("[setup] %s: mode=%s freq=%u %s duration=%u sec "
           "moisture_threshold=%u burst=%u sec soak=%u sec max=%u sec\n",
           label,
           settings->mode == MODE_AUTO ? "AUTO" : "TIMED",
           (unsigned int)settings->freqValue,
           settings->freqUnit == UNIT_MIN
               ? "MIN"
               : settings->freqUnit == UNIT_HOUR ? "HOUR" : "DAY",
           (unsigned int)settings->durationSec,
           (unsigned int)settings->moistureThreshold,
           (unsigned int)settings->burstSec,
           (unsigned int)settings->soakDelaySec,
           (unsigned int)settings->maxWaterSec);
}

/* ---------- input ---------- */

bool setup_mode_input_active(joystick_hal_t *joystick, joystick_pos_t *pos)
{
    if (button_is_button_pressed())
        return true;

    if (joystick_hal_read(joystick, pos) == 0 && pos->direction != NO_DIR)
        return true;

    return false;
}

/* Instantaneous read of the inputs (button has priority). */
static ui_event_t ui_read_raw(const setup_mode_ctx_t *ctx)
{
    if (button_is_button_pressed())
        return EV_SELECT;

    joystick_pos_t pos = {0};
    if (joystick_hal_read(ctx->joystick, &pos) == 0) {
        switch (pos.direction) {
        case UP:    return EV_UP;
        case DOWN:  return EV_DOWN;
        case LEFT:  return EV_LEFT;
        case RIGHT: return EV_RIGHT;
        default:    break;
        }
    }
    return EV_NONE;
}

/* Blocks until one input event. A held direction auto-repeats. */
static ui_event_t ui_wait_event(const setup_mode_ctx_t *ctx)
{
    static ui_event_t held = EV_NONE;
    static bool repeating = false;

    if (held != EV_NONE && held != EV_SELECT) {
        unsigned int delay_ms = repeating ? 80 : 400;
        bool still_held = true;

        for (unsigned int t = 0; t < delay_ms; t += 10) {
            usleep(10 * 1000);
            if (ui_read_raw(ctx) != held) {
                still_held = false;
                break;
            }
        }
        if (still_held) {
            repeating = true;
            return held;
        }
    }

    held = EV_NONE;
    repeating = false;

    /* Wait for release (also swallows the press that triggered setup). */
    while (ui_read_raw(ctx) != EV_NONE)
        usleep(10 * 1000);

    /* Wait for a fresh press. */
    ui_event_t ev;
    while ((ev = ui_read_raw(ctx)) == EV_NONE)
        usleep(10 * 1000);

    usleep(30 * 1000);   /* debounce */
    held = ev;
    printf("[setup] input event: %s\n", event_name(ev));
    return ev;
}

/* ---------- display ---------- */

/* Two-line LCD update. Lines are padded to 16 chars, so no clear/flicker. */
static void lcd_show(const setup_mode_ctx_t *ctx, const char *line1,
                     const char *line2)
{
    char buf[LCD_HAL_COLUMNS + 1];

    snprintf(buf, sizeof buf, "%-16.16s", line1);
    lcd_hal_set_cursor(ctx->lcd, 0, 0);
    lcd_hal_write(ctx->lcd, buf);

    snprintf(buf, sizeof buf, "%-16.16s", line2);
    lcd_hal_set_cursor(ctx->lcd, 0, 1);
    lcd_hal_write(ctx->lcd, buf);
}

/* ---------- widgets ---------- */

/* Pick one of N options. Returns the chosen index. */
static int ui_choose_option(const setup_mode_ctx_t *ctx, const char *title,
                            const char *const *options, int count, int initial)
{
    int idx = initial;

    while (true) {
        char line[24];
        snprintf(line, sizeof line, "< %s >", options[idx]);
        lcd_show(ctx, title, line);

        switch (ui_wait_event(ctx)) {
        case EV_UP:
        case EV_RIGHT:
            idx = (idx + 1) % count;
            break;
        case EV_DOWN:
        case EV_LEFT:
            idx = (idx + count - 1) % count;
            break;
        case EV_SELECT:
            printf("[setup] %s selected: %s\n", title, options[idx]);
            return idx;
        default:
            break;
        }
    }
}

/* Pick a number in [min, max]. Up/down = fine step, left/right = coarse. */
static uint16_t ui_choose_value(const setup_mode_ctx_t *ctx, const char *title,
                                uint16_t min, uint16_t max, uint16_t initial,
                                int fine, int coarse, const char *suffix)
{
    int value = initial;
    if (value < min) value = min;
    if (value > max) value = max;

    while (true) {
        char line[24];
        snprintf(line, sizeof line, "%d %s", value, suffix);
        lcd_show(ctx, title, line);

        switch (ui_wait_event(ctx)) {
        case EV_UP:     value += fine;   break;
        case EV_DOWN:   value -= fine;   break;
        case EV_RIGHT:  value += coarse; break;
        case EV_LEFT:   value -= coarse; break;
        case EV_SELECT:
            printf("[setup] %s selected: %u %s\n", title,
                   (unsigned int)value, suffix);
            return (uint16_t)value;
        default:        break;
        }

        if (value < min) value = min;
        if (value > max) value = max;
    }
}

/* ---------- public entry point ---------- */

int setup_mode_run(const setup_mode_ctx_t *ctx, settings_t *settings)
{
    static const char *const mode_names[] = { "Auto", "Timed" };
    static const char *const unit_names[] = { "Min", "Hour", "Day" };
    /* Indexed by enum Unit. Keeps interval_ms inside uint32_t. */
    static const uint16_t unit_max[] = { 59, 23, 30 };

    printf("[setup] entering setup mode\n");
    debug_print_settings("initial settings", settings);

    /* Pump OFF */
    relay_status_t rs = relay_hal_set(ctx->relay, ctx->pump_channel, false);
    if (rs != RELAY_OK) {
        fprintf(stderr, "Pump shutdown failed: %s\n", relay_hal_strerror(rs));
        fprintf(stderr, "[setup] failed to enter setup mode\n");
        return -1;
    }
    printf("[setup] pump off\n");

    lcd_show(ctx, "Setup Mode", "Pump OFF");
    usleep(1000 * 1000);

    settings->mode = (enum Mode)ui_choose_option(ctx, "Mode:", mode_names, 2,
                                                 (int)settings->mode);

    if (settings->mode == MODE_AUTO) {
        /* 0..1023 assumes a 10-bit ADC; adjust to your sensor's range. */
        settings->moistureThreshold = ui_choose_value(
            ctx, "Moisture thresh",
                0,
                4095,
                settings->moistureThreshold,
                1,
                25,
                "");
        printf("[setup] auto settings selected\n");
    } else {
        settings->freqUnit = (enum Unit)ui_choose_option(
            ctx, "Every:", unit_names, 3, (int)settings->freqUnit);

        settings->freqValue = ui_choose_value(
            ctx, "Frequency", 1, unit_max[settings->freqUnit],
            settings->freqValue, 1, 5, unit_names[settings->freqUnit]);

        settings->durationSec = ui_choose_value(
            ctx, "Water length", 1, 600, settings->durationSec, 1, 10, "sec");

        char line[24];
        snprintf(line, sizeof line, "%lu ms",
                 (unsigned long)settings_interval_ms(settings));
        lcd_show(ctx, "Interval set", line);
        usleep(1500 * 1000);
        printf("[setup] timed interval: %s\n", line);
    }

    debug_print_settings("final settings", settings);
    lcd_show(ctx, "Setup done", "Resuming...");
    usleep(1000 * 1000);
    return 0;
}