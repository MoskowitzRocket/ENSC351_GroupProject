#ifndef _LCD_DISPLAY_H_
#define _LCD_DISPLAY_H_

#include <time.h>

#include "hal/lcd.h"

typedef struct {
    time_t current_time;
    time_t last_watering_time;
    unsigned int watering_duration_seconds;
} lcd_display_data_t;

/*
 * Formats and displays the current time, last watering time, and watering
 * duration. The caller owns the watering state and can replace the placeholder
 * values without changing the display implementation.
 */
lcd_hal_status_t lcd_display_update(lcd_hal_t *lcd,
                                     const lcd_display_data_t *data);

void lcd_display_placeholder(lcd_display_data_t *data);

#endif