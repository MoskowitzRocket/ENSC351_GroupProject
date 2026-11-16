#include "timed.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>


// Maximum Run Length ( 30 Seconds ) 
#define MAX_PUMP_SEC  30    

// Turning Pump On / Off
static bool pump_set(relay_hal_t *relay, unsigned int channel, bool on)
{

    // Relay_Status_T = enum
    // rs = Variable Holding Result Of Relay Operation
    // Setting rs = Relay Function
    relay_status_t rs = relay_hal_set(relay, channel, on);

    // Checking If Successful
    if (rs == RELAY_OK)
        return true;

    // Somethings Gone Wrong 
    // Failed At Start Or Shutdown? What Error? 
    // relay_hal_strerror Shows Us What Error, Shown On Second "%s"
    fprintf(stderr, "Pump %s failed: %s\n", on ? "start" : "shutdown", relay_hal_strerror(rs));
    return false;
}


// Control Pump With Timer
// Settings = Watering Settings
// State = Watering State
// Relay = Relay Hardware Interface
int timed_update(const settings_t *s, state_t *st, relay_hal_t *relay, unsigned int pump_channel)
{
    // Getting Time
    const uint32_t now = (uint32_t)time(NULL);

    // Is Timed Mode Enabled?
    // s Is Pointer To Setting Struct
    // ->mode Access What Mode We Are In
    // Checking, Are We In Timed Mode?
    if (s->mode != MODE_TIMED)
        return 0;

    // Calculating How Many Seconds Between Watering Intervals
    // Take What User Inputs In LED Thing, Convert To Seconds
    const uint32_t interval_s = settings_interval_ms(s) / 1000U;

    // How Long Should Pump Run?
    // If Longer Than Maximum Duration, Use MAX_PUMP_SEC, Defined Above Otherwise Is Fine
    // Look Into Settings With "s" Pointer, -> durationSec

    const uint32_t run_s = s->durationSec > MAX_PUMP_SEC ? MAX_PUMP_SEC : s->durationSec;

    /* Bad settings? Do nothing (and make sure the pump is off).
     * The pump also needs a rest between runs. */

     // Checking If Valid To Run
     // Is Freq < 1?
     // Pump Duration < 1? 
     // Interval Session < Running Time? 

     // Ex.) Water Every 10 Secs But Run Pump For 10 Secs
    if (s->freqValue < 1 || run_s < 1 || interval_s <= run_s) {

        // Is Pump Currently Watering?
        if (st->watering) {

            // If So, Need To Turn Off
            // If Turning Off Pump Fails, Return -1
            if (!pump_set(relay, pump_channel, false))
                return -1;

            // Set To Stop Water
            st->watering = false;
        }
        return 0;
    }



    // What If Clock Backwards? 
    // Now Is Current Time
    // st -> lastWaterTime = Last Water Time
    if (now < st->lastWaterTime)

    // Reset Last Water Time To Now
        st->lastWaterTime = now;

    // If Off
    if (!st->watering) {

        // Check If Current Time - Last Water Is Less Than Interval 
        if (now - st->lastWaterTime < interval_s)
            return 0;

        // If Cant Turn On, Return -1
        if (!pump_set(relay, pump_channel, true))
            return -1;

        // Set To True
        // Update Time
        st->watering       = true;
        st->waterStartTime = now;

        // Optinal Print
        // printf("[timed] watering started\n");
        return 0;
    }



    // If Pump Is Already On
    // Is Elapsed Water Time Less Than The Set Duration Run Time?
    if (now - st->waterStartTime < run_s)
        return 0;

    // If Fail Turn Off
    if (!pump_set(relay, pump_channel, false))
        return -1;

    // Setting Variables
    st->watering       = false;
    st->lastWaterTime  = now;
    st->waterStartTime = 0;
    st->totalThisCycle = 0;

    // Optional
    // printf("[timed] watering complete; timer reset\n");
    return 0;
}