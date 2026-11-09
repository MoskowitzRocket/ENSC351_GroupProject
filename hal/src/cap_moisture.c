#include "hal/cap_moisture.h"

#include <stddef.h>

int cap_moisture_hal_init(cap_moisture_hal_t *sensor, const char *device,
                          uint32_t speed_hz) {
    if (sensor == NULL) {
        return -1;
    }

    return adc_hal_init(&sensor->adc, device, speed_hz);
}

int cap_moisture_hal_read(cap_moisture_hal_t *sensor) {
    if (sensor == NULL) {
        return -1;
    }

    return adc_hal_read_channel(&sensor->adc, CAP_MOISTURE_SENSOR_CHANNEL);
}

int cap_moisture_hal_cleanup(cap_moisture_hal_t *sensor) {
    return sensor == NULL ? -1 : adc_hal_cleanup(&sensor->adc);
}
