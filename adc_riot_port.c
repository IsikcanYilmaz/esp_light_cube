/*
 * JON JON JON JON
 * So as of 2025 RIOT doesnt have direct access to ESP-IDF's adc code which does some dma for you
 * Internally, it uses FreeRTOS's ringbuffers and there's no FreeRTOS code in RIOT. Hence this porting
 * effort.
 *
 * I hope one day I can remove this code from my repo
 *
 */

#include "adc_riot_port.h"

#if 0
#include "driver/periph_ctrl.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "hal/adc_types.h"
#include "hal/adc_hal.h"
#include "hal/dma_types.h"

// #include "esp_efuse_rtc_calib.h"
#include "esp_private/gdma.h"


/*---------------------------------------------------------------
                    Digital Controller Context
---------------------------------------------------------------*/
typedef struct adc_digi_context_t {
    uint8_t                         *rx_dma_buf;                //dma buffer
    adc_hal_context_t               hal;                        //hal context
    gdma_channel_handle_t           rx_dma_channel;             //dma rx channel handle
    // RingbufHandle_t                 ringbuf_hdl;                //RX ringbuffer handler
    intptr_t                        rx_eof_desc_addr;           //eof descriptor address of RX channel
    bool                            ringbuf_overflow_flag;      //1: ringbuffer overflow
    bool                            driver_start_flag;          //1: driver is started; 0: driver is stoped
    bool                            use_adc1;                   //1: ADC unit1 will be used; 0: ADC unit1 won't be used.
    bool                            use_adc2;                   //1: ADC unit2 will be used; 0: ADC unit2 won't be used. This determines whether to acquire sar_adc2_mutex lock or not.
    adc_atten_t                     adc1_atten;                 //Attenuation for ADC1. On this chip each ADC can only support one attenuation.
    adc_atten_t                     adc2_atten;                 //Attenuation for ADC2. On this chip each ADC can only support one attenuation.
    adc_hal_digi_ctrlr_cfg_t             hal_digi_ctrlr_cfg;         //Hal digital controller configuration
    esp_pm_lock_handle_t            pm_lock;                    //For power management
} adc_digi_context_t;

static adc_digi_context_t *s_adc_digi_ctx = NULL;

static int8_t adc_digi_get_io_num(uint8_t adc_unit, uint8_t adc_channel)
{
	return adc_channel_io_map[adc_unit][adc_channel];
}

static esp_err_t adc_digi_gpio_init(adc_unit_t adc_unit, uint16_t channel_mask)
{
	esp_err_t ret = ESP_OK;
	uint64_t gpio_mask = 0;
	uint32_t n = 0;
	int8_t io = 0;

	while (channel_mask) {
		if (channel_mask & 0x1) {
			io = adc_digi_get_io_num(adc_unit, n);
			if (io < 0) {
				return ESP_ERR_INVALID_ARG;
			}
			gpio_mask |= BIT64(io);
		}
		channel_mask = channel_mask >> 1;
		n++;
	}

	gpio_config_t cfg = {
		.pin_bit_mask = gpio_mask,
		.mode = GPIO_MODE_DISABLE,
	};
	ret = gpio_config(&cfg);

	return ret;
}

static IRAM_ATTR bool s_adc_dma_intr(adc_digi_context_t *adc_digi_ctx) // JON this seems to be the callback/isr
{
    portBASE_TYPE taskAwoken = 0;
    BaseType_t ret;
    adc_hal_dma_desc_status_t status = false;
    dma_descriptor_t *current_desc = NULL;

    while (1) {
        status = adc_hal_get_reading_result(&adc_digi_ctx->hal, adc_digi_ctx->rx_eof_desc_addr, &current_desc);
        if (status != ADC_HAL_DMA_DESC_VALID) {
            break;
        }

        ret = xRingbufferSendFromISR(adc_digi_ctx->ringbuf_hdl, current_desc->buffer, current_desc->dw0.length, &taskAwoken);
        if (ret == pdFALSE) {
            //ringbuffer overflow
            adc_digi_ctx->ringbuf_overflow_flag = 1;
        }
    }

    if (status == ADC_HAL_DMA_DESC_NULL) {
        //start next turns of dma operation
        adc_hal_digi_start(&adc_digi_ctx->hal, adc_digi_ctx->rx_dma_buf);
    }

    return (taskAwoken == pdTRUE);
}

