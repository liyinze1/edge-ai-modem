#include "enable_print.h"
#include "read_vcap.h"


LOG_MODULE_REGISTER(read_Vcap);

#define ADC_NODE DT_NODELABEL(adc)
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
static int16_t sample_buffer;


int8_t Vcap_init(void) {
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
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
        // .differential     = 0,
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
