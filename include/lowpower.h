//=============================================================================#
// Power Management                                                            #
//=============================================================================#

#ifndef APPLICATION_LOWPOWER_H_
#define APPLICATION_LOWPOWER_H_

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>



static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct device *const console_dev2 = DEVICE_DT_GET(DT_NODELABEL(uart2));		// Enable UART2 by using Power Management Subsystem

// Start - ************************************ For Low Power ***********************************
static const struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec latch_en = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), latch_en_gpios);

static const struct gpio_dt_spec wp = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wp_gpios);
static const struct gpio_dt_spec hold = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), hold_gpios);
// Stop - ************************************ For Low Power ************************************

// Function definitions
void setup_accel(void);
int8_t setup_gpio(void);
int8_t setup_uart0_DIS();       // Disable UART console
int8_t setup_uart2_DIS();       // Disable UART2
int8_t setup_uart0_ENA();       // Enable UART console
int8_t setup_uart2_ENA();       // Enable UART2
#endif  /*APPLICATION_LOWPOWER_H_*/