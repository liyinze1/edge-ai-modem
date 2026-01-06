
#include "astar_paras.h"

// ------------------------------------------------------------------------------------
/**
 * @brief: Reconnection thread function
 */
// ------------------------------------------------------------------------------------
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
    if (k_sem_take(&my_semaphore_vcap, K_SECONDS(5))==0)    // Prevent "Re-connection thread" from reading V_cap when "Main thread" thread is reading V_cap
    {  
      if ((!nighttimeFlag) && (oldV>3700))
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
      k_sleep(K_SECONDS(600));                      // In seconds - Do not check the connection continously to save energy
    else 
      k_sleep(K_SECONDS(20));                       // In seconds - Do not check the connection continously to save energy

  }
}