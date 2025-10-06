/**
 * @brief:  + The nRF is woken up by the RoadRunner (using a GPIO interrupt) after it completed ML inference
 *              so that the nRF can receive from the RR and send UART data to the cloud.
 *          + When the RR pulls the wake-in pin high (rising edge), a GPIO interrupt is fired to wake up the nRF 
*/ 

#include "nrf_wakeup.h"
#include "serial_interface.h"


LOG_MODULE_REGISTER(nRF_wakeup);
static const struct gpio_dt_spec dev_wake_in = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, wakein_gpios);

static struct gpio_callback wakein_cb_data;

// GPIO Interrupt Callback
void static wakein_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    LOG_INF("The nRF received Wake input signal from the RoadRunner and ready to receive the UART message");
    // ToDo: RESUME UART here
    uart_send_ready();          // Send "A" to RR to notify “I am awake and ready to communicate”
}






//===================================================================================================
int8_t nrf_wakeup_init(void) {
    if (!gpio_is_ready_dt(&dev_wake_in)) {
         LOG_ERR("GPIO port %s is not ready!", dev_wake_in.port->name);
	    return 1;
    }

    // Configures the wake-in pin to generate an interrupt on rising edge.
    if (gpio_pin_interrupt_configure_dt(&dev_wake_in, GPIO_INT_EDGE_RISING)) {
        LOG_ERR("Failed to configure interrupt on %s pin %d\n",
			dev_wake_in.port->name, dev_wake_in.pin);
		return 1;
    }

    //--------------------------------------------------------------------------------------------------
    //  gpio_init_callback() fills the wakein_cb_data struct (with PIN + its callback)
    //  Zephyr adds wakein_cb_data (wakeup PIN + callback) to the list of callbacks for this GPIO port
    //--------------------------------------------------------------------------------------------------
    gpio_init_callback(&wakein_cb_data, wakein_cb, BIT(dev_wake_in.pin));
	gpio_add_callback(dev_wake_in.port, &wakein_cb_data);

    LOG_INF("nRF wake-up initialized successfully");
    return 0;
}