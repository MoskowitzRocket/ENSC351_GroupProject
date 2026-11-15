#ifndef _RELAY_H_
#define _RELAY_H_

#include <stdbool.h>

#define RELAY_HAL_NUM_CHANNELS 4

typedef enum {
    RELAY_OK = 0,
    RELAY_ERR_INVALID_ARG = -1,
    RELAY_ERR_NOMEM       = -2,
    RELAY_ERR_CHIP        = -3,
    RELAY_ERR_REQUEST     = -4,
    RELAY_ERR_IO          = -5,
} relay_status_t;

typedef struct {
    const char  *chip_path;   /* e.g. "/dev/gpiochip2" */
    unsigned int offset;      /* line offset on that chip */
} relay_hal_line_t;

typedef struct {
    relay_hal_line_t lines[RELAY_HAL_NUM_CHANNELS];  /* In1..In4 */
    bool             active_low;                     /* true for most 4-ch modules */
} relay_hal_config_t;

typedef struct relay_hal relay_hal_t;

/* Opens each chip, requests each line as output. All relays start OFF. */
relay_status_t relay_hal_init(const relay_hal_config_t *cfg, relay_hal_t **out);

/* Turns all relays off, releases the lines, frees the handle. */
void relay_hal_deinit(relay_hal_t *hal);

/* channel is 0..3 (In1..In4). */
relay_status_t relay_hal_set(relay_hal_t *hal, unsigned int channel, bool on);
relay_status_t relay_hal_get(relay_hal_t *hal, unsigned int channel, bool *on);
relay_status_t relay_hal_toggle(relay_hal_t *hal, unsigned int channel);

/* Bit 0 = In1 ... bit 3 = In4. Applied channel by channel (not atomic,
 * since the lines live on different chips). */
relay_status_t relay_hal_set_mask(relay_hal_t *hal, unsigned int mask);
relay_status_t relay_hal_all_off(relay_hal_t *hal);

const char *relay_hal_strerror(relay_status_t status);

#endif /* _RELAY_H_ */