/*
    joystick.h
    2026 Kale Moskowitz
    Defines the SPI joystick data structures and HAL function prototypes.

*/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_
#include <stdint.h>

#include "hal/adc.h"

#define JOYSTICK_X_AXIS_CHANNEL 0
#define JOYSTICK_Y_AXIS_CHANNEL 1
#define JOYSTICK_DIRECTION_DEADZONE 0.20f
#define JOYSTICK_CALIBRATION_DEADZONE 0.05f

typedef struct {
    adc_hal_t adc;
    int x_min;
    int x_max;
    int y_min;
    int y_max;
} joystick_hal_t;

typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NO_DIR
} direction_t;

typedef struct {
    int x_axis;
    int y_axis;
    direction_t direction;
} joystick_pos_t;

int joystick_hal_init(joystick_hal_t *joystick, const char *device, uint32_t speed_hz);

int joystick_hal_read(joystick_hal_t *joystick, joystick_pos_t *pos);

void joystick_hal_reset_calibration(joystick_hal_t *joystick);

int joystick_hal_cleanup(joystick_hal_t *joystick);




#endif /* _JOYSTICK_H_ */