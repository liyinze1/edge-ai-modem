
/**
 * @brief: v- To control a digital switch that enables a voltage divider for reading the solar panel voltage
 * 
*/

#ifndef SWITCH_DIVIDER_H
#define SWITCH_DIVIDER_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>

// FUNCTION DEFINITIONS 
int8_t check_gpio_div_sw3(void);
// To turrn on the Digital Switch
void turn_on_div_sw3(void);
// To turrn off the Digital Switch
void turn_off_div_sw3(void);

#endif

