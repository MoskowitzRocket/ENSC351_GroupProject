#include "hal/lcd.h"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LCD_PIN_RS 0x01
#define LCD_PIN_EN 0x04
#define LCD_PIN_BACKLIGHT 0x08

static lcd_hal_status_t expander_write(lcd_hal_t *lcd, uint8_t value)
{
    return write(lcd->fd, &value, sizeof(value)) == (ssize_t)sizeof(value)
               ? LCD_HAL_OK
               : LCD_HAL_ERR_IO;
}

static lcd_hal_status_t write_nibble(lcd_hal_t *lcd, uint8_t nibble,
                                     uint8_t register_select)
{
    const uint8_t data = (nibble & 0xF0) | register_select |
                         LCD_PIN_BACKLIGHT;
    lcd_hal_status_t status = expander_write(lcd, data);
    if (status != LCD_HAL_OK)
        return status;

    status = expander_write(lcd, data | LCD_PIN_EN);
    if (status != LCD_HAL_OK)
        return status;
    usleep(2);

    status = expander_write(lcd, data & (uint8_t)~LCD_PIN_EN);
    if (status != LCD_HAL_OK)
        return status;
    usleep(50);
    return LCD_HAL_OK;
}

static lcd_hal_status_t send_byte(lcd_hal_t *lcd, uint8_t value,
                                  uint8_t register_select)
{
    lcd_hal_status_t status = write_nibble(lcd, value, register_select);
    if (status != LCD_HAL_OK)
        return status;
    return write_nibble(lcd, (uint8_t)(value << 4), register_select);
}

static lcd_hal_status_t send_command(lcd_hal_t *lcd, uint8_t command)
{
    return send_byte(lcd, command, 0);
}

static lcd_hal_status_t send_data(lcd_hal_t *lcd, uint8_t value)
{
    return send_byte(lcd, value, LCD_PIN_RS);
}

static lcd_hal_status_t initialize_controller(lcd_hal_t *lcd)
{
    lcd_hal_status_t status;

    usleep(50000);
    status = write_nibble(lcd, 0x30, 0);
    if (status != LCD_HAL_OK)
        return status;
    usleep(5000);
    status = write_nibble(lcd, 0x30, 0);
    if (status != LCD_HAL_OK)
        return status;
    usleep(5000);
    status = write_nibble(lcd, 0x30, 0);
    if (status != LCD_HAL_OK)
        return status;
    usleep(200);
    status = write_nibble(lcd, 0x20, 0);
    if (status != LCD_HAL_OK)
        return status;
    usleep(200);

    status = send_command(lcd, 0x28); /* 4-bit, 2 lines, 5x8 font */
    if (status != LCD_HAL_OK)
        return status;
    status = send_command(lcd, 0x08); /* display off */
    if (status != LCD_HAL_OK)
        return status;
    status = lcd_hal_clear(lcd);
    if (status != LCD_HAL_OK)
        return status;
    status = send_command(lcd, 0x06); /* cursor moves right */
    if (status != LCD_HAL_OK)
        return status;
    return send_command(lcd, 0x0C); /* display on, cursor off */
}

lcd_hal_status_t lcd_hal_init(lcd_hal_t *lcd, const lcd_hal_config_t *config)
{
    const char *device;
    uint8_t address;

    if (!lcd || !config)
        return LCD_HAL_ERR_INVALID_ARG;

    lcd->fd = -1;
    device = config->device ? config->device : LCD_HAL_DEFAULT_DEVICE;
    address = config->address ? config->address : LCD_HAL_DEFAULT_ADDRESS;

    lcd->fd = open(device, O_RDWR);
    if (lcd->fd < 0)
        return LCD_HAL_ERR_OPEN;
    if (ioctl(lcd->fd, I2C_SLAVE, address) < 0) {
        close(lcd->fd);
        lcd->fd = -1;
        return LCD_HAL_ERR_CONFIGURE;
    }

    if (initialize_controller(lcd) != LCD_HAL_OK) {
        close(lcd->fd);
        lcd->fd = -1;
        return LCD_HAL_ERR_IO;
    }
    return LCD_HAL_OK;
}

lcd_hal_status_t lcd_hal_clear(lcd_hal_t *lcd)
{
    lcd_hal_status_t status;

    if (!lcd || lcd->fd < 0)
        return LCD_HAL_ERR_INVALID_ARG;
    status = send_command(lcd, 0x01);
    if (status == LCD_HAL_OK)
        usleep(2000);
    return status;
}

lcd_hal_status_t lcd_hal_set_cursor(lcd_hal_t *lcd, unsigned int column,
                                    unsigned int row)
{
    static const uint8_t row_offsets[LCD_HAL_ROWS] = {0x00, 0x40};

    if (!lcd || lcd->fd < 0 || column >= LCD_HAL_COLUMNS ||
        row >= LCD_HAL_ROWS)
        return LCD_HAL_ERR_INVALID_ARG;
    return send_command(lcd, (uint8_t)(0x80 | (row_offsets[row] + column)));
}

lcd_hal_status_t lcd_hal_write_n(lcd_hal_t *lcd, const char *text, size_t length)
{
    if (!lcd || lcd->fd < 0 || (!text && length != 0))
        return LCD_HAL_ERR_INVALID_ARG;

    for (size_t i = 0; i < length; i++) {
        lcd_hal_status_t status = send_data(lcd, (uint8_t)text[i]);
        if (status != LCD_HAL_OK)
            return status;
    }
    return LCD_HAL_OK;
}

lcd_hal_status_t lcd_hal_write(lcd_hal_t *lcd, const char *text)
{
    if (!text)
        return LCD_HAL_ERR_INVALID_ARG;
    return lcd_hal_write_n(lcd, text, strlen(text));
}

lcd_hal_status_t lcd_hal_cleanup(lcd_hal_t *lcd)
{
    if (!lcd)
        return LCD_HAL_ERR_INVALID_ARG;
    if (lcd->fd >= 0) {
        close(lcd->fd);
        lcd->fd = -1;
    }
    return LCD_HAL_OK;
}

const char *lcd_hal_strerror(lcd_hal_status_t status)
{
    switch (status) {
    case LCD_HAL_OK:              return "ok";
    case LCD_HAL_ERR_INVALID_ARG: return "invalid argument";
    case LCD_HAL_ERR_OPEN:        return "cannot open I2C device";
    case LCD_HAL_ERR_CONFIGURE:   return "cannot configure I2C slave address";
    case LCD_HAL_ERR_IO:          return "LCD I/O error";
    default:                      return "unknown error";
    }
}
