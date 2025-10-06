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
#include "lowpower.h"
#include "roadrunner_wakeup.h"
#include "serial_interface.h"
#include "nrf_wakeup.h"

LOG_MODULE_REGISTER(main);


// Declare variables
int8_t ret;
uint16_t sleeptimer;


uint8_t waterLevel_tx_len;
uint16_t waterLevel_tx_buf[2];
uint16_t waterlevel_tx;


//--------------------------------------- S- UART -------------------------------------
#define STACKSIZE 512
#define THREAD_UARTPROCESS_PRIORITY 5
void thread_uartprocess (void);

K_THREAD_DEFINE(thread_uartprocess_id, STACKSIZE, thread_uartprocess, NULL, NULL, NULL,
	THREAD_UARTPROCESS_PRIORITY, 0, 0);
//--------------------------------------- E- UART -------------------------------------

int main(void)
{
  LOG_INF("Starting...");
  setup_gpio();			
	setup_accel();
	// setup_uart();

  // Init modem - power off modem for saving enegy
	nrf_modem_lib_init();
	lte_lc_init();

  uart_init();
  
	// GPIO init for Waking up RR
  ret = runner_wakeup_int();
	if(ret){
		LOG_INF("Fail to retrieving GPIO for waking up RoadRunner from DTs");
		exit(1);
	}

  // GPIO init for Waking up the nRF
  ret = nrf_wakeup_init();
	if(ret){
		LOG_INF("Fail to retrieving GPIO for waking up nRF from DTs");
		exit(1);
	}

  LOG_INF("Please wait 30 seconds for the RR to boot ...");
  k_sleep(K_MSEC(30000));          // Wait for the RR to finish its first booting
  LOG_INF("Wake up RR to start working");
  runner_set_wakeup();           // Wake RoadRunner up
  
  while (1)
  {
    // ToDo: Resume UART to be able to be waken up by the RR UART interrupt and receive UART
    LOG_INF("Resume UART to be able to be waken up by the RR UART interrupt and receive UART data");
    
    LOG_INF("nRF sleeps until the RR finishes its ML inference and then wake it up by sending UART water level data to it ... \n");
   

    /* ????????????????????????????????? 
    * Whether or not we use the GPIO interrupt to wakeup the nRF - Actually, when we send water_level data to the nRF => UART ISR fired => nRF is waken up automatically 
    *
    * Wait until the RR has finished the ML inference and then Wake the nRF via GPIO interrupt
    *   The nRF then sends 'A' to the RR to notify: 'I am awake and ready to receive UART data.'
    */


    /*
    * The nRF waits until the RR has finished the ML inference and then wake the nRF up 
    *   by sending the water level data via UART to the nRF 
    */
    k_sem_take(&uart_process_rx_done, K_FOREVER);   // MCU automatically falls into sleep during the interval waiting for "k_sem_give(&uart_process_rx_done)" being called
    
    // Todo: retrieve Water level to send to the server
    uint16_t water_level = waterlevel_tx;
    LOG_INF("Sucessfully retrieve water level into main(): %d (cm)", water_level);

    // Todo: AsTAR++ scheduler here
    LOG_INF("Run AsTAR++");
    // Seed the random number generator with the current time
    srand(time(NULL));
    int min = 1799;
    int max = 1801;
    // Generate random number in range [min, max]
    sleeptimer = min + rand() % (max - min + 1);    // Dummy sleep interval
    // sleeptimer = 30;     // Dummy sleep interval


    // Based on the calculated sleep interval => to send sleeping mode command to the RR
      /**
       * if interval < 30m => Suspend-to-RAM Mode
       * if interval >=30m => Powerdown Mode
       */
    LOG_INF("Calcualted Sleep interval: %d (s)", sleeptimer);
    LOG_INF("Based on the calculated sleep interval => to send sleeping-mode command to the RR");
    if (sleeptimer < 1800)
    {
      uart_send_cmd_suspendRAM();
      LOG_INF("suspend-to RAM command <S> has sent to the RR");
    }
    else
    { 
      uart_send_cmd_powerdown();
      LOG_INF(" power-down command <P> has sent to the RR");
    }

    // ToDo: Suspend UART before sleep to save the energy during sleep interval
    LOG_INF("Suspend UART before sleep to save the energy during sleep interval");

    // ToDo: send data to server
    LOG_INF("Send data set to the server");

    // ToDo: Sleep
    LOG_INF("Wait for the nRF sleeping for %d (s)", 20);
    LOG_INF(" ----------------------------------------------------------------------------------------------------");
    k_sleep(K_SECONDS(20));

  }

  return (0);
}








//======================================== S - Function Definitions ==========================================
/**
 * @brief: UART Thread Function
 */
void thread_uartprocess (void) {
    while (1) {
        k_sem_take(&uart_data_ready, K_FOREVER);
        uart_process_rx();
        k_sem_give(&uart_process_rx_done);      // Finished processing UART data
    }
}

