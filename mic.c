#include "mic.h"

#include "periph/adc.h"
#include "driver/adc.h"
#include "xtimer.h"

// ADC Configuration
#define ADC_UNIT          ADC_UNIT_1    // Use ADC1
#define ADC_CHANNEL       ADC_CHANNEL_6 // Example: GPIO6
#define ADC_WIDTH         ADC_WIDTH_BIT_12
#define ADC_ATTEN         ADC_ATTEN_DB_0
#define DMA_BUFFER_SIZE   1024

// static adc_continuous_handle_t adc_handle;

#define RES             ADC_RES_10BIT
#define MIC_ADC_LINE 		ADC_LINE(8)

static void configure_adc_dma(void) {
	// adc_continuous_config_t config = {
	// 	.unit_id = ADC_UNIT,
	// 	.channel_config = {
	// 		.channel = ADC_CHANNEL,
	// 		.atten = ADC_ATTEN,
	// 	},
	// 	.sample_freq_hz = 1000, // Sampling frequency in Hz
	// 	.max_store_buf_size = DMA_BUFFER_SIZE,
	// 	.conv_mode = ADC_CONV_SINGLE_UNIT_1,
	// };

	// ESP_ERROR_CHECK(adc_continuous_new(&config, &adc_handle));

	// Start ADC conversion
	// ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}


void Mic_Init(void)
{
	// /* initialize all available ADC lines */
	// if (adc_init(MIC_ADC_LINE) < 0) {
	// 	printf("Initialization of ADC_LINE(%u) failed\n", MIC_ADC_LINE);
	// 	return 1;
	// } else {
	// 	printf("Successfully initialized ADC_LINE(%u)\n", MIC_ADC_LINE);
	// }
}

void Mic_Test(void)
{
  int sample;
	sample = adc_sample(MIC_ADC_LINE, RES);
	printf("%i\n", sample);
}
