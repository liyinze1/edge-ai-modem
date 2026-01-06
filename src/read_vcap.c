#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "read_vcap.h"

// AIN1
// ADC settings
#define ADC_RESOLUTION         14
#define ADC_GAIN               ADC_GAIN_1_6
#define ADC_REFERENCE          ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME   ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID         1
#define ADC_CHANNEL_INPUT SAADC_CH_PSELP_PSELP_AnalogInput1      // AIN1
LOG_MODULE_REGISTER(read_solar, LOG_LEVEL_DBG);

#define ADC_NODE DT_NODELABEL(adc)
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
static int16_t sample_buffer;


int8_t Vcap_init(void) {
    if (!device_is_ready(adc_dev)) {
        printk("ADC device not ready\n");
        return -1;
    }
    return 0;
}

uint16_t read_Vcap_mv(void)
{
    struct adc_channel_cfg channel_cfg = {
        .gain             = ADC_GAIN,
        .reference        = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id       = ADC_CHANNEL_ID,
        .input_positive   = ADC_CHANNEL_INPUT,
    };

    if (adc_channel_setup(adc_dev, &channel_cfg)) {
        LOG_ERR("ADC channel setup failed");
        return -1;
    }

    const struct adc_sequence sequence = {
        .channels    = BIT(ADC_CHANNEL_ID),
        .buffer      = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
        .resolution  = ADC_RESOLUTION,
    };

    k_sleep(K_MSEC(10));

    int32_t avg_mv = 0;
    for (uint8_t i = 0; i < 5; i++)
    {
        sample_buffer = 0;
        if (adc_read(adc_dev, &sequence)) {
            LOG_ERR("ADC read failed");
            return -1;
        }

        int16_t raw = sample_buffer;
        if ((raw < 0) || (raw>30000)) raw = 0;
        int32_t mv = raw;
        adc_raw_to_millivolts(adc_ref_internal(adc_dev), ADC_GAIN, ADC_RESOLUTION, &mv);
        avg_mv += mv;
    }
    avg_mv = avg_mv/5;
    return (uint16_t)avg_mv;
}
