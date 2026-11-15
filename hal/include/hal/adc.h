#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <stdint.h>

#define ADC_MIN 0
#define ADC_MAX 4095
#define ADC_MIDPOINT ((ADC_MIN + ADC_MAX) / 2)

typedef struct {
    int fd;
    uint32_t speed_hz;
} adc_hal_t;

int adc_hal_init(adc_hal_t *adc, const char *device, uint32_t speed_hz);
int adc_hal_read_channel(adc_hal_t *adc, uint8_t channel);
int adc_hal_cleanup(adc_hal_t *adc);

#endif