static IRAM_ATTR bool adc_dma_in_suc_eof_callback(gdma_channel_handle_t dma_chan, gdma_event_data_t *event_data, void *user_data)
{
	assert(event_data);
	s_adc_digi_ctx->rx_eof_desc_addr = event_data->rx_eof_desc_addr;
	return s_adc_dma_intr(user_data);
}

esp_err_t riot_adc_digi_initialize(const adc_digi_init_config_t *init_config) // JON you want this bit
{
	esp_err_t ret = ESP_OK;

	s_adc_digi_ctx = calloc(1, sizeof(adc_digi_context_t));
	if (s_adc_digi_ctx == NULL) {
		ret = ESP_ERR_NO_MEM;
		goto cleanup;
	}

	//ringbuffer
	s_adc_digi_ctx->ringbuf_hdl = xRingbufferCreate(init_config->max_store_buf_size, RINGBUF_TYPE_BYTEBUF);
	if (!s_adc_digi_ctx->ringbuf_hdl) {
		ret = ESP_ERR_NO_MEM;
		goto cleanup;
	}

	//malloc internal buffer used by DMA
	s_adc_digi_ctx->rx_dma_buf = heap_caps_calloc(1, init_config->conv_num_each_intr * INTERNAL_BUF_NUM, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA); // JON so this allocates memory for the dma reception. I could use static memory here
	if (!s_adc_digi_ctx->rx_dma_buf) {
		ret = ESP_ERR_NO_MEM;
		goto cleanup;
	}

	//malloc dma descriptor
	s_adc_digi_ctx->hal.rx_desc = heap_caps_calloc(1, (sizeof(dma_descriptor_t)) * INTERNAL_BUF_NUM, MALLOC_CAP_DMA);
	if (!s_adc_digi_ctx->hal.rx_desc) {
		ret = ESP_ERR_NO_MEM;
		goto cleanup;
	}

	//malloc pattern table
	s_adc_digi_ctx->hal_digi_ctrlr_cfg.adc_pattern = calloc(1, SOC_ADC_PATT_LEN_MAX * sizeof(adc_digi_pattern_config_t));
	if (!s_adc_digi_ctx->hal_digi_ctrlr_cfg.adc_pattern) {
		ret = ESP_ERR_NO_MEM;
		goto cleanup;
	}

	//init gpio pins
	if (init_config->adc1_chan_mask) {
		ret = adc_digi_gpio_init(ADC_NUM_1, init_config->adc1_chan_mask);
		if (ret != ESP_OK) {
			goto cleanup;
		}
	}
	if (init_config->adc2_chan_mask) {
		ret = adc_digi_gpio_init(ADC_NUM_2, init_config->adc2_chan_mask);
		if (ret != ESP_OK) {
			goto cleanup;
		}
	}

	//alloc rx gdma channel
	gdma_channel_alloc_config_t rx_alloc_config = {
		.direction = GDMA_CHANNEL_DIRECTION_RX,
	};
	ret = gdma_new_channel(&rx_alloc_config, &s_adc_digi_ctx->rx_dma_channel);
	if (ret != ESP_OK) {
		goto cleanup;
	}
	gdma_connect(s_adc_digi_ctx->rx_dma_channel, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_ADC, 0));

	gdma_strategy_config_t strategy_config = {
		.auto_update_desc = true,
		.owner_check = true
	};
	gdma_apply_strategy(s_adc_digi_ctx->rx_dma_channel, &strategy_config);

	gdma_rx_event_callbacks_t cbs = {
		.on_recv_eof = adc_dma_in_suc_eof_callback
	};
	gdma_register_rx_event_callbacks(s_adc_digi_ctx->rx_dma_channel, &cbs, s_adc_digi_ctx);

	int dma_chan;
	gdma_get_channel_id(s_adc_digi_ctx->rx_dma_channel, &dma_chan);

	adc_hal_config_t config = {
		.dev = (void *)GDMA_LL_GET_HW(0),
		.desc_max_num = INTERNAL_BUF_NUM,
		.dma_chan = dma_chan,
		.eof_num = init_config->conv_num_each_intr / ADC_HAL_DATA_LEN_PER_CONV
	};
	adc_hal_context_config(&s_adc_digi_ctx->hal, &config);

	//enable ADC digital part
	periph_module_enable(PERIPH_SARADC_MODULE);
	//reset ADC digital part
	periph_module_reset(PERIPH_SARADC_MODULE);

	adc_hal_calibration_init(ADC_NUM_1);
	adc_hal_calibration_init(ADC_NUM_2);

	return ret;

