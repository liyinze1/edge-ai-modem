/**
 * @brief: Read Voltage of solar Panels
 *     + Channel AIN0	
 *     + Resolution: 14 bit
*/

#ifndef READ_SOLAR_H_
#define READ_SOLAR_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>


// The number of ADC channels will be used ( = 1 means only AIN0 is ultilized)
#define ADC_NUM_CHANNELS 1			

#define ADC_NODE DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION          14
#define ADC_GAIN                ADC_GAIN_1_6
#define ADC_REFERENCE           ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME    ADC_ACQ_TIME_DEFAULT

// To read average ADC value
uint16_t read_adc(void);

#endif