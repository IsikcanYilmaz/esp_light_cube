#include "dma_rmt_test.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "esp_intr_alloc.h"
#include "driver/rmt.h"
#include "hal/rmt_types.h"
#include "hal/rmt_ll.h"
#include "soc/rmt_struct.h"

// TESTING DMA->RMT
//

// #define RMT_DEFAULT_CONFIG_TX(gpio, channel_id)      \
//     {                                                \
//         .rmt_mode = RMT_MODE_TX,                     \
//         .channel = channel_id,                       \
//         .gpio_num = gpio,                            \
//         .clk_div = 80,                               \
//         .mem_block_num = 1,                          \
//         .flags = 0,                                  \
//         .tx_config = {                               \
//             .carrier_freq_hz = 38000,                \
//             .carrier_level = RMT_CARRIER_LEVEL_HIGH, \
//             .idle_level = RMT_IDLE_LEVEL_LOW,        \
//             .carrier_duty_percent = 33,              \
//             .carrier_en = false,                     \
//             .loop_en = false,                        \
//             .idle_output_en = true,                  \
//         }                                            \
//     }

static int init_rmt(void)
{
	// static_assert(RMT_CH_NUMOF <= RMT_CH_NUMOF_MAX,
	//                 "[ws281x_esp32] RMT configuration problem");

	/* determine used RMT channel from configured GPIO */
	// uint8_t channel = _rmt_channel(dev);
	uint8_t channel = 0;

	rmt_config_t cfg = RMT_DEFAULT_CONFIG_TX(LED0_PIN, channel); // this is in the espsdk 

	cfg.clk_div = 2;

	// if ((rmt_config(&cfg) != ESP_OK) ||
	// 	(rmt_driver_install(channel, 0, 0) != ESP_OK) ||
	// 	(rmt_translator_init(channel, ws2812_rmt_adapter) != ESP_OK)) {
	// 	printf("[ws281x_esp32] RMT initialization failed\n");
	// 	return -EIO;
	// }
	
	if (rmt_config(&cfg) != ESP_OK)
	{
		printf("%s:%d\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	if (rmt_driver_install(channel, 0, 0) != ESP_OK)
	{
		printf("%s:%d\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	return 0;
}

static void rmt_square_wave(void)
{
	// So, 
	// lets try outputting a 1Hz square wave
	// half a second high half a second low 
	
	// Rmt uses a struct like so as its "items"
	//
	// 	typedef struct rmt_item32_s {
	//     union {
	//         struct {
	//             uint32_t duration0 :15;
	//             uint32_t level0 :1;
	//             uint32_t duration1 :15;
	//             uint32_t level1 :1;
	//         };
	//         uint32_t val;
	//     };
	// } rmt_item32_t;
	
	// Lets do it without a translator
	// rmt_item32_t 
	


}


void dma_rmt_test(void)
{
	if (init_rmt() == 0)
	{
		printf("rmt initd\n");
	}
}
