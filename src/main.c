#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

// For generate random sleep interval
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/sys/reboot.h>

// Declare header files
#include "enable_print.h"
#include "modem.h"
#include "lowpower.h"
#include "roadrunner_wakeup.h"
#include "serial_interface.h"
#include "nrf_wakeup.h"

#include "astar_paras.h"
#include "opencircuit.h"
#include "read_solar.h"


LOG_MODULE_REGISTER(main);

// K_SEM_DEFINE(my_semaphore_vcap, 1, 1);    // prevent a lot of threads from reading Vcap at the same time

// Declare variables
int8_t ret;
int8_t err2;
uint16_t sleeptimer;

uint8_t waterLevel_tx_len;
uint16_t waterLevel_tx_buf[2];
uint16_t waterlevel_tx;

//----------------------------------------------------------------------------------------
// ++++++++++++++++++++++ S- UART THREAD ++++++++++++++++++++++++
//----------------------------------------------------------------------------------------
#define UART_STACKSIZE 512
#define THREAD_UARTPROCESS_PRIORITY 5
// void thread_uartprocess (void);
K_THREAD_DEFINE(thread_uartprocess_id, UART_STACKSIZE, thread_uartprocess, NULL, NULL, NULL,
	THREAD_UARTPROCESS_PRIORITY, 0, 0);
// -------------------------------------- E- UART THREAD ---------------------------------

//----------------------------------------------------------------------------------------
// ++++++++++++++++++++++ S- Re-connection THREAD ++++++++++++++++++++++++
//----------------------------------------------------------------------------------------
#define RECONN_STACK_SIZE  4096
#define RECONN_PRIORITY 5
// void reconnection_thread(void);
K_THREAD_DEFINE(ReConn_id, RECONN_STACK_SIZE, reconnection_thread, NULL, NULL, NULL,
		            RECONN_PRIORITY, 0, 0);
// ---------------------- E- Re-connection THREAD -----------------------------------------

//----------------------------------------------------------------------------------------
//++++++++++++++++++++++ Caps Overvoltage Protection THREAD ++++++++++++++++++++++++
//----------------------------------------------------------------------------------------
#define OVER_V_STACK_SIZE 512
#define OVER_V_PRIORITY 1
// void overV_protection_thread(void);
K_THREAD_DEFINE(over_v_id, OVER_V_STACK_SIZE, overV_protection_thread, NULL, NULL, NULL,
	              OVER_V_PRIORITY, 0, 0);
//++++++++++++++++++++++ E- Caps Overvoltage Protection THREAD +++++++++++++++++++++


