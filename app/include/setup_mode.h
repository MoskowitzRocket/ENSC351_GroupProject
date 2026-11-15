#ifndef SETUP_MODE_H
#define SETUP_MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "hal/joystick.h"
#include "hal/lcd.h"
#include "hal/relay.h"
#include "settings.h"

// The HAL pieces the setup UI needs. Pointers are borrowed, not owned.
typedef struct {
    joystick_hal_t *joystick;
    lcd_hal_t *lcd;
    relay_hal_t *relay;
    unsigned int pump_channel;
} setup_mode_ctx_t;

// True if the button is pressed or the joystick is deflected.
// `pos` is scratch space for the joystick read.
bool setup_mode_input_active(joystick_hal_t *joystick, joystick_pos_t *pos);

// Runs the interactive setup (pump off, choose Auto/Timed, etc.),
// updating `settings`. Blocks until the user finishes.
// Returns 0 on success, -1 if the pump could not be turned off.
int setup_mode_run(const setup_mode_ctx_t *ctx, settings_t *settings);

#endif /* SETUP_MODE_H */