/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "roadrunner_wakeup.h"
int8_t ret_SW;

#define D_OP1 DT_ALIAS(op1)

static const struct gpio_dt_spec dev_op1  = GPIO_DT_SPEC_GET(D_OP1, gpios);

int8_t runner_wakeup_int(void){
    if (!device_is_ready(dev_op1.port)) {
        printk("GPIO initialization for waking up RoadRunner is not successful!");
	    return (1);
    }
    ret_SW = gpio_pin_configure_dt(&dev_op1, GPIO_OUTPUT_ACTIVE);
    if (ret_SW < 0) {
	    printk("Pin Configuration for waking up RoadRunner is not successful!");
        return (1);
    }
    return 0;
}

void runner_reset_wakeup(void){
    gpio_pin_set_dt(&dev_op1,1);
}


/**
 * @brief: Drive the RoadRunner wakeup pin LOW to wake it up.
 */
void runner_set_wakeup(void){
    gpio_pin_set_dt(&dev_op1,0);
    k_sleep(K_MSEC(500));
    gpio_pin_set_dt(&dev_op1,1);
}