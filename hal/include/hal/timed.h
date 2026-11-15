#ifndef TIMED_H
#define TIMED_H

#include "hal/relay.h"
#include "settings.h"
#include "state.h"

// Timed Takes 4 Parameters
// The Settings User Puts (Mode / Frequency / Duration)
// State (Watering / Not Watering)
// Relay
// Channel
int timed_update(const settings_t *settings, state_t *state, relay_hal_t *relay, unsigned int pump_channel);

#endif /* TIMED_H */