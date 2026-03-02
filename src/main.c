#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/reboot.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

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
#include "switch_Vpv_divider.h"
#include "read_vcap.h"


LOG_MODULE_REGISTER(main);

uint8_t waterLevel_tx_len;
uint16_t waterLevel_tx_buf[2];
uint16_t waterlevel_tx;

int8_t ret;
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
#define OVER_V_PRIORITY 4
// void overV_protection_thread(void);
K_THREAD_DEFINE(over_v_id, OVER_V_STACK_SIZE, overV_protection_thread, NULL, NULL, NULL,
	              OVER_V_PRIORITY, 0, 0);
//++++++++++++++++++++++ E- Caps Overvoltage Protection THREAD +++++++++++++++++++++


int main(void)
{
  if ENABLE_PRINT
    LOG_INF("Entering main function .....");
  
  // Power Management
  setup_gpio();			
	setup_accel();
	// setup_uart();      // Suspend UART2


  // Wake RoadRunner up
  ret = runner_wakeup_int();
  if(ret){
		if ENABLE_PRINT
      LOG_INF("RR wakeup - Fail to retrieving GPIO waking RoadRunner up");
		exit(1);
	}


  // Open circuit
	ret = check_gpio_sw2();
	if(ret){
      LOG_ERR("Open Circuit - Fail to retrieving GPIO controlling the digital switch from DTs");
		exit(1);
	}

  // Vpv divider
	ret = check_gpio_div_sw3();
	if(ret){
      LOG_ERR("Vpv Divider - Fail to retrieving GPIO for controlling the digital switch from DTs");
		exit(1);
	}
  turn_off_div_sw3();	     // Disable Vpv divider to save energy during the connection time

  
  // Initialize + Configurate modem
	// modem_main_init();
  // k_sleep(K_SECONDS(1));

  // Send start up notification (discarded at server)
  // modem_transmitData_astar(0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu);


  // To make sure that the RoadRunner UART is initialized before the nRF's
  runner_set_wakeup();
  k_sleep(K_SECONDS(5));
  uart_init();


  while (1)
  {

  //========================================================================================# 
  // Todo: Wake up RR                                                                       #
  //========================================================================================#
  if ENABLE_PRINT
    LOG_INF("Wake up RR to start working");
  runner_set_wakeup();

  //==========================================================================================================#
  // Todo: AsTAR - case 1: if Vcap <= shutOffVoltage (110% of V_brownout)                                     #     
  //        + Send UART message "P" to RR so that it enter powerdown mode                                     #
  //        + nRF enters deep sleep mode                                                                      #
  //==========================================================================================================#
  rerun_astar_after_suspension:
    k_sem_take(&my_semaphore_vcap, K_FOREVER);
    newV = read_Vcap_mv();
    if ENABLE_PRINT
      LOG_INF("The supercapacitor Voltage - Vcap = %d mV", newV);
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
    setup_uart0_ENA();
    setup_uart2_ENA();

    
    //==================================================================================================================#
    // Todo: Execute AsTAR++ scheduler (case 2 when Vcap > Vshutoff)                                                    #
    //==================================================================================================================#
    //------------------------ S- Connection Attemps -----------------------
    reconnection_times = reconnection_numbers();
    if ENABLE_PRINT
    {
      LOG_INF("The total number of re-connections to the eNodeB: %d", (reconnection_times));
      LOG_INF("Check Current Network Status: Is_Connected = %d", check_network_connection());
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
      LOG_INF("Voltage of Solar Panels - Vpv = %d mV", solarV);
    //-------------------------- E - Read V_solar --------------------------
    
    // Run AsTAR Scheduler - case 2 when Vcap > Vshutoff
    if ENABLE_PRINT
      LOG_INF("Run AsTAR scheduler when Vcap > Vshutoff");
    sleepTimer = schedule();


    // =======================================================================================================================#
    // Todo: Once the UART data (waterlevel/photo) is sent to nRF - "k_sem_give(&uart_data_ready, K_FOREVER)", nRF is         # 
    //          waken up and sends the UART data to the server - executed in "serial_interface.c"                             #  
    // =======================================================================================================================#

    //============================================================================================================================================#
    // ToDo: nRF automatically enters sleep state during the interval RR infers ML                                                                #
    //       + The nRF waits and sleeps until RR has finished the ML inference by using k_sem_take(&uart_process_rx_done, K_FOREVER);             #
    //           and then nRF is waken up by RR when RR send the UART waterlevel/photo data to after RR finishs its ML execution                  #    
    //============================================================================================================================================#
    if ENABLE_PRINT
      LOG_INF("nRF sleeps until the RR finishes its ML inference, and then RR wakes nRF up by sending UART waterlevel/photo data to it ...");
    k_sem_take(&uart_process_rx_done, K_FOREVER);   // Help nRF only send AsTAR paras to the server after receiving, processing and sending UART data to the server



    // Comment out for testing without Cellular network, please uncomment it when deploying the BEAVER
    // modem_transmitData_astar(newV, sleepTimer, solarV, reconnection_times);



    //==========================================================================================================================#
    // Todo: Specify RR's sleeping mode                                                                                         #
    //    According to the sleep interval specified by AsTAR, the nRF transmits the corresponding sleep-mode command to the RR. #
    //      + if interval < 30m => Suspend-to-RAM Mode                                                                          #
    //      + if interval >=30m => Powerdown Mode                                                                               #
    //==========================================================================================================================#
    if ENABLE_PRINT
    {
      LOG_INF("Calcualted Sleep interval: %d (s)", sleepTimer);
      LOG_INF("Based on the calculated sleep interval => to send sleeping-mode command to the RR");
    }
    // if (sleepTimer < 839)
    // {
    //   uart_send_cmd_suspendRAM();
    //   if ENABLE_PRINT
    //     LOG_INF("suspend-to RAM command <S> has sent to the RR");
    // }
    // else
    // { 
    //   uart_send_cmd_powerdown();
    //   if ENABLE_PRINT
    //     LOG_INF("power-down command <P> has sent to the RR");
    // }
    uart_send_sleep_timer(sleepTimer);     // Send the sleep timer to RR, and RR will decide its sleeping mode based on the sleep timer value

    //==============================================================================================#
    // ToDo: Suspend UART before sleep to save the energy during sleep interval                     #
    //==============================================================================================#
    if ENABLE_PRINT
      LOG_INF("Suspend UART before sleep to save the energy during sleep interval");
    setup_uart2_DIS();    // Disable UART console
    setup_uart0_DIS();    // Disable UART console

    //==============================================================================================#
    // ToDo: Enter deep sleep                                                                       #
    //==============================================================================================#
    
    
    
    
    // Just for testing, please comment it out when deploying the BEAVER
    sleepTimer = 60;





    if ENABLE_PRINT
    {
      LOG_INF("The nRF is sleeping for %d (s)", sleepTimer);
      LOG_INF(" ----------------------------------------------------------------------------------------------------");
    }

    k_sleep(K_SECONDS(sleepTimer));
  }

  return (0);
}