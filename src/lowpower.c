#include "enable_print.h"
#include"lowpower.h"

LOG_MODULE_REGISTER(lowpower);

static int8_t err;

void setup_accel(void)
{
	const struct device *sensor = DEVICE_DT_GET(DT_ALIAS(accel0));
	if (!device_is_ready(sensor))
	{
		LOG_ERR("Could not get accel0 device\n");
		return;
	}
	// Disable the device
	struct sensor_value odr = {
		.val1 = 0,
	};

	int rc = sensor_attr_set(sensor, SENSOR_CHAN_ACCEL_XYZ,
							 SENSOR_ATTR_SAMPLING_FREQUENCY,
							 &odr);
	if (rc != 0)
	{
		LOG_ERR("Failed to set odr: %d\n", rc);
		return;
	}
}

int8_t setup_gpio(void)
{
	gpio_pin_configure_dt(&sw0, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&led0, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&latch_en, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&wp, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure_dt(&hold, GPIO_INPUT | GPIO_PULL_UP);
	return 0;
}


//----------------------------------------------------------------------------------------------------#
// Disable UART2 and UART console																	  #
//----------------------------------------------------------------------------------------------------#
// Disable UART console
int8_t setup_uart0_DIS()
{
	//static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	/* Disable console UART */
	err = pm_device_action_run(console_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0)
	{
		LOG_ERR("Unable to suspend console UART. (err: %d)\n", err);
		return err;
	}

	/* Turn off to save power (High Speed clock) */
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	return 0;
}

// Disable UART2
int8_t setup_uart2_DIS()
{
	//static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	/* Disable console UART */
	err = pm_device_action_run(console_dev2, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0)
	{
		LOG_ERR("Unable to suspend UART 2 (err: %d)\n", err);
		return err;
	}

	return 0;
}


//----------------------------------------------------------------------------------------------------#
// Enable UART2 and UART console																	  #
//----------------------------------------------------------------------------------------------------#
// Enable UART0
int8_t setup_uart0_ENA()
{
	// Enable UART2 by using Power Management Subsystem  
    err = pm_device_action_run(console_dev, PM_DEVICE_ACTION_RESUME);
    if (err < 0)
    {
      LOG_ERR("Unable to RESUME console UART (err: %d)\n", err);
	  return err;
    }
	return 0;
}

// Enable UART2
int8_t setup_uart2_ENA()
{
	// static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	// Enable UART2 by using Power Management Subsystem  
    err = pm_device_action_run(console_dev2, PM_DEVICE_ACTION_RESUME);
    if (err < 0)
    {
      LOG_ERR("Unable to RESUME UART2 (err: %d)\n", err);
	  return err;
    }
  
    // Enable UART2 by using Register Accesses
      NRF_UARTE2_NS->TASKS_STARTTX = 1;
      NRF_UARTE2_NS->TASKS_STARTRX = 1;
      NRF_UARTE2_NS->ENABLE = 8;
	return 0;
}