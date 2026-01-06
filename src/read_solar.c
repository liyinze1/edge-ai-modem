#include"read_solar.h"

int32_t sample_buffer[ADC_NUM_CHANNELS];   			// Create buffer including n members to storage ADC_channels' raw value
int32_t avg_reading[ADC_NUM_CHANNELS];            	// To hold average value of consecutive n-time readings


#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
 	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
 	#error "No suitable devicetree overlay specified"
 #endif


 /* Get the numbers of up to two channels */
static uint8_t channel_ids[ADC_NUM_CHANNELS] = {
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 0),
#if ADC_NUM_CHANNELS == 2
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 1)
#endif
};

// V- Used to configure each specific channel's parameters 
struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	.channel_id = 0,
	.differential = 0
	// .input_positive 			//- "Input pin" will be updated later below
};

/**
 * @brief: V- This struct is ONLY used to specify a "buffer" where an ADC readings is saved  				 
 */
	struct adc_sequence sequence = {
	.channels    = 0,
	.buffer      = sample_buffer,
	.buffer_size = sizeof(sample_buffer),		/* buffer size in bytes, not number of samples */
	.resolution  = ADC_RESOLUTION,
};


// Integration
uint16_t read_adc(void)
{
	int err;
	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);

	if (!device_is_ready(dev_adc)) {
		printk("ADC device not found\n");
		return 1;
	}
	sequence.channels = 0;
	for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
		channel_cfg.channel_id = channel_ids[i];
		#ifdef CONFIG_ADC_NRFX_SAADC
			channel_cfg.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + channel_ids[i];    // Specify the input pin for ADC channel
		#endif
		adc_channel_setup(dev_adc, &channel_cfg);
		k_sleep(K_MSEC(5));
		sequence.channels |= BIT(channel_ids[i]);
	}												  	
														  
	int32_t adc_vref = adc_ref_internal(dev_adc);           // Read ADC internal Vref
	k_sleep(K_MSEC(5));

    printk("Start reading V_Solar_Pannels:\n");
	memset(avg_reading, 0, sizeof(avg_reading));			// Reset the buffer for the next reading					
    for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
		avg_reading[i] = 0;
        for (uint8_t m = 0; m < 5; m++)
		{	
			// Read ADC channels based on the sequence setup above (fails if not supported by MCU)
			memset(sample_buffer, 0, sizeof(sample_buffer));  	// Reset buffer
			err = adc_read(dev_adc, &sequence);   				// "&sequence": The sequence of READING and SAVING ADC raw values in the buffer
			k_sleep(K_MSEC(10));				  								
			if (err != 0) {
				printk("ADC reading failed with error %d.\n", err);
				return 1;
			}
			// Convert raw reading to millivolts
			int32_t raw_value = sample_buffer[i];
			if (((raw_value < 0) || (raw_value > 30000)))
				raw_value = 0;
        	// printk("Channel %d: ", i);
        	printk(" Raw %d: %d", i, raw_value);
        	if (adc_vref > 0) {
				// Convert raw reading to millivolts if driver supports reading of ADC reference voltage
				int32_t mv_value = raw_value;
				adc_raw_to_millivolts(adc_vref, ADC_GAIN, ADC_RESOLUTION, &mv_value);
				k_sleep(K_MSEC(10));
				printk("     ~ Vpv[%d] = %d (mV) \n", m, mv_value);
				avg_reading[i] += mv_value;
			}
        }
    }

	// Get average  of consecutive 5-time readings
	avg_reading[0] = avg_reading[0]/5;
	// avg_reading[1] = avg_reading[1]/10;                  // Average Voltage of 2nd Channel - AIN1
	// printk(" => V_AIN0 = %d mV  \n", avg_reading[0]);
	// printk(" => V_AIN1 = %d mV  \n", avg_reading[1]);
	return (uint16_t)avg_reading[0];
}