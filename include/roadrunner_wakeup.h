//=============================================================================#
// Management of Waking nRF up                                                 #
//      @brief: Used to wake the RR by pull its wake-up pin down               # 
//=============================================================================#
#ifndef APPLICATION_ROADRUNNERWAKEUP_H
#define APPLICATION_ROADRUNNERWAKEUP_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>


// FUNCTION DEFINITIONS 
int8_t runner_wakeup_int(void);
void runner_set_wakeup(void);
void runner_reset_wakeup(void);

#endif  /*APPLICATION_ROADRUNNERWAKEUP_H*/