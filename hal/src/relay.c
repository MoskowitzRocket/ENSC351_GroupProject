#include "hal/relay.h"

#include <gpiod.h>
#include <stdlib.h>

typedef struct {
    struct gpiod_chip         *chip;
    struct gpiod_line_request *req;
    unsigned int               offset;
} relay_channel_t;

struct relay_hal {
    relay_channel_t ch[RELAY_HAL_NUM_CHANNELS];
};

static void channel_close(relay_channel_t *c)
{
    if (c->req)
        gpiod_line_request_release(c->req);
    if (c->chip)
        gpiod_chip_close(c->chip);
    c->req  = NULL;
    c->chip = NULL;
}

/* Fills in the three config objects. Returns true on success. */
static bool configure_request(struct gpiod_line_settings  *settings,
                              struct gpiod_line_config    *line_cfg,
                              struct gpiod_request_config *req_cfg,
                              const unsigned int          *offset,
                              bool                         active_low)
{
    /* Initial value INACTIVE => relay OFF from the moment of the request. */
    if (gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT))
        return false;
    if (gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE))
        return false;
    gpiod_line_settings_set_active_low(settings, active_low);

    if (gpiod_line_config_add_line_settings(line_cfg, offset, 1, settings))
        return false;

    gpiod_request_config_set_consumer(req_cfg, "relay_hal");
    return true;
}

static relay_status_t channel_open(relay_channel_t *c,
                                   const relay_hal_line_t *line,
                                   bool active_low)
{
    if (!line->chip_path)
        return RELAY_ERR_INVALID_ARG;

    c->offset = line->offset;
    c->chip = gpiod_chip_open(line->chip_path);
    if (!c->chip)
        return RELAY_ERR_CHIP;

    struct gpiod_line_settings  *settings = gpiod_line_settings_new();
    struct gpiod_line_config    *line_cfg = gpiod_line_config_new();
    struct gpiod_request_config *req_cfg  = gpiod_request_config_new();

    relay_status_t rc = RELAY_ERR_NOMEM;

    if (settings && line_cfg && req_cfg) {
        rc = RELAY_ERR_REQUEST;
        if (configure_request(settings, line_cfg, req_cfg,
                              &c->offset, active_low)) {
            c->req = gpiod_chip_request_lines(c->chip, req_cfg, line_cfg);
            if (c->req)
                rc = RELAY_OK;
        }
    }

    /* The libgpiod free functions accept NULL, so this is safe even if
     * one of the allocations above failed. */
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (rc != RELAY_OK)
        channel_close(c);   /* closes the chip; req is still NULL */

    return rc;
}

relay_status_t relay_hal_init(const relay_hal_config_t *cfg, relay_hal_t **out)
{
    if (!cfg || !out)
        return RELAY_ERR_INVALID_ARG;

    relay_hal_t *hal = calloc(1, sizeof(*hal));
    if (!hal)
        return RELAY_ERR_NOMEM;

    for (int i = 0; i < RELAY_HAL_NUM_CHANNELS; i++) {
        relay_status_t s = channel_open(&hal->ch[i], &cfg->lines[i],
                                        cfg->active_low);
        if (s != RELAY_OK) {
            for (int j = 0; j < i; j++)
                channel_close(&hal->ch[j]);
            free(hal);
            return s;
        }
    }

    *out = hal;
    return RELAY_OK;
}

void relay_hal_deinit(relay_hal_t *hal)
{
    if (!hal)
        return;

    relay_hal_all_off(hal);
    for (int i = 0; i < RELAY_HAL_NUM_CHANNELS; i++)
        channel_close(&hal->ch[i]);
    free(hal);
}

relay_status_t relay_hal_set(relay_hal_t *hal, unsigned int channel, bool on)
{
    if (!hal || channel >= RELAY_HAL_NUM_CHANNELS)
        return RELAY_ERR_INVALID_ARG;

    relay_channel_t *c = &hal->ch[channel];
    int r = gpiod_line_request_set_value(c->req, c->offset,
                                         on ? GPIOD_LINE_VALUE_ACTIVE
                                            : GPIOD_LINE_VALUE_INACTIVE);
    return r ? RELAY_ERR_IO : RELAY_OK;
}

relay_status_t relay_hal_get(relay_hal_t *hal, unsigned int channel, bool *on)
{
    if (!hal || !on || channel >= RELAY_HAL_NUM_CHANNELS)
        return RELAY_ERR_INVALID_ARG;

    relay_channel_t *c = &hal->ch[channel];
    enum gpiod_line_value v = gpiod_line_request_get_value(c->req, c->offset);
    if (v == GPIOD_LINE_VALUE_ERROR)
        return RELAY_ERR_IO;

    *on = (v == GPIOD_LINE_VALUE_ACTIVE);
    return RELAY_OK;
}

relay_status_t relay_hal_toggle(relay_hal_t *hal, unsigned int channel)
{
    bool cur;
    relay_status_t s = relay_hal_get(hal, channel, &cur);
    if (s != RELAY_OK)
        return s;
    return relay_hal_set(hal, channel, !cur);
}

relay_status_t relay_hal_set_mask(relay_hal_t *hal, unsigned int mask)
{
    if (!hal)
        return RELAY_ERR_INVALID_ARG;

    relay_status_t result = RELAY_OK;
    for (unsigned int i = 0; i < RELAY_HAL_NUM_CHANNELS; i++) {
        relay_status_t s = relay_hal_set(hal, i, (mask >> i) & 1u);
        if (s != RELAY_OK)
            result = s;   /* keep going so one failure doesn't leave others on */
    }
    return result;
}

relay_status_t relay_hal_all_off(relay_hal_t *hal)
{
    return relay_hal_set_mask(hal, 0);
}

const char *relay_hal_strerror(relay_status_t status)
{
    switch (status) {
    case RELAY_OK:              return "ok";
    case RELAY_ERR_INVALID_ARG: return "invalid argument";
    case RELAY_ERR_NOMEM:       return "out of memory";
    case RELAY_ERR_CHIP:        return "cannot open gpiochip";
    case RELAY_ERR_REQUEST:     return "cannot request line (busy or bad offset?)";
    case RELAY_ERR_IO:          return "gpio I/O error";
    default:                    return "unknown error";
    }
}