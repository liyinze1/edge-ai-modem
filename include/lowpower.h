/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APPLICATION_LOWPOWER_H_
#define APPLICATION_LOWPOWER_H_

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>


// Start - ************************************ For Low Power ************************************
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec latch_en = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), latch_en_gpios);

static const struct gpio_dt_spec wp = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wp_gpios);
static const struct gpio_dt_spec hold = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), hold_gpios);
// Stop - ************************************ For Low Power ************************************

// Function definition
void setup_accel(void);
int setup_gpio(void);
int setup_uart();

#endif