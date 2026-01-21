#include "astar_paras.h"
#include "enable_print.h"
#include "modem.h"
#include "serial_interface.h"
#include "read_vcap.h"
#include "opencircuit.h"


uint16_t newV = 0;
uint16_t oldV = 0;
int16_t deltaV = 0;
uint16_t  solarV = 0;;
const uint16_t maxVoltage = 3000;
const uint16_t shutOffVoltage = 900;
const uint16_t OpenCircuitVoltage = 3000;
uint32_t optimumV = 2800;
uint16_t wakeupThreshold_Vpv = 4000;
uint16_t sleepThreshold_Vpv  = 3800;
uint16_t beginSleeping_Vcap = 0;            // Vcap once the node begins sleeping

// In seconds
uint32_t sleepTimer = 30;
const uint16_t LowVolt_SleepTime = 7200;     
const uint32_t maxRate = 120;
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
uint16_t  reconnection_times          = 0;


LOG_MODULE_REGISTER(AsTAR);

K_SEM_DEFINE(my_semaphore_vcap, 1, 1);      // prevent a lot of threads from reading Vcap at the same time

//------------------------------------------------------------------------------------
/**
 * @brief: Reconnection thread function
 */
//------------------------------------------------------------------------------------
void reconnection_thread(void)
{  
  while (1)
  {
    if (check_network_connection())
    {
      failed_reconnection_times = 0;
      reconnection_interval = MIN_RECONNECTION_INTERVAL;

      // Do not check the connection continously to save energy
      if(nighttimeFlag)
        k_sleep(K_SECONDS(21600));
      else 
        k_sleep(K_SECONDS(3600));                       
    }
    else
    {
      if ENABLE_PRINT
        LOG_INF("Connection is failed - Start reconnection steps ....!");
      if (newV >= 4100)
        {
          reconnect_to_network();
          failed_reconnection_times = failed_reconnection_times + 1;
          if ENABLE_PRINT
            LOG_INF(" The number of consecutive failed reconnection attempts: %d", failed_reconnection_times);
        }

      if (failed_reconnection_times >= 4)
      {
        if (newV >= 4200)
        {
          sys_reboot(0);
        }
      }
      reconnection_interval = MIN_RECONNECTION_INTERVAL;
      if (!(check_network_connection()))
      {
        for (uint8_t i = 1; i < (failed_reconnection_times); i++)          
        {
          reconnection_interval *= 3;
          if (reconnection_interval < MIN_RECONNECTION_INTERVAL) reconnection_interval = MIN_RECONNECTION_INTERVAL;
          if (reconnection_interval > MAX_RECONNECTION_INTERVAL) reconnection_interval = MAX_RECONNECTION_INTERVAL;
          if ENABLE_PRINT
            LOG_INF("Consecutive Re-connection Interval: %d", reconnection_interval);
          k_sleep(K_SECONDS(reconnection_interval));
        }
      }
    }
    if ENABLE_PRINT
        LOG_INF("++++++++++++++++ Escaped Re-connection Thread ++++++++++++++");
  }
}



// ------------------------------------------------------------------------------------
/**
 * @brief:  Thread function for the Capacitors Overvoltage Protection 
 */
// ------------------------------------------------------------------------------------
void overV_protection_thread(void)
{  
  while (1)
  {
    if ENABLE_PRINT
        LOG_INF("++++++++++++++ Entered OverVcap Protection Thread ++++++++++++++");
    if (k_sem_take(&my_semaphore_vcap, K_SECONDS(5))==0)    // Prevent 2 or more threads reading V_cap at the same time
    {  
      if ((!nighttimeFlag) && (oldV>1500))
        { 
          newV = read_Vcap_mv();       // Read V_Caps
          if (newV > OpenCircuitVoltage)
            isolate_solar();              // When V_caps is HIGH, solar pannels are isolated 
          else 
            connect_solar();              // When V_caps is normal, solar pannels are connected to Caps 
        }
      k_sem_give(&my_semaphore_vcap);
    } else {
        if ENABLE_PRINT
            LOG_INF("Thread timed out waiting for my_semaphore_vcap");
      }


    if(nighttimeFlag)
      k_sleep(K_SECONDS(1800));          // In seconds - Not check the connection continously to save energy
    else 
      k_sleep(K_SECONDS(600));           // In seconds - Not check the connection continously to save energy
    
    if ENABLE_PRINT
        LOG_INF("+++++++++++++ Escaped OverVcap Protection Thread ++++++++++++++");
  }
}


// ------------------------------------------------------------------------------------
/**
 * @brief: Scheduler
 */
// ------------------------------------------------------------------------------------