cleanup:
	adc_digi_deinitialize();
	return ret;
}

esp_err_t riot_adc_digi_controller_configure(const adc_digi_configuration_t *config)
{
	if (!s_adc_digi_ctx) {
		return ESP_ERR_INVALID_STATE;
	}

	//Pattern related check
	ESP_RETURN_ON_FALSE(config->pattern_num <= SOC_ADC_PATT_LEN_MAX, ESP_ERR_INVALID_ARG, ADC_TAG, "Max pattern num is %d", SOC_ADC_PATT_LEN_MAX);
	for (int i = 0; i < config->pattern_num; i++) {
		ESP_RETURN_ON_FALSE((config->adc_pattern[i].bit_width == SOC_ADC_DIGI_MAX_BITWIDTH), ESP_ERR_INVALID_ARG, ADC_TAG, "ADC bitwidth not supported");
	}
	ESP_RETURN_ON_FALSE(config->sample_freq_hz <= SOC_ADC_SAMPLE_FREQ_THRES_HIGH && config->sample_freq_hz >= SOC_ADC_SAMPLE_FREQ_THRES_LOW, ESP_ERR_INVALID_ARG, ADC_TAG, "ADC sampling frequency out of range");

	ESP_RETURN_ON_FALSE(config->format == ADC_DIGI_OUTPUT_FORMAT_TYPE2, ESP_ERR_INVALID_ARG, ADC_TAG, "Please use type2");

	s_adc_digi_ctx->hal_digi_ctrlr_cfg.conv_limit_en = config->conv_limit_en;
	s_adc_digi_ctx->hal_digi_ctrlr_cfg.conv_limit_num = config->conv_limit_num;
	s_adc_digi_ctx->hal_digi_ctrlr_cfg.adc_pattern_len = config->pattern_num;
	s_adc_digi_ctx->hal_digi_ctrlr_cfg.sample_freq_hz = config->sample_freq_hz;
	s_adc_digi_ctx->hal_digi_ctrlr_cfg.conv_mode = config->conv_mode;
	memcpy(s_adc_digi_ctx->hal_digi_ctrlr_cfg.adc_pattern, config->adc_pattern, config->pattern_num * sizeof(adc_digi_pattern_config_t));

	const int atten_uninitialized = 999;
	s_adc_digi_ctx->adc1_atten = atten_uninitialized;
	s_adc_digi_ctx->adc2_atten = atten_uninitialized;
	s_adc_digi_ctx->use_adc1 = 0;
	s_adc_digi_ctx->use_adc2 = 0;
	for (int i = 0; i < config->pattern_num; i++) {
		const adc_digi_pattern_config_t *pat = &config->adc_pattern[i];
		if (pat->unit == ADC_NUM_1) {
			s_adc_digi_ctx->use_adc1 = 1;

			if (s_adc_digi_ctx->adc1_atten == atten_uninitialized) {
				s_adc_digi_ctx->adc1_atten = pat->atten;
			} else if (s_adc_digi_ctx->adc1_atten != pat->atten) {
				return ESP_ERR_INVALID_ARG;
			}
		} else if (pat->unit == ADC_NUM_2) {
			//See whether ADC2 will be used or not. If yes, the ``sar_adc2_mutex`` should be acquired in the continuous read driver
			s_adc_digi_ctx->use_adc2 = 1;

			if (s_adc_digi_ctx->adc2_atten == atten_uninitialized) {
				s_adc_digi_ctx->adc2_atten = pat->atten;
			} else if (s_adc_digi_ctx->adc2_atten != pat->atten) {
				return ESP_ERR_INVALID_ARG;
			}
		}
	}

	return ESP_OK;
}

