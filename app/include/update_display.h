#ifndef _UPDATE_DISPLAY_H_
#define _UPDATE_DISPLAY_H_

#include "hal/lcd.h"
#include "settings.h"
#include "state.h"

// Alternates between two panes every few seconds:
//   Pane 1: current time + mode
//   Pane 2: Auto  -> moisture reading + state (Idle | Watering | Soaking)
//           Timed -> time until next watering + state (Idle | Watering)
// Cheap to call every loop iteration; it rate-limits its own LCD writes.
void update_display(lcd_hal_t *lcd, const settings_t *settings,
                    const state_t *state, int moisture);

                    
#endif