int main(void)
{
  if ENABLE_PRINT
    LOG_INF("Starting...");
  
  // Power Management
  setup_gpio();			
	setup_accel();
	// setup_uart();      // Suspend UART2

  // Open circuit
	ret2 = check_gpio_sw2(); 
	if(ret2){
		if ENABLE_PRINT
      LOG_INF("Open Circuit - Fail to retrieving GPIO for Switch 2 from DTs");
		exit(1);
	}

  // Vpv divider
	uint8_t ret3 = check_gpio_div_sw3();
	if(ret3){
		if ENABLE_PRINT
      LOG_INF("Vpv Divider - Fail to retrieving GPIO for Switch 3 from DTs");
		exit(1);
	}
	
  turn_off_div_sw3();	     // Disable Vpv divider to save energy during the connection time

  // Initialize + Configurate modem
	modem_main_init();    
  k_sleep(K_SECONDS(1));

  // Send start up notification (discarded at server)
  modem_transmitData_astar(0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu);

  if ENABLE_PRINT
    LOG_INF("Wake up RR to start working");
  runner_set_wakeup();                  // Wake RoadRunner up using GPIO interrupt
  uart_init();
  
  while (1)
  {
  
  //========================================================================================# 
  // Todo: Wake up RR                                                                       #
  //========================================================================================#
  runner_set_wakeup();

  //==========================================================================================================#
  // Todo: AsTAR - case 1: if Vcap <= shutOffVoltage (110% of V_brownout)                                     #     
  //        + Send UART message "P" to RR so that it enter powerdown mode                                     #
  //        + nRF enters deep sleep mode                                                                      #
  //==========================================================================================================#
  rerun_astar_after_suspension:
    k_sem_take(&my_semaphore_vcap, K_FOREVER);
    newV = read_Vcap_mv();
    k_sem_give(&my_semaphore_vcap);
    if (newV <= shutOffVoltage) { 
      setSuspensionHandler();
      goto rerun_astar_after_suspension;
    }
    
    //==========================================================================================================================#
    // ToDo: Resume nRF UART so that the nRF is able to be waken up by the RR's UART interrupt and receive its UART message     #
    //==========================================================================================================================#
    if ENABLE_PRINT 
      LOG_INF("Resume UART to be able to be waken up by the RR UART interrupt and receive UART data");

    // Disable UART2 by using Power Management Subsystem
    err2 = pm_device_action_run(console_dev2, PM_DEVICE_ACTION_RESUME);
    if (err2 < 0)
    {
      LOG_ERR("Unable to resume console UART. (err: %d)\n", err2);
    }

    setup_uart_ENA();


      


    //==========================================================================================================================#
    // ToDo: nRF automatically enters sleep state during the interval RR infers ML                                              #
    //       The nRF waits until the RR has finished the ML inference and then wake the nRF up                                  #
    //            by sending the waterlevel/photo data via UART to the nRF                                                           #
    //==========================================================================================================================#
    if ENABLE_PRINT
      LOG_INF("nRF sleeps until the RR finishes its ML inference and then wake nRF up by sending UART waterlevel/photo data to it ... \n");
    // To make MCU automatically fall into sleep during the interval waiting for "k_sem_give(&uart_data_ready)" being called
    k_sem_take(&uart_data_ready, K_FOREVER);
    
    
    //=======================================================================================================================#
    // Todo: Once the UART data (waterlevel/photo) is available on nRF "k_sem_take(&uart_data_ready, K_FOREVER)", nRF sends  #
    //        the UART data to the server - executed in "serial_interface.c"                                                 #  
    //=======================================================================================================================#

    
    //==================================================================================================================#
    // Todo: AsTAR++ scheduler (case 2 when Vcap > Vshutoff) => then Send its parameter to the server                   #
    //          only do this after receiving UART data  and send it to the serser                                       #
    //==================================================================================================================#
    //------------------------ S- Connection Attemps -----------------------
    Reconnection_Times = reconnection_numbers();
    if ENABLE_PRINT
    {
      LOG_INFO("\n\nThe total number of re-connections to the eNodeB: %d \n", (Reconnection_Times));
      LOG_INFO("Check Current Network Status: Is_Connected = %d \n", check_network_connection());
    }
    //----------------------- E- Connection Attemps -------------------------

    //-------------------------- S - Read V_solar ---------------------------
    isolate_solar();                  // To measure open-circuit Voltage
		turn_on_div_sw3();                // Enable Vpv divider
    k_sleep(K_MSEC(20));
		solarV = 2*(read_adc());          // Read Vpv
		// if (solarV > 7100) solarV = 7100;
		turn_off_div_sw3();	              // Disable Vpv divider
    if (newV > maxVoltage)
      isolate_solar();
    else 
      connect_solar();

    if ENABLE_PRINT
      LOG_INFO(" - Voltage of Solar Panels - AIN0 = %d mV\n\n", solarV);
    //-------------------------- E - Read V_solar --------------------------
    
    // Run AsTAR Scheduler - case 2 when Vcap > Vshutoff
    if ENABLE_PRINT
      LOG_INF("Run AsTAR++ when Vcap > Vshutoff");
    Schedule();

    // Send AsTAR paras to the server
      // Only do this after receiving UART data and send it to the serser 
    k_sem_take(&uart_process_rx_done);
    modem_transmitData_astar(uint16_t newV, uint16_t sleepTime, 
						uint16_t solarV, uint16_t reconnection_times);
    //======================================= E- AsTAR++ scheduler - Case 2 =====================================

    //==========================================================================================================================#
    // Todo: Specify the RR's sleeping mode                                                                                     #
    //    According to the sleep interval specified by AsTAR, the nRF transmits the corresponding sleep-mode command to the RR. #
    //      + if interval < 30m => Suspend-to-RAM Mode                                                                          #
    //      + if interval >=30m => Powerdown Mode                                                                               #
    //==========================================================================================================================#
    if ENABLE_PRINT
    {
      LOG_INF("Calcualted Sleep interval: %d (s)", sleeptimer);
      LOG_INF("Based on the calculated sleep interval => to send sleeping-mode command to the RR");
    }
    if (sleeptimer < 1800)
    {
      uart_send_cmd_suspendRAM();
      if ENABLE_PRINT
        LOG_INF("suspend-to RAM command <S> has sent to the RR");
    }
    else
    { 
      uart_send_cmd_powerdown();
      if ENABLE_PRINT
        LOG_INF("power-down command <P> has sent to the RR");
    }

    //==============================================================================================#
    // ToDo: Suspend UART before sleep to save the energy during sleep interval                     #
    //==============================================================================================#
    if ENABLE_PRINT
      LOG_INF("Suspend UART before sleep to save the energy during sleep interval");
    
    // Disable UART2 by using Power Management Subsystem
    err2 = pm_device_action_run(console_dev2, PM_DEVICE_ACTION_SUSPEND);
    if (err2 < 0)
    {
      LOG_INFO("Unable to suspend console UART. (err: %d)\n", err2);
    }

    setup_uart_DIS();

    //==============================================================================================#
    // ToDo: Enter deep sleep                                                                       #
    //==============================================================================================#
    if ENABLE_PRINT
      LOG_INF("The nRF sleeping for %d (s)", sleepTimer);
    LOG_INF(" ----------------------------------------------------------------------------------------------------");
    k_sleep(K_SECONDS(sleepTimer));

  }

  return (0);
}


//======================================== S - Function Definitions ==========================================





