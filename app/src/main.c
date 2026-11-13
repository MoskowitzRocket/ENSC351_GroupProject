// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "hal/cap_moisture.h"
#include "hal/joystick.h"
#include "hal/lcd.h"
#include "hal/relay.h"

int main()
{
    // const char *device = "/dev/spidev0.0";
    // const uint32_t speed_hz = 250000;
    // joystick_hal_t joystick = {.adc.fd = -1};
    // cap_moisture_hal_t moisture = {.adc.fd = -1};
    // joystick_pos_t position;

    // if (joystick_hal_init(&joystick, device, speed_hz) != 0) {
    //     joystick_hal_cleanup(&joystick);
    //     return 1;
    // }
    // if (cap_moisture_hal_init(&moisture, device, speed_hz) != 0) {
    //     joystick_hal_cleanup(&joystick);
    //     cap_moisture_hal_cleanup(&moisture);
    //     return 1;
    // }

    // const struct timespec sample_interval = {
    //     .tv_sec = 0,
    //     .tv_nsec = 100000000
    // };

    // while (1) {
    //     int moisture_value = cap_moisture_hal_read(&moisture);
    //     if (joystick_hal_read(&joystick, &position) != 0 || moisture_value < 0) {
    //         fprintf(stderr, "Failed to read joystick or moisture sensor\n");
    //         joystick_hal_cleanup(&joystick);
    //         cap_moisture_hal_cleanup(&moisture);
    //         return 1;
    //     }

    //     printf("Joystick: x=%d, y=%d; moisture ADC: %d\n",
    //            position.x_axis, position.y_axis, moisture_value);
    //     nanosleep(&sample_interval, NULL);
    // }
    relay_hal_config_t cfg = {
        .lines = {
            { "/dev/gpiochip2",  8 },   /* In1 -> pin 11, GPIO17 */
            { "/dev/gpiochip1", 33 },   /* In2 -> pin 13, GPIO27 */
            { "/dev/gpiochip1", 41 },   /* In3 -> pin 15, GPIO22 */
            { "/dev/gpiochip0",  7 },   /* In4 -> pin 16, GPIO23 */
        },
        .active_low = true,
    };

    relay_hal_t *relay;
    relay_status_t s = relay_hal_init(&cfg, &relay);
    if (s != RELAY_OK) {
        fprintf(stderr, "init: %s\n", relay_hal_strerror(s));
        return 1;
    }

    for (unsigned int ch = 0; ch < RELAY_HAL_NUM_CHANNELS; ch++) {
        relay_hal_set(relay, ch, true);
        sleep(1);
        relay_hal_set(relay, ch, false);
    }

    relay_hal_deinit(relay);

    lcd_hal_t lcd;
    const lcd_hal_config_t lcd_config = {
        .device = LCD_HAL_DEFAULT_DEVICE,
        .address = LCD_HAL_DEFAULT_ADDRESS,
    };

    lcd_hal_status_t lcd_status = lcd_hal_init(&lcd, &lcd_config);
    if (lcd_status != LCD_HAL_OK) {
        fprintf(stderr, "LCD init: %s\n", lcd_hal_strerror(lcd_status));
        return 1;
    }

    lcd_status = lcd_hal_set_cursor(&lcd, 0, 0);
    if (lcd_status == LCD_HAL_OK)
        lcd_status = lcd_hal_write(&lcd, "Hello BeagleY!");
    if (lcd_status == LCD_HAL_OK)
        lcd_status = lcd_hal_set_cursor(&lcd, 0, 1);
    if (lcd_status == LCD_HAL_OK)
        lcd_status = lcd_hal_write(&lcd, "LCD HAL works!");
    if (lcd_status != LCD_HAL_OK) {
        fprintf(stderr, "LCD test: %s\n", lcd_hal_strerror(lcd_status));
        lcd_hal_cleanup(&lcd);
        return 1;
    }

    printf("LCD test passed; message displayed for 5 seconds.\n");
    sleep(5);
    lcd_hal_cleanup(&lcd);
    return 0;


}