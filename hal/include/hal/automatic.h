#ifndef AUTOMATIC_H
#define AUTOMATIC_H

#include "hal/cap_moisture.h"
#include "hal/relay.h"
#include "settings.h"
#include "state.h"

// Goes From Idle -> Soil Dry -> Watering -> Soaking -> Checks If Dry 
// Yes? -> Watering
// No? -> Idle

// Call When settings->mode == MODE_AUTO
int automatic_update(const settings_t *settings, state_t *state, cap_moisture_hal_t *sensor, 
                    relay_hal_t *relay, 
                    unsigned int pump_channel, int *moisture);

#endif /* AUTOMATIC_H */