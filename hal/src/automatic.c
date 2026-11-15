#include "automatic.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define SENSOR_MIN    2300   // Broken If Outside Min (When Tested, SUPER WET Was ~2500)
#define SENSOR_MAX    3800   // Broken If Outside Max (When Tested DRY Was ~ 3600)
#define CHECK_SEC     60     // How Often Check Soil                
#define PAUSE_SEC     3600   // Pause After Problem Or Max Water
#define MAX_PUMP_SEC  30     // Max Duration



// Average Reading 10 Captures
static int read_moisture(cap_moisture_hal_t *sensor)
{
    int total = 0;

    for (int i = 0; i < 10; i++) {
        int value = cap_moisture_hal_read(sensor);

        if (value < 0)
            return -1;

        total += value;

        // 20 ms
        usleep(20000);                    
    }

    return total / 10;
}


// Is Reading Outside Range?
static bool sensor_bad(int reading)
{
    return reading < SENSOR_MIN || reading > SENSOR_MAX;
}



// Turning Pump On / Off
static bool pump_set(relay_hal_t *relay, unsigned int channel, bool on)
{
    // Pointer To Relay, Which Channel, On Or Off
    relay_status_t rs = relay_hal_set(relay, channel, on);

    // Check If Worked
    if (rs == RELAY_OK)
        return true;

    // If Failed
    fprintf(stderr, "Pump %s failed: %s\n", on ? "start" : "shutdown",
            relay_hal_strerror(rs));

    return false;
}




// Pump One Burst
// Reads State, Relay, Channel, Time In Seconds
static int start_burst(state_t *st, relay_hal_t *relay, unsigned int channel, uint32_t now)
{
    // If It Didin't Work, Return -1
    if (!pump_set(relay, channel, true))
        return -1;

    // Setting States, Variables
    st->watering       = true;
    st->waterStartTime = now;

    return 0;
}



// If Issue Occured, Wait This Long
// State, Time, Did Cycle End Yet?
static void finish_cycle(state_t *st, uint32_t now, bool problem)
{

    // Reset Total Watering Time
    st->totalThisCycle = 0;

    // Not Soaking Phase/State
    st->soaking        = false;

    // Next Check Is Time Right Now Plus, Pause Sec Or Check Sec
    // Pause Sec == Issue Occured, Wait Longer
    // Check_Sec == No Issue, Usual Normal Wait Timer
    st->nextCheckTime  = now + (problem ? PAUSE_SEC : CHECK_SEC);
}



int automatic_update(const settings_t *s, state_t *st, cap_moisture_hal_t *sensor, 
                     relay_hal_t *relay,
                     unsigned int pump_channel, int *moisture)
{
    // Current Time
    const uint32_t now = (uint32_t)time(NULL);
    int m;

    // If Mode Is Not Automatic, Leave!
    if (s->mode != MODE_AUTO)
        return 0;


    // Checking If Settings Are Okay
    // If Burst < 1, 
    // Delay < 1, 
    // Maximum Water < Burst Sec,
    // Maximum Water > Max Pump Sec

    if (s->burstSec < 1 || s->soakDelaySec < 1 ||
        s->maxWaterSec < s->burstSec || s->maxWaterSec > MAX_PUMP_SEC) {

        // Check If Watering State
        if (st->watering) {

            // If Cannot Set To False, Return -1
            if (!pump_set(relay, pump_channel, false))
                return -1;

            // Set To False
            st->watering = false;
        }

        // Set False
        st->soaking = false;

        return 0;
    }



    // 1.
    // Pump Running, After 1 Burst, Turn Off Let Soak
    if (st->watering) {

        // Check How Long Pump Running
        // Current Time - Start Time < Intended Duration? 
        if (now - st->waterStartTime < s->burstSec)
            return 0;

        // Outside Above If Statement, Means Run Time Exceeded
        // Turn Off, If Issue Return -1
        if (!pump_set(relay, pump_channel, false))

            return -1;

        // Setting Variables
        st->watering        = false;
    
        // Update Total Water Time
        st->totalThisCycle += s->burstSec;
        st->soaking         = true;
        st->soakStartTime   = now;

        // Optional
        printf("[auto] pump off, soaking\n");

        return 0;
    }



    // Soaking, Wait And Check
    if (st->soaking) {

        // Current Time - Start Time < Intended Delay Duration?
        if (now - st->soakStartTime < s->soakDelaySec)
            return 0;

        // Read Mositure Level
        m = read_moisture(sensor);

        // *moisture Is Pointer
        // if (moisture) Checks If Pointer Is Not NULL
        // Yes? == *mosisture = m
        if (moisture) *moisture = m;

        // Outside Range?
        if (sensor_bad(m)) {
            printf("[auto] sensor problem, ending cycle\n");
            finish_cycle(st, now, true);

        // Mositure Level? Wet Enough?
        } else if (m <= s->moistureThreshold) {
            printf("[auto] soil is wet enough (%d)\n", m);
            finish_cycle(st, now, false);

        // Exceed Total Time? 
        // Soil Still Dry, But Check:
        // Total Watered This Cycle, Burst Duration, Max Water Allowed This Cycle
        // Check If Goes Over Max Allowed
        } else if (st->totalThisCycle + s->burstSec <= s->maxWaterSec) {
            printf("[auto] still dry (%d), another burst\n", m);
            st->soaking = false;
            return start_burst(st, relay, pump_channel, now);

        } else {
            printf("[auto] max water reached, pausing\n");
            finish_cycle(st, now, true);
        }
        return 0;
    }

    // Idle

    // Current Time < Check Time
    if (now < st->nextCheckTime)

        return 0;

    // Read Moisture
    m = read_moisture(sensor);

    // Check
    if (moisture) *moisture = m;

    // Schedule Next Check
    st->nextCheckTime = now + CHECK_SEC;

    // Bad Sensor
    if (sensor_bad(m)) {
        printf("[auto] sensor problem, not pumping\n");

    // Moisture Level
    } else if (m > s->moistureThreshold) {
        printf("[auto] soil is dry (%d), watering\n", m);
        st->totalThisCycle = 0;
        return start_burst(st, relay, pump_channel, now);
    }

    return 0;
}