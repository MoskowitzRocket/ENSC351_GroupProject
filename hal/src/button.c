#include "hal/button.h"

#include <gpiod.h>
#include <stdio.h>

// BeagleY-AI physical pin 7 (GPIO4)
#define BUTTON_GPIO_CHIP "/dev/gpiochip1"
#define BUTTON_GPIO_OFFSET 38

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *request = NULL;

void button_init(void)
{
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *line_config = NULL;
    struct gpiod_request_config *request_config = NULL;

    unsigned int offset = BUTTON_GPIO_OFFSET;

    // Open GPIO chip
    chip = gpiod_chip_open(BUTTON_GPIO_CHIP);

    if (!chip)
    {
        perror("Button: failed to open GPIO chip");
        goto cleanup;
    }

    // Allocate configuration objects
    settings = gpiod_line_settings_new();
    line_config = gpiod_line_config_new();
    request_config = gpiod_request_config_new();

    if (!settings || !line_config || !request_config)
    {
        perror("Button: configuration allocation failed");
        goto cleanup;
    }

    // Configure GPIO as input
    if (gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_INPUT) < 0)
    {
        perror("Button: failed to set direction");
        goto cleanup;
    }

    // Enable internal pull-up
    if (gpiod_line_settings_set_bias(
            settings, GPIOD_LINE_BIAS_PULL_UP) < 0)
    {
        perror("Button: failed to set pull-up");
        goto cleanup;
    }

    // Add GPIO line to configuration
    if (gpiod_line_config_add_line_settings(
            line_config, &offset, 1, settings) < 0)
    {
        perror("Button: failed to configure GPIO line");
        goto cleanup;
    }

    // Set consumer name
    gpiod_request_config_set_consumer(
        request_config, "plant-waterer-button");

    // Request GPIO line
    request = gpiod_chip_request_lines(
        chip, request_config, line_config);

    if (!request)
    {
        perror("Button: failed to request GPIO line");
        goto cleanup;
    }

    // printf("Button initialized successfully\n");

cleanup:
    gpiod_request_config_free(request_config);
    gpiod_line_config_free(line_config);
    gpiod_line_settings_free(settings);

    if (!request && chip)
    {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}

bool button_is_button_pressed(void)
{
    if (!request)
        return false;

    enum gpiod_line_value value =
        gpiod_line_request_get_value(request, BUTTON_GPIO_OFFSET);

    if (value == GPIOD_LINE_VALUE_ERROR)
    {
        perror("Button: failed to read GPIO");
        return false;
    }

    // Active-low: pressed = LOW
    return value == GPIOD_LINE_VALUE_INACTIVE;
}

void button_cleanup(void)
{
    if (request)
    {
        gpiod_line_request_release(request);
        request = NULL;
    }

    if (chip)
    {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}