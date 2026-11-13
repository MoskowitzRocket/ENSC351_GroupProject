#ifndef HAL_LCD_H
#define HAL_LCD_H

#include <stddef.h>
#include <stdint.h>

#define LCD_HAL_DEFAULT_DEVICE "/dev/i2c-1"
#define LCD_HAL_DEFAULT_ADDRESS 0x3F
#define LCD_HAL_COLUMNS 16
#define LCD_HAL_ROWS 2

typedef enum {
    LCD_HAL_OK = 0,
    LCD_HAL_ERR_INVALID_ARG = -1,
    LCD_HAL_ERR_OPEN = -2,
    LCD_HAL_ERR_CONFIGURE = -3,
    LCD_HAL_ERR_IO = -4
} lcd_hal_status_t;

typedef struct {
    const char *device;
    uint8_t address;
} lcd_hal_config_t;

typedef struct {
    int fd;
} lcd_hal_t;

lcd_hal_status_t lcd_hal_init(lcd_hal_t *lcd, const lcd_hal_config_t *config);
lcd_hal_status_t lcd_hal_clear(lcd_hal_t *lcd);
lcd_hal_status_t lcd_hal_set_cursor(lcd_hal_t *lcd, unsigned int column,
                                    unsigned int row);
lcd_hal_status_t lcd_hal_write(lcd_hal_t *lcd, const char *text);
lcd_hal_status_t lcd_hal_write_n(lcd_hal_t *lcd, const char *text, size_t length);
lcd_hal_status_t lcd_hal_cleanup(lcd_hal_t *lcd);
const char *lcd_hal_strerror(lcd_hal_status_t status);

#endif
