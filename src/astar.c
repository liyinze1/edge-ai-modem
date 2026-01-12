
#include "astar_paras.h"
#include "enable_print.h"
#include "modem.h"
#include "serial_interface.h"

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
      if (ENABLE_PRINT)
        printk("Connection is failed - Start reconnection steps ....! \n");
      if (newV >= 4100)
        {
          reconnect_to_network();
          failed_reconnection_times = failed_reconnection_times + 1;
          if (ENABLE_PRINT)
            printk(" The number of consecutive failed reconnection attempts: %d \n", failed_reconnection_times);
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
          if (ENABLE_PRINT)
            printk(" Consecutive Re-connection Interval: %d \n", reconnection_interval);
          k_sleep(K_SECONDS(reconnection_interval));
        }
      }
      if (ENABLE_PRINT)
        printk("\n ++++++++++++++++++ Escaped Re-connection Thread ++++++++++++++++++++\n\n\n");
    }
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
        printk("\n\n++++++++++++++++++ Entered OverVoltage Protection Thread ++++++++++++++++++++\n");
    if (k_sem_take(&my_semaphore_vcap, K_SECONDS(5))==0)    // Prevent 2 or more threads reading V_cap at the same time
    {  
      if ((!nighttimeFlag) && (oldV>1500))
        { 
          newV = get_cap_voltage();       // Read V_Caps
          if (newV > OpenCircuitVoltage)
            isolate_solar();              // When V_caps is HIGH, solar pannels are isolated 
          else 
            connect_solar();              // When V_caps is normal, solar pannels are connected to Caps 
        }
      k_sem_give(&my_semaphore_vcap);
    } else {
        if ENABLE_PRINT
            printk("Thread timed out waiting for my_semaphore_vcap.\n");
      }


    if(nighttimeFlag)
      k_sleep(K_SECONDS(600));          // In seconds - Not check the connection continously to save energy
    else 
      k_sleep(K_SECONDS(20));           // In seconds - Not check the connection continously to save energy
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

void Schedule(void) {
  uint32_t sleepDelta;
  deltaV = newV-oldV;
  // When wakes up?
  if (nighttimeFlag) {
    if (solarV >= wakeupThreshold_V)
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
  if (solarV < sleepThreshold_V){
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
   // When the node just wakes up, the "Sleep_timer" is very high => this can help mitagate this
  if (first_wakeup_flag)
  {
    sleepTimer = 450;
    first_wakeup_flag = false;
  }

  // When the node just sleep in early morning, the "Sleep_timer" may be very high => this can help mitagate this
  if (first_sleep_flag)
  {
    sleepTimer = 200;
    first_sleep_flag = false;
  }

  // sleepTimer = 20;     // Test
}