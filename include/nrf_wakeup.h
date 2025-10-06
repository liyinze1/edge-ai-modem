
#ifndef APPLICATION_NRFWAKEUP_H
#define APPLICATION_NRFWAKEUP_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <stdlib.h>


// FUNCTION DEFINITIONS 
int8_t nrf_wakeup_init(void);

#endif /*APPLICATION_NRFWAKEUP_H*/