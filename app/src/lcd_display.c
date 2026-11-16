#include "lcd_display.h"

#include <stdio.h>
#include <string.h>

static lcd_hal_status_t write_line(lcd_hal_t *lcd, unsigned int row,
                                   const char *line)
{
    char padded_line[LCD_HAL_COLUMNS];
    size_t length = strlen(line);

    if (length > sizeof(padded_line))
        length = sizeof(padded_line);
    memset(padded_line, ' ', sizeof(padded_line));
    memcpy(padded_line, line, length);

    lcd_hal_status_t status = lcd_hal_set_cursor(lcd, 0, row);
    if (status != LCD_HAL_OK)
        return status;
    return lcd_hal_write_n(lcd, padded_line, sizeof(padded_line));
}

lcd_hal_status_t lcd_display_update(lcd_hal_t *lcd,
                                     const lcd_display_data_t *data)
{
    char current_line[LCD_HAL_COLUMNS + 1];
    char watering_line[LCD_HAL_COLUMNS + 1];
    struct tm current_tm;
    struct tm watering_tm;

    if (!lcd || !data)
        return LCD_HAL_ERR_INVALID_ARG;
    if (!localtime_r(&data->current_time, &current_tm) ||
        !localtime_r(&data->last_watering_time, &watering_tm))
        return LCD_HAL_ERR_IO;

    if (strftime(current_line, sizeof(current_line), "%m/%d %I:%M:%S",
                 &current_tm) == 0 ||
        strftime(watering_line, sizeof(watering_line), "LW %I:%M D:",
                 &watering_tm) == 0)
        return LCD_HAL_ERR_IO;

    if (snprintf(watering_line + strlen(watering_line),
                 sizeof(watering_line) - strlen(watering_line), "%03us",
                 data->watering_duration_seconds) < 0)
        return LCD_HAL_ERR_IO;

    lcd_hal_status_t status = write_line(lcd, 0, current_line);
    if (status != LCD_HAL_OK)
        return status;
    return write_line(lcd, 1, watering_line);
}

void lcd_display_placeholder(lcd_display_data_t *data)
{
    if (!data)
        return;

    data->current_time = time(NULL);
    data->last_watering_time = data->current_time - (time_t)3600;
    data->watering_duration_seconds = 30;
}