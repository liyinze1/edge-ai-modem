//=============================================================================#
//  AsTAR ++                                                                   #
//=============================================================================#

#ifndef APPLICATION_ASTAR_PARAS_H
#define APPLICATION_ASTAR_PASAS_H

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

#define ENABLE_PRINT 0
K_SEM_DEFINE(my_semaphore_vcap, 1, 1);      // prevent a lot of threads from reading Vcap at the same time


int8_t ret2;

uint16_t oldV = 0;
uint16_t newV = 0;
int16_t deltaV = 0;                          // newV - oldV
const uint16_t shutOffVoltage = 900;
const uint16_t maxVoltage = 3000;
const uint16_t OpenCircuitVoltage = 3000;
uint32_t optimumV = 2800;
uint16_t wakeupThreshold_V = 4000;          // solarV
uint16_t sleepThreshold_V  = 3800;          // solarV
uint16_t solarV = 0;
uint16_t beginSleeping_Vcap = 0;            // Vcap once the node begins sleeping

// in seconds
uint32_t sleepTimer = 30;
const uint16_t LowVolt_SleepTime = 7200;     
uint32_t maxRate = 120;
const uint32_t minRate = 7200;

// Variables are only dedicated to NightOptimisation algorithms
const uint32_t nighttimeMaxRate = maxRate;
const uint16_t daytimeOptimumV = 2800;
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

// Reconnect to the cellular network
#define   MIN_RECONNECTION_INTERVAL   1200              // In seconds - Min. interval for re-establishing connection
#define   MAX_RECONNECTION_INTERVAL   43200             // In seconds - Max. interval for re-establishing connection
uint32_t  reconnection_interval       = 0;              // In seconds - The time interval between two consecutive reconnections
uint8_t   failed_reconnection_times   = 0;              // The number of consecutive failed reconnections
uint16_t  Reconnection_Times          = 0;


// ---------------------------------------------------------------------------
// Function Definitions
// ---------------------------------------------------------------------------
// Declare Functions used in AsTAR++
void setSuspensionHandler();
void Schedule(void);

// Thread definitions
void reconnection_thread(void);
void overV_protection_thread(void);

#endif  /*APPLICATION_ASTAR_PARAS_H*/