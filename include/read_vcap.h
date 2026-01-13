//=============================================================================#
/** 
 * @brief: v- Read Voltage of capacitor using:
 *     + Resolution: 14 bit"
 *     + Pin AIN1
 * @note: Vcap =! Vbatt in this case due to using the boost
*/                                                  #
//=============================================================================#

#ifndef APPICATION_READ_VCAP_H_
#define APPICATION_READ_VCAP_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

// AIN1
// ADC settings
#define ADC_RESOLUTION         14
#define ADC_GAIN               ADC_GAIN_1_6
#define ADC_REFERENCE          ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME   ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID         1

#define ADC_CHANNEL_INPUT SAADC_CH_PSELP_PSELP_AnalogInput1      // AIN1

int8_t Vcap_init(void);
uint16_t read_Vcap_mv(void);

#endif /*APPICATION_READ_VCAP_H_*/