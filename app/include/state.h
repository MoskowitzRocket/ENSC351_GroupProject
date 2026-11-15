#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool watering;
    bool soaking;              // Auto mode: waiting after a burst
    uint32_t lastWaterTime;    // time(NULL) of last watering
    uint32_t waterStartTime;
    uint32_t totalThisCycle;   // for the safety cutoff

    // Newly Added
    uint32_t soakStartTime;    // Auto mode: when the soak wait began
    uint32_t nextCheckTime;    // Auto mode: don't check the soil before this time
} state_t;



#endif /* STATE_H */