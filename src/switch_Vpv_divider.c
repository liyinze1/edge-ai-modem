#include "switch_Vpv_divider.h"

int8_t ret_SW3;
#define SW_DIV DT_ALIAS(swdivider)
static const struct gpio_dt_spec sw_div  = GPIO_DT_SPEC_GET(SW_DIV, gpios);
int8_t check_gpio_div_sw3(void){
    if (!device_is_ready(sw_div.port)) {
        printk("GPIO initialization for Digital SW is not successful!");
        return (1);
    }
    ret_SW3 = gpio_pin_configure_dt(&sw_div, GPIO_OUTPUT_ACTIVE);
    if (ret_SW3 < 0) {
        printk("Pin Configuration for Digital SW is not successful!");
        return (1);
    }
    return 0;
}

// Turrn on the Digital Switch to enable the divider
void turn_on_div_sw3(void){
    gpio_pin_set_dt(&sw_div,1);
}

// Turrn off the Digital Switch to disable the divider
void turn_off_div_sw3(void){
    gpio_pin_set_dt(&sw_div,0);
}