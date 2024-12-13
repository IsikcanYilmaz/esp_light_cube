#include "dma_rmt_test.h"
// #include "esp_private/periph_ctrl.h"
#include "hal/dma_types.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "esp_intr_alloc.h"
#include "driver/rmt.h"
#include "hal/rmt_types.h"
#include "hal/rmt_ll.h"
#include "soc/rmt_struct.h"
#include "esp_private/esp_clk.h"
#include "hal/cpu_hal.h"
#include "soc/rtc.h"
#include "board.h"
#include "log.h"
#include "periph_cpu.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "log.h"
#include "periph_cpu.h"

#include "ws281x.h"
#include "ws281x_params.h"
#include "ws281x_constants.h"

#include "esp_private/esp_clk.h"
#include "hal/cpu_hal.h"
#include "soc/rtc.h"
// TESTING DMA->RMT
//

// #define RMT_DEFAULT_CONFIG_TX(gpio, channel_id)      \
//     {                                                \
//         .rmt_mode = RMT_MODE_TX,                     \
//         .channel = channel_id,                       \
//         .gpio_num = gpio,                            \
//         .clk_div = 80,                               \ // 1MHz
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

uint8_t channel;
static int init_rmt(void)
{
	// static_assert(RMT_CH_NUMOF <= RMT_CH_NUMOF_MAX,
	//                 "[ws281x_esp32] RMT configuration problem");

	/* determine used RMT channel from configured GPIO */
	// uint8_t channel = _rmt_channel(dev);
	channel = 0;
	gpio_t testpin = GPIO_PIN(0, 1); // LED0_PIN;
	gpio_init(testpin, GPIO_OUT);
	rmt_config_t cfg = RMT_DEFAULT_CONFIG_TX(testpin, channel); // this is in the espsdk 

	cfg.clk_div = 2;

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

#define DMA_BUFFER_SIZE 16
uint8_t dma_buffer[DMA_BUFFER_SIZE];
static int init_dma(void)
{
	// dma_descriptor_t dma_desc = {
	// 	.buffer = dma_buffer,
	// 	.length = DMA_BUFFER_SIZE
	// };
	// assert(((uint32_t)dma_desc.buffer & 0x3) == 0); // Buffer address must be word-aligned
	


	return 0;
}

#define SIGNAL_LEN (4*4*5*3*8)
rmt_item32_t signal[SIGNAL_LEN];
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
	memset(&signal, 0, sizeof(rmt_item32_t) * SIGNAL_LEN);
	
	uint32_t freq;
	if (rmt_get_counter_clock(channel, &freq) != ESP_OK) {
		LOG_ERROR("[ws281x_esp32] Could not get RMT counter clock\n");
		return;
	}

	uint32_t total_cycles = freq / (NS_PER_SEC / WS281X_T_DATA_NS);
	uint32_t one_on, one_off, zero_on, zero_off;
	one_on = freq / (NS_PER_SEC / WS281X_T_DATA_ONE_NS);
	one_off = total_cycles - one_on;
	zero_on = freq / (NS_PER_SEC / WS281X_T_DATA_ZERO_NS);
	zero_off = total_cycles - zero_on;

	const rmt_item32_t bit0 = {{{ zero_on, 1, zero_off, 0}}};
	const rmt_item32_t bit1 = {{{ one_on, 1, one_off, 0}}};

	for (int i = 0; i < SIGNAL_LEN; i++)
	{
		signal[i] = (i%3 == 0) ? bit1 : bit0;
	}
	
	while(true)
	{
		uint32_t t0 = ztimer_now(ZTIMER_USEC);
		esp_err_t ret = rmt_write_items(channel, &signal, SIGNAL_LEN, false);
		uint32_t t1 = ztimer_now(ZTIMER_USEC);
		printf("%d siglen %d in %d\n", ret, SIGNAL_LEN, t1-t0);
		printf("one on %d one off %d zero on %d zero off %d\n", one_on, one_off, zero_on, zero_off);
		ztimer_sleep(ZTIMER_USEC, 1 * US_PER_SEC);
		// break;
	}

}


void dma_rmt_test(void)
{
	if (init_rmt() == 0)
	{
		printf("rmt initd\n");
	}
	// if (init_dma() == 0)
	// {
	// 	printf("rmt initd\n");
	// }
	rmt_square_wave();
}
