#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

enum Mode { MODE_AUTO, MODE_TIMED };
enum Unit { UNIT_MIN, UNIT_HOUR, UNIT_DAY };

typedef struct {
    enum Mode mode;

    // Timed mode
    enum Unit freqUnit;
    uint16_t freqValue;       // e.g. every 2 hours
    uint16_t durationSec;     // how long to water

    // Auto mode
    uint16_t moistureThreshold;
    uint16_t burstSec;
    uint16_t soakDelaySec;
    uint16_t maxWaterSec;     // safety cutoff per cycle
} settings_t;

void settings_set_defaults(settings_t *settings);

// Timed-mode interval (freqValue * freqUnit) in milliseconds.
uint32_t settings_interval_ms(const settings_t *settings);

#endif /* SETTINGS_H */