//=============================================================================#
//  AsTAR ++                                                                   #
//=============================================================================#

#ifndef APPLICATION_ASTAR_PARAS_H
#define APPLICATION_ASTAR_PASAS_H

// #include <inttypes.h>
// #include <stddef.h>
// #include <stdint.h>
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
// #include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>


// Declare GLOBAL variables
extern uint16_t  newV;
extern uint16_t  solarV;;
extern const uint16_t shutOffVoltage;
extern const uint16_t   maxVoltage;
extern uint16_t  sleepTimer;
extern uint16_t  reconnection_times;
extern struct k_sem my_semaphore_vcap;      // prevent a lot of threads from reading Vcap at the same time

// ---------------------------------------------------------------------------
// Function Definitions
// ---------------------------------------------------------------------------

// Declare Functions used in AsTAR++
void setSuspensionHandler(void);
uint32_t schedule(void);

// Thread definitions
void reconnection_thread(void);
void overV_protection_thread(void);

#endif  /*APPLICATION_ASTAR_PARAS_H*/