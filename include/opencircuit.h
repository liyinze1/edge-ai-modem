//=============================================================================#
// Isolate the solar panel when V > Vmax to protect the system                 #
//=============================================================================#

#ifndef APPICATION_OPEN_CIRCUIT_H
#define APPICATION_OPEN_CIRCUIT_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>                 // For Using "exit" function
#include <zephyr/logging/log.h>


int8_t check_gpio_sw2(void);
// Turrn on the Digital Switch to connect solar panels to charge the Caps
void connect_solar (void);
// Turrn off the Digital Switch to isolate solar panels from the Caps
void isolate_solar (void);

#endif  /*APPICATION_OPEN_CIRCUIT_H*/