void setSuspensionHandler(void) {  
  sleepTimer = LowVolt_SleepTime;
  // sleepTimer = 20; // For test
  oldV = newV;
  uart_send_cmd_powerdown();
  if ENABLE_PRINT
    LOG_INF("Vcap is very low - power-down command <P> has sent to the RR");
  k_sleep(K_SECONDS(sleepTimer));
}

uint32_t schedule(void) {
  uint32_t sleepDelta;
  deltaV = newV-oldV;
  // When wakes up?
  if (nighttimeFlag) {
    if (solarV >= wakeupThreshold_Vpv)
      timeSinceSunrise +=sleepTimer;
    else timeSinceSunrise = 0;
    if(timeSinceSunrise >= nightVRiseTimeThreshold)
    {  
      if (timeSinceSunset >= 14400) { // Threshold of 4 hours to count towards night estimate
        nightDurationRollingEstimate = (((100 - weightingNewNightLength) * nightDurationRollingEstimate)/100) + ((weightingNewNightLength * timeSinceSunset)/100);
      }
      nighttimeFlag = false;
      optimumV = daytimeOptimumV;
      timeSinceSunset = 0;
      first_wakeup_flag = true;
    }
  } 

  // When sleeps?
  if (solarV < sleepThreshold_Vpv){
    timeSinceSunset += sleepTimer;
    if (timeSinceSunset >= nightVLossTimeThreshold) {
      if (!nighttimeFlag) 
      {
        beginSleeping_Vcap = newV;
        nighttimeVSwing = beginSleeping_Vcap - (1.1*shutOffVoltage);
        first_sleep_flag = true;
      }
      nighttimeFlag = true;
    }
  }

  
  if(nighttimeFlag) {
    // optimumV = daytimeOptimumV - ((nighttimeVSwing * timeSinceSunset)/nightDurationRollingEstimate);
    optimumV = beginSleeping_Vcap - ((nighttimeVSwing * timeSinceSunset)/nightDurationRollingEstimate);
    if (optimumV < (1.1 * shutOffVoltage)) optimumV = (1.1 * shutOffVoltage);
  }
  // ****** End night optimisations ********

  
  // Start *************** Specify Kp ***************
  float kp;
  kp = fabs(newV - optimumV)/50.0;
  if (kp < 1.4)
    kp = 1.4;
  else if (kp > 2)
    kp = 2;
  if (abs(deltaV) >= 250)
    kp = kp + (fabs((double)deltaV/125.0));

  if(newV >= maxVoltage)                                //  State = "Very High";
  {  
    sleepTimer = maxRate;
  } 
  else if ((newV < maxVoltage)&&(newV > optimumV))      //  State = "high";
  {
    if (deltaV > 0) sleepTimer = sleepTimer / kp;
    if (deltaV <= 0) {
      sleepDelta = sleepTimer / 10;
      if (sleepDelta < 1) sleepDelta = 1;               // Account for rounding
      if (nighttimeFlag){
        if ((newV-optimumV) >= 150)
          sleepTimer = sleepTimer/kp;                   // (newV-optimumV) > 150mV)
        else sleepTimer -= sleepDelta;                  // Track decreasing optimumV                            
      } 
      else {
        sleepTimer += sleepDelta;
      }
    }

    if (sleepTimer < maxRate) sleepTimer = maxRate;
    if (nighttimeFlag && (sleepTimer < nighttimeMaxRate)) sleepTimer = nighttimeMaxRate;
    if (sleepTimer > minRate) sleepTimer = minRate;
  }
  else if (newV < optimumV)                             // State = "low";
  {
    if (deltaV > 0) {
      sleepDelta = sleepTimer / 10;
      if (sleepDelta < 1) sleepDelta = 1;               // Account for rounding
      sleepTimer -= sleepDelta;
    }
    if (deltaV <= 0) sleepTimer = sleepTimer * kp; 
    if (sleepTimer < maxRate) sleepTimer = maxRate;
    if (nighttimeFlag && (sleepTimer < nighttimeMaxRate)) sleepTimer = nighttimeMaxRate;
    if (sleepTimer > minRate) sleepTimer = minRate;
  }
  else  {
    // When newV = optimumV => do nothing (New rate = old rate)
   }


  //--------------------------------------------------------------------------------------------------
  // Set constant Sleep_Timer when the node just wakes up and just sleeps
  //--------------------------------------------------------------------------------------------------
   // Can help when the node just wakes up, the "Sleep_timer" is very long
  if (first_wakeup_flag)
  {
    sleepTimer = 450;
    first_wakeup_flag = false;
  }

  // Can help when the node just sleep, the "Sleep_timer" may be very long
  if (first_sleep_flag)
  {
    sleepTimer = 200;
    first_sleep_flag = false;
  }

  if ENABLE_PRINT
    LOG_INF("Finished run the AsTAR scheduler - Sleeptimer = ", sleepTimer);
  return sleepTimer;
}