esp_err_t riot_adc_digi_start(void)
{
	if (s_adc_digi_ctx) {
		if (s_adc_digi_ctx->driver_start_flag != 0) {
			ESP_LOGE(ADC_TAG, "The driver is already started");
			return ESP_ERR_INVALID_STATE;
		}
		adc_power_acquire();
		//reset flags
		s_adc_digi_ctx->ringbuf_overflow_flag = 0;
		s_adc_digi_ctx->driver_start_flag = 1;
		if (s_adc_digi_ctx->use_adc1) {
			SAR_ADC1_LOCK_ACQUIRE();
		}
		if (s_adc_digi_ctx->use_adc2) {
			SAR_ADC2_LOCK_ACQUIRE();
		}
		
		adc_hal_init();
		adc_arbiter_t config = ADC_ARBITER_CONFIG_DEFAULT();
		adc_hal_arbiter_config(&config);

		adc_hal_set_controller(ADC_NUM_1, ADC_HAL_CONTINUOUS_READ_MODE);
		adc_hal_set_controller(ADC_NUM_2, ADC_HAL_CONTINUOUS_READ_MODE);

		adc_hal_digi_init(&s_adc_digi_ctx->hal);
		adc_hal_digi_controller_config(&s_adc_digi_ctx->hal, &s_adc_digi_ctx->hal_digi_ctrlr_cfg);

		//start conversion
		adc_hal_digi_start(&s_adc_digi_ctx->hal, s_adc_digi_ctx->rx_dma_buf);
	}
	return ESP_OK;
}

esp_err_t riot_adc_digi_read_bytes(uint8_t *buf, uint32_t length_max, uint32_t *out_length, uint32_t timeout_ms)
{
	TickType_t ticks_to_wait;
	esp_err_t ret = ESP_OK;
	uint8_t *data = NULL;
	size_t size = 0;

	ticks_to_wait = timeout_ms / portTICK_RATE_MS;
	if (timeout_ms == ADC_MAX_DELAY) {
		ticks_to_wait = portMAX_DELAY;
	}

	data = xRingbufferReceiveUpTo(s_adc_digi_ctx->ringbuf_hdl, &size, ticks_to_wait, length_max);
	if (!data) {
		ESP_LOGV(ADC_TAG, "No data, increase timeout or reduce conv_num_each_intr");
		ret = ESP_ERR_TIMEOUT;
		*out_length = 0;
		return ret;
	}

	memcpy(buf, data, size);
	vRingbufferReturnItem(s_adc_digi_ctx->ringbuf_hdl, data);
	assert((size % 4) == 0);
	*out_length = size;

	if (s_adc_digi_ctx->ringbuf_overflow_flag) {
		ret = ESP_ERR_INVALID_STATE;
	}

	return ret;
}

esp_err_t riot_adc_digi_stop(void)
{
	if (s_adc_digi_ctx) {
		if (s_adc_digi_ctx->driver_start_flag != 1) {
			ESP_LOGE(ADC_TAG, "The driver is already stopped");
			return ESP_ERR_INVALID_STATE;
		}
		s_adc_digi_ctx->driver_start_flag = 0;

		//disable the in suc eof intrrupt
		adc_hal_digi_dis_intr(&s_adc_digi_ctx->hal, ADC_HAL_DMA_INTR_MASK);
		//clear the in suc eof interrupt
		adc_hal_digi_clr_intr(&s_adc_digi_ctx->hal, ADC_HAL_DMA_INTR_MASK);
		//stop ADC
		adc_hal_digi_stop(&s_adc_digi_ctx->hal);

		adc_hal_digi_deinit(&s_adc_digi_ctx->hal);

		if (s_adc_digi_ctx->use_adc1) {
			SAR_ADC1_LOCK_RELEASE();
		}
		if (s_adc_digi_ctx->use_adc2) {
			SAR_ADC2_LOCK_RELEASE();
		}
		adc_power_release();
	}
	return ESP_OK;
}

esp_err_t riot_adc_digi_deinitialize(void)
{
	if (!s_adc_digi_ctx) {
		return ESP_ERR_INVALID_STATE;
	}

	if (s_adc_digi_ctx->driver_start_flag != 0) {
		ESP_LOGE(ADC_TAG, "The driver is not stopped");
		return ESP_ERR_INVALID_STATE;
	}

	if (s_adc_digi_ctx->ringbuf_hdl) {
		vRingbufferDelete(s_adc_digi_ctx->ringbuf_hdl);
		s_adc_digi_ctx->ringbuf_hdl = NULL;
	}

	free(s_adc_digi_ctx->rx_dma_buf);
	free(s_adc_digi_ctx->hal.rx_desc);
	free(s_adc_digi_ctx->hal_digi_ctrlr_cfg.adc_pattern);
	gdma_disconnect(s_adc_digi_ctx->rx_dma_channel);
	gdma_del_channel(s_adc_digi_ctx->rx_dma_channel);
	free(s_adc_digi_ctx);
	s_adc_digi_ctx = NULL;
	periph_module_disable(PERIPH_SARADC_MODULE);
	return ESP_OK;
}

#endif
