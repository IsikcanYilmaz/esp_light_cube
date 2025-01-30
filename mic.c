#include "mic.h"

#include "periph/adc.h"
#include "driver/adc.h" // hmm this makes things a bit weird
#include "xtimer.h"
#include "logger.h"
#include "sdkconfig.h"
#include "esp_private/gdma.h"

// ADC Configuration
#define ADC_UNIT          ADC_UNIT_1    // Use ADC1
#define ADC_CHANNEL       ADC_CHANNEL_6 // Example: GPIO6
#define ADC_WIDTH         ADC_WIDTH_BIT_12
#define ADC_ATTEN         ADC_ATTEN_DB_0
#define DMA_BUFFER_SIZE   1024

// static adc_continuous_handle_t adc_handle;

#define RES             ADC_RES_10BIT
#define MIC_ADC_LINE 		ADC_LINE(8)

// Yanked from adc_dma_example_main.c of esp-idf (commit 1329b19fe4)
#define TIMES              256
#define GET_UNIT(x)        ((x>>3) & 0x1)

#define ADC_RESULT_BYTE     4
#define ADC_CONV_LIMIT_EN   0
#define ADC_CONV_MODE       ADC_CONV_BOTH_UNIT
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE2

static uint16_t adc1_chan_mask = BIT(2) | BIT(3);
static uint16_t adc2_chan_mask = BIT(0);
static adc_channel_t channel[3] = {ADC1_CHANNEL_2, ADC1_CHANNEL_3, (ADC2_CHANNEL_0 | 1 << 3)};
static const uint8_t channel_num = sizeof(channel) / sizeof(adc_channel_t);

static void configure_adc_dma(void) {
	// uint8_t channel_num;
	//
	// adc_digi_init_config_t adc_dma_config = {
	// 	.max_store_buf_size = 1024,
	// 	.conv_num_each_intr = TIMES,
	// 	.adc1_chan_mask = adc1_chan_mask,
	// 	.adc2_chan_mask = adc2_chan_mask,
	// };
	// adc_digi_initialize(&adc_dma_config);
	// ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

	// adc_digi_configuration_t dig_cfg = {
	// 	.conv_limit_en = ADC_CONV_LIMIT_EN,
	// 	.conv_limit_num = 250,
	// 	.sample_freq_hz = 10 * 1000,
	// 	.conv_mode = ADC_CONV_MODE,
	// 	.format = ADC_OUTPUT_TYPE,
	// };
	//
	// adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
	// dig_cfg.pattern_num = channel_num;
	// for (int i = 0; i < channel_num; i++) {
	// 	uint8_t unit = GET_UNIT(channel[i]);
	// 	uint8_t ch = channel[i] & 0x7;
	// 	adc_pattern[i].atten = ADC_ATTEN_DB_0;
	// 	adc_pattern[i].channel = ch;
	// 	adc_pattern[i].unit = unit;
	// 	adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
	//
	// 	logprint("adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
	// 	logprint("adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
	// 	logprint("adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
	// }
	// dig_cfg.adc_pattern = adc_pattern;
	// ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}
// continuous_adc_init(adc1_chan_mask, adc2_chan_mask, channel, sizeof(channel) / sizeof(adc_channel_t));

void Mic_Init(void)
{
	// /* initialize all available ADC lines */
	// if (adc_init(MIC_ADC_LINE) < 0) {
	// 	printf("Initialization of ADC_LINE(%u) failed\n", MIC_ADC_LINE);
	// 	return 1;
	// } else {
	// 	printf("Successfully initialized ADC_LINE(%u)\n", MIC_ADC_LINE);
	// }
	configure_adc_dma();
}

int Mic_GetSingleSample(void)
{
	return 0;
	// return adc_sample(MIC_ADC_LINE, RES);
}

void Mic_Test(void)
{
  int sample;
	sample = Mic_GetSingleSample();
	printf("%i\n", sample);
}
