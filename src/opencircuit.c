#include "enable_print.h"
#include "opencircuit.h"

int8_t ret_open;
#define open_circuit DT_ALIAS(opencircuit)
static const struct gpio_dt_spec sw2  = GPIO_DT_SPEC_GET(open_circuit, gpios);
int8_t check_gpio_sw2(void){
    if (!device_is_ready(sw2.port)) {
        printk("GPIO initialization for Digital SW2 is not successful!");
        return (1);
    }
    ret_open = gpio_pin_configure_dt(&sw2, GPIO_OUTPUT_ACTIVE);
    if (ret_open < 0) {
        printk("Pin Configuration for Digital SW2 is not successful!");
        return (1);
    }
    return 0;
}

void connect_solar(void){
    gpio_pin_set_dt(&sw2,1);        // Pin is at FLOAT state => Switch on
}

void isolate_solar(void){
    gpio_pin_set_dt(&sw2,0);        // Pin is at low level => Switch off
}