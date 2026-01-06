#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

int8_t ret2;
// Declare Functions used in AsTAR++
void setSuspensionHandler();
void Schedule(void);


uint16_t oldV = 0;
uint16_t newV = 0;
int16_t deltaV = 0;                          // newV - oldV in mV
const uint16_t shutOffVoltage = 900;
const uint16_t maxVoltage = 3000;
// const uint16_t OpenCircuitVoltage = 3000;
uint32_t optimumV = 2900;
uint16_t wakeupThreshold_V = 4000;
uint16_t sleepThreshold_V  = 3800;
uint16_t solarV = 0;
uint16_t beginSleeping_Vcap = 0;              // Vcap when the node begins sleeping

// in seconds
uint32_t sleepTimer = 30;
const uint16_t LowVolt_SleepTime = 7200;     
uint32_t maxRate = 120;
const uint32_t minRate = 7200;

// Variables are only dedicated to NightOptimisation algorithms
const uint32_t nighttimeMaxRate = maxRate;
const uint16_t daytimeOptimumV = 2900;
uint16_t nighttimeVSwing = 0;
bool nighttimeFlag = false;
uint32_t nightDurationRollingEstimate =  48000;         // In seconds
uint32_t timeSinceSunset  = 0;
uint32_t timeSinceSunrise = 0;
const uint16_t nightVLossTimeThreshold = 300;           // In seconds
const uint16_t nightVRiseTimeThreshold = 300;           // In seconds
const uint8_t weightingNewNightLength = 30;             // Percentage weighting for newly recorded night length to total night length

// To set "sleep timer" When the node just wakes up/sleep
bool first_wakeup_flag = false;                         
bool first_sleep_flag = false;