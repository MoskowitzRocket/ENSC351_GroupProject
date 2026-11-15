#ifndef _CAP_MOISTURE_H_
#define _CAP_MOISTURE_H_

#include "hal/adc.h"
#include <stdint.h>

#define CAP_MOISTURE_SENSOR_CHANNEL 2

typedef struct {
    adc_hal_t adc;
} cap_moisture_hal_t;

int cap_moisture_hal_init(cap_moisture_hal_t *sensor, const char *device,
                          uint32_t speed_hz);
int cap_moisture_hal_read(cap_moisture_hal_t *sensor);
int cap_moisture_hal_cleanup(cap_moisture_hal_t *sensor);

#endif