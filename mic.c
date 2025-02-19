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

static void configure_adc_dma(void) { // yanked from idf's example code
	adc_digi_init_config_t adc_dma_config = {
		.max_store_buf_size = 1024,
		.conv_num_each_intr = TIMES,
		.adc1_chan_mask = adc1_chan_mask,
		.adc2_chan_mask = adc2_chan_mask,
	};
	adc_digi_initialize(&adc_dma_config);
	ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

	adc_digi_configuration_t dig_cfg = {
		.conv_limit_en = ADC_CONV_LIMIT_EN,
		.conv_limit_num = 250,
		.sample_freq_hz = 10 * 1000,
		.conv_mode = ADC_CONV_MODE,
		.format = ADC_OUTPUT_TYPE,
	};

	adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
	dig_cfg.pattern_num = channel_num;
	for (int i = 0; i < channel_num; i++) {
		uint8_t unit = GET_UNIT(channel[i]);
		uint8_t ch = channel[i] & 0x7;
		adc_pattern[i].atten = ADC_ATTEN_DB_0;
		adc_pattern[i].channel = ch;
		adc_pattern[i].unit = unit;
		adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

		logprint("adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
		logprint("adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
		logprint("adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
	}
	dig_cfg.adc_pattern = adc_pattern;
	ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}

// typedef struct adc_digi_context_t {
//     uint8_t                         *rx_dma_buf;                //dma buffer
//     adc_hal_context_t               hal;                        //hal context
// #if SOC_GDMA_SUPPORTED
//     gdma_channel_handle_t           rx_dma_channel;             //dma rx channel handle
// #elif CONFIG_IDF_TARGET_ESP32S2
//     spi_host_device_t               spi_host;                   //ADC uses this SPI DMA
//     intr_handle_t                   intr_hdl;                   //Interrupt handler
// #elif CONFIG_IDF_TARGET_ESP32
//     i2s_port_t                      i2s_host;                   //ADC uses this I2S DMA
//     intr_handle_t                   intr_hdl;                   //Interrupt handler
// #endif
//     RingbufHandle_t                 ringbuf_hdl;                //RX ringbuffer handler
//     intptr_t                        rx_eof_desc_addr;           //eof descriptor address of RX channel
//     bool                            ringbuf_overflow_flag;      //1: ringbuffer overflow
//     bool                            driver_start_flag;          //1: driver is started; 0: driver is stoped
//     bool                            use_adc1;                   //1: ADC unit1 will be used; 0: ADC unit1 won't be used.
//     bool                            use_adc2;                   //1: ADC unit2 will be used; 0: ADC unit2 won't be used. This determines whether to acquire sar_adc2_mutex lock or not.
//     adc_atten_t                     adc1_atten;                 //Attenuation for ADC1. On this chip each ADC can only support one attenuation.
//     adc_atten_t                     adc2_atten;                 //Attenuation for ADC2. On this chip each ADC can only support one attenuation.
//     adc_hal_digi_ctrlr_cfg_t             hal_digi_ctrlr_cfg;         //Hal digital controller configuration
//     esp_pm_lock_handle_t            pm_lock;                    //For power management
// } adc_digi_context_t;
//
// static adc_digi_context_t *s_adc_digi_ctx = NULL;

static void adc_digi_read_bytes_port(uint8_t *buf, uint32_t length_max, uint32_t *out_length, uint32_t timeout_ms)
{
    // TickType_t ticks_to_wait;
    // esp_err_t ret = ESP_OK;
    // uint8_t *data = NULL;
    // size_t size = 0;

    // ticks_to_wait = timeout_ms / portTICK_RATE_MS;
    // if (timeout_ms == ADC_MAX_DELAY) {
    //     ticks_to_wait = portMAX_DELAY;
    // }

    // data = xRingbufferReceiveUpTo(s_adc_digi_ctx->ringbuf_hdl, &size, ticks_to_wait, length_max);
    // if (!data) {
    //     ESP_LOGV(ADC_TAG, "No data, increase timeout or reduce conv_num_each_intr");
    //     ret = ESP_ERR_TIMEOUT;
    //     *out_length = 0;
    //     return ret;
    // }
    //
    // memcpy(buf, data, size);
    // vRingbufferReturnItem(s_adc_digi_ctx->ringbuf_hdl, data);
    // assert((size % 4) == 0);
    // *out_length = size;
    //
    // if (s_adc_digi_ctx->ringbuf_overflow_flag) {
    //     ret = ESP_ERR_INVALID_STATE;
    // }
    //
    // return ret;
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
	configure_adc_dma();
	adc_digi_start();
	// adc_digi_stop();
}

int Mic_GetSingleSample(void)
{
	esp_err_t ret;
	uint32_t ret_num = 0;
	uint8_t times = 8;
	uint8_t result[times];
	ret = adc_digi_read_bytes(result, times, &ret_num, ADC_MAX_DELAY); // This function uses 
	for (int i = 0; i < times; i++)
	{
		logprint("%d ", result[i]);
	}
	logprint("\n");
	return 0;
	// return adc_sample(MIC_ADC_LINE, RES);
}

void Mic_Test(void)
{
	Mic_GetSingleSample();
	// printf("%i\n", sample);
}
