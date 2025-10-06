#include"lowpower.h"

void setup_accel(void)
{
	const struct device *sensor = DEVICE_DT_GET(DT_ALIAS(accel0));
	if (!device_is_ready(sensor))
	{
		printk("Could not get accel0 device\n");
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
		printk("Failed to set odr: %d\n", rc);
		return;
	}
}

int setup_gpio(void)
{

	gpio_pin_configure_dt(&sw0, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&led0, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&latch_en, GPIO_DISCONNECTED);
	gpio_pin_configure_dt(&wp, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure_dt(&hold, GPIO_INPUT | GPIO_PULL_UP);
	return 0;
}


int setup_uart()
{
	static const struct device *const console_dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	/* Disable console UART */
	int err = pm_device_action_run(console_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err < 0)
	{
		printk("Unable to suspend console UART. (err: %d)\n", err);
		return err;
	}
	/* Turn off to save power (High Speed clock) */
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;
	return 0;
}

