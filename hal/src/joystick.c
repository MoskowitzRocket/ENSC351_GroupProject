// joystick.c 
/*
    joystick.c
    2026 Kale Moskowitz
    Implements HAL functions for the SPI joystick
    read_channel is 'private' function used to gather value from a channel
    joystick_hal_init sets up the joystick
    joystick_hal_read calls read_channel on channels defined in joystick.h
    joystick_hal_cleanup closes the joystick and cleans up after it's done

*/

#include "hal/joystick.h"
#include <stdio.h>

static float normalized_axis_value(int value, int minimum, int maximum) {
    if (value >= ADC_MIDPOINT) {
        if (maximum == ADC_MIDPOINT) {
            return (float)(value - ADC_MIDPOINT) /
                   (float)(ADC_MAX - ADC_MIDPOINT);
        }
        return (float)(value - ADC_MIDPOINT) /
               (float)(maximum - ADC_MIDPOINT);
    }

    if (minimum == ADC_MIDPOINT) {
        return (float)(value - ADC_MIDPOINT) /
               (float)(ADC_MIDPOINT - ADC_MIN);
    }
    return (float)(value - ADC_MIDPOINT) /
           (float)(ADC_MIDPOINT - minimum);
}

static direction_t find_direction(const joystick_hal_t *joystick, int x, int y) {
    float x_value = normalized_axis_value(x, joystick->x_min, joystick->x_max);
    float y_value = normalized_axis_value(y, joystick->y_min, joystick->y_max);
    float x_magnitude = x_value < 0.0f ? -x_value : x_value;
    float y_magnitude = y_value < 0.0f ? -y_value : y_value;

    if (x_magnitude < JOYSTICK_DIRECTION_DEADZONE &&
        y_magnitude < JOYSTICK_DIRECTION_DEADZONE) {
        return NO_DIR;
    }

    if (x_magnitude >= y_magnitude) {
        return x_value < 0.0f ? LEFT : RIGHT;
    }
    return y_value < 0.0f ? DOWN : UP;
}

static void update_calibration(joystick_hal_t *joystick, int x, int y) {
    const int calibration_deadzone = (int)((ADC_MAX -
                                            ADC_MIN) *
                                           JOYSTICK_CALIBRATION_DEADZONE);

    if (x < ADC_MIDPOINT - calibration_deadzone && x < joystick->x_min) {
        joystick->x_min = x;
    }
    if (x > ADC_MIDPOINT + calibration_deadzone && x > joystick->x_max) {
        joystick->x_max = x;
    }
    if (y < ADC_MIDPOINT - calibration_deadzone && y < joystick->y_min) {
        joystick->y_min = y;
    }
    if (y > ADC_MIDPOINT + calibration_deadzone && y > joystick->y_max) {
        joystick->y_max = y;
    }
}

int joystick_hal_init(joystick_hal_t *joystick, const char *device, uint32_t speed_hz) {
    if (joystick == NULL) {
        return -1;
    }

    if (adc_hal_init(&joystick->adc, device, speed_hz) != 0) {
        return -1;
    }
    joystick_hal_reset_calibration(joystick);
    return 0;
}

int joystick_hal_read(joystick_hal_t *joystick, joystick_pos_t *pos) {
    if (joystick == NULL || pos == NULL) {
        return -1;
    }

    int ch0 = adc_hal_read_channel(&joystick->adc, JOYSTICK_X_AXIS_CHANNEL);
    int ch1 = adc_hal_read_channel(&joystick->adc, JOYSTICK_Y_AXIS_CHANNEL);
    if (ch0 < 0 || ch1 < 0) {
        return -1;
    }

    pos->x_axis = ch0;
    pos->y_axis = ch1;
    update_calibration(joystick, ch0, ch1);
    pos->direction = find_direction(joystick, ch0, ch1);
    return 0;
}

void joystick_hal_reset_calibration(joystick_hal_t *joystick) {
    joystick->x_min = ADC_MIDPOINT;
    joystick->x_max = ADC_MIDPOINT;
    joystick->y_min = ADC_MIDPOINT;
    joystick->y_max = ADC_MIDPOINT;
}

int joystick_hal_cleanup(joystick_hal_t *joystick) {
    return joystick == NULL ? -1 : adc_hal_cleanup(&joystick->adc);
}