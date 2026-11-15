#include "settings.h"

void settings_set_defaults(settings_t *settings)
{
    settings->mode = MODE_AUTO;
    settings->freqUnit = UNIT_HOUR;
    settings->freqValue = 2;
    settings->durationSec = 10;
    settings->moistureThreshold = 3000;
    settings->burstSec = 2;
    settings->soakDelaySec = 10;
    settings->maxWaterSec = 30;
}

uint32_t settings_interval_ms(const settings_t *settings)
{
    uint32_t unit_ms = 0;

    switch (settings->freqUnit) {
    case UNIT_MIN:  unit_ms = 60U * 1000U;             break;
    case UNIT_HOUR: unit_ms = 60U * 60U * 1000U;       break;
    case UNIT_DAY:  unit_ms = 24U * 60U * 60U * 1000U; break;
    }
    return (uint32_t)settings->freqValue * unit_ms;
}