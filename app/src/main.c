// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "hal/cap_moisture.h"
#include "hal/joystick.h"
#include "hal/lcd.h"
#include "hal/relay.h"
#include "hal/button.h"
#include "settings.h"
#include "setup_mode.h"
#include "state.h"
#include "update_display.h"

typedef struct {
    joystick_hal_t joystick;
    cap_moisture_hal_t moisture;
    relay_hal_t *relay;
    lcd_hal_t lcd;
    bool joystick_initialized;
    bool moisture_initialized;
    bool lcd_initialized;
    bool button_initialized;
} app_hal_t;

// typedef struct {
//   bool watering;
//   bool soaking;              // Auto mode: waiting after a burst
//   uint32_t lastWaterTime;    // millis() of last watering
//   uint32_t waterStartTime;
//   uint32_t totalThisCycle;   // for the safety cutoff
// } state_t;


#define PUMP_RELAY_CHANNEL 0U



static void cleanup_hal(app_hal_t *hal)
{
    if (hal->button_initialized)
        button_cleanup();
    if (hal->lcd_initialized)
        lcd_hal_cleanup(&hal->lcd);
    if (hal->relay)
        relay_hal_deinit(hal->relay);
    if (hal->moisture_initialized)
        cap_moisture_hal_cleanup(&hal->moisture);
    if (hal->joystick_initialized)
        joystick_hal_cleanup(&hal->joystick);
}

static int init_hal(app_hal_t *hal)
{
    const char *device = "/dev/spidev0.0";
    const uint32_t speed_hz = 250000;
    const relay_hal_config_t relay_config = {
        .lines = {
            { "/dev/gpiochip2", 8 },
            { "/dev/gpiochip1", 33 },
            { "/dev/gpiochip1", 41 },
            { "/dev/gpiochip0", 7 },
        },
        .active_low = true,
    };
    const lcd_hal_config_t lcd_config = {
        .device = LCD_HAL_DEFAULT_DEVICE,
        .address = LCD_HAL_DEFAULT_ADDRESS,
    };

    hal->relay = NULL;
    hal->joystick_initialized = false;
    hal->moisture_initialized = false;
    hal->lcd_initialized = false;
    hal->button_initialized = false;

    if (joystick_hal_init(&hal->joystick, device, speed_hz) != 0) {
        fprintf(stderr, "Joystick init failed\n");
        return 1;
    }
    hal->joystick_initialized = true;

    if (cap_moisture_hal_init(&hal->moisture, device, speed_hz) != 0) {
        fprintf(stderr, "Moisture sensor init failed\n");
        cleanup_hal(hal);
        return 1;
    }
    hal->moisture_initialized = true;

    relay_status_t relay_status = relay_hal_init(&relay_config, &hal->relay);
    if (relay_status != RELAY_OK) {
        fprintf(stderr, "Relay init: %s\n", relay_hal_strerror(relay_status));
        cleanup_hal(hal);
        return 1;
    }

    lcd_hal_status_t lcd_status = lcd_hal_init(&hal->lcd, &lcd_config);
    if (lcd_status != LCD_HAL_OK) {
        fprintf(stderr, "LCD init: %s\n", lcd_hal_strerror(lcd_status));
        cleanup_hal(hal);
        return 1;
    }
    hal->lcd_initialized = true;

    button_init();
    hal->button_initialized = true;
    return 0;
}

// static int update_timed_watering(app_hal_t *hal, const settings_t *settings,
//                                  state_t *state)
// {
//     const uint32_t now = (uint32_t)time(NULL);

//     if (settings->mode != MODE_TIMED)
//         return 0;

//     if (!state->watering) {
//         const uint32_t interval_s = settings_interval_ms(settings) / 1000U;
//         if (now - state->lastWaterTime < interval_s)
//             return 0;

//         relay_status_t status =
//             relay_hal_set(hal->relay, PUMP_RELAY_CHANNEL, true);
//         if (status != RELAY_OK) {
//             fprintf(stderr, "Pump start failed: %s\n",
//                     relay_hal_strerror(status));
//             return -1;
//         }

//         state->watering = true;
//         state->waterStartTime = now;
//         printf("[timed] watering started\n");
//         return 0;
//     }

//     if (now - state->waterStartTime < settings->durationSec)
//         return 0;

//     relay_status_t status =
//         relay_hal_set(hal->relay, PUMP_RELAY_CHANNEL, false);
//     if (status != RELAY_OK) {
//         fprintf(stderr, "Pump shutdown failed: %s\n",
//                 relay_hal_strerror(status));
//         return -1;
//     }

//     state->watering = false;
//     state->lastWaterTime = now;
//     state->waterStartTime = 0;
//     state->totalThisCycle = 0;
//     printf("[timed] watering complete; timer reset\n");
//     return 0;
// }


int main(void)
{
    app_hal_t hal = {0};
    settings_t settings;
    state_t state = {
        .watering = false,
        .lastWaterTime = (uint32_t)time(NULL),
        .waterStartTime = 0,
        .totalThisCycle = 0,
    };
    joystick_pos_t position = {0};
    bool input_was_pressed = false;

    if (init_hal(&hal) != 0)
        return 1;

    settings_set_defaults(&settings);

    const setup_mode_ctx_t setup_ctx = {
        .joystick = &hal.joystick,
        .lcd = &hal.lcd,
        .relay = hal.relay,
        .pump_channel = PUMP_RELAY_CHANNEL,
    };

    printf("Plant Waterer Software Starting!!\n");

    while (true) {
        bool input_now = setup_mode_input_active(&hal.joystick, &position);

        if (input_now && !input_was_pressed) {
            printf("Input detected, entering setup\n");
            if (setup_mode_run(&setup_ctx, &settings) != 0)
                break;   /* pump failed to shut off */

            /* EEPROM save goes here once you add it */
            state.lastWaterTime  = (uint32_t)time(NULL);
            state.totalThisCycle = 0;
            state.watering       = false;
            state.waterStartTime = 0;

            input_now = true;   /* don't treat the final SELECT as a new event */
        }
        input_was_pressed = input_now;

        if (settings.mode == MODE_AUTO) {
            // printf("[AUTO] Handled later :)\n");
        } else if (settings.mode == MODE_TIMED) {
            // if (update_timed_watering(&hal, &settings, &state) != 0)
            //     break;
        }

        // Update Display
        int moisture = 0;
        update_display(&hal.lcd, &settings, &state, moisture);

        usleep(50 * 1000);
    }

    cleanup_hal(&hal);
    return 0;
}