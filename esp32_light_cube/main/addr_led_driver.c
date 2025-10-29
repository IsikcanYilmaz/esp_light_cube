#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "addr_led_driver.h"
#include "usr_commands.h"

// RIOT OS
#if defined(LIGHT_CUBE_RIOT) ///////////////////////////////////////////////////////
#include "button.h"
#include "usr_commands.h"
#include "ztimer.h"
#include "xtimer.h"
#include "thread.h"

/*WS2812B Related */
ws281x_t neopixelHandle;
uint8_t pixelBuffer[NEOPIXEL_SIGNAL_BUFFER_LEN];
ws281x_params_t neopixelParams = {
  .buf = pixelBuffer,
  .numof = NUM_LEDS,
  .pin = NEOPIXEL_SIGNAL_GPIO_PIN
};

// ESP IDF
#elif defined(LIGHT_CUBE_ESP_IDF) ///////////////////////////////////////////////////////
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "esp_check.h"

static const char *TAG = "AddrLedDriver";

// Yanked out of the ESP IDF ledstrip example. maybe put these in a different file? TODO
typedef struct {
  uint32_t resolution; /*!< Encoder resolution, in Hz */
} led_strip_encoder_config_t;
typedef struct {
  rmt_encoder_t base;
  rmt_encoder_t *bytes_encoder;
  rmt_encoder_t *copy_encoder;
  int state;
  rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
  rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
  rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
  rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
  rmt_encode_state_t session_state = RMT_ENCODING_RESET;
  rmt_encode_state_t state = RMT_ENCODING_RESET;
  size_t encoded_symbols = 0;
  switch (led_encoder->state) {
    case 0: // send RGB data
      encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
      if (session_state & RMT_ENCODING_COMPLETE) {
        led_encoder->state = 1; // switch to next state when current encoding session finished
      }
      if (session_state & RMT_ENCODING_MEM_FULL) {
        state |= RMT_ENCODING_MEM_FULL;
        goto out; // yield if there's no free space for encoding artifacts
      }
    // fall-through
    case 1: // send reset code
      encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                              sizeof(led_encoder->reset_code), &session_state);
      if (session_state & RMT_ENCODING_COMPLETE) {
        led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
        state |= RMT_ENCODING_COMPLETE;
      }
      if (session_state & RMT_ENCODING_MEM_FULL) {
        state |= RMT_ENCODING_MEM_FULL;
        goto out; // yield if there's no free space for encoding artifacts
      }
  }
out:
  *ret_state = state;
  return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
  rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
  rmt_del_encoder(led_encoder->bytes_encoder);
  rmt_del_encoder(led_encoder->copy_encoder);
  free(led_encoder);
  return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
  rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
  rmt_encoder_reset(led_encoder->bytes_encoder);
  rmt_encoder_reset(led_encoder->copy_encoder);
  led_encoder->state = RMT_ENCODING_RESET;
  return ESP_OK;
}

esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
  esp_err_t ret = ESP_OK;
  rmt_led_strip_encoder_t *led_encoder = NULL;
  ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
  led_encoder = rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
  ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");
  led_encoder->base.encode = rmt_encode_led_strip;
  led_encoder->base.del = rmt_del_led_strip_encoder;
  led_encoder->base.reset = rmt_led_strip_encoder_reset;
  // different led strip might have its own timing requirements, following parameter is for WS2812
  rmt_bytes_encoder_config_t bytes_encoder_config = {
    .bit0 = {
      .level0 = 1,
      .duration0 = 0.3 * config->resolution / 1000000, // T0H=0.3us
      .level1 = 0,
      .duration1 = 0.9 * config->resolution / 1000000, // T0L=0.9us
    },
    .bit1 = {
      .level0 = 1,
      .duration0 = 0.9 * config->resolution / 1000000, // T1H=0.9us
      .level1 = 0,
      .duration1 = 0.3 * config->resolution / 1000000, // T1L=0.3us
    },
    .flags.msb_first = 1 // WS2812 transfer bit order: G7...G0R7...R0B7...B0
  };
  ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");
  rmt_copy_encoder_config_t copy_encoder_config = {};
  ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, TAG, "create copy encoder failed");

  uint32_t reset_ticks = config->resolution / 1000000 * 50 / 2; // reset code duration defaults to 50us
  led_encoder->reset_code = (rmt_symbol_word_t) {
    .level0 = 0,
    .duration0 = reset_ticks,
    .level1 = 0,
    .duration1 = reset_ticks,
  };
  *ret_encoder = &led_encoder->base;
  return ESP_OK;
err:
  if (led_encoder) {
    if (led_encoder->bytes_encoder) {
      rmt_del_encoder(led_encoder->bytes_encoder);
    }
    if (led_encoder->copy_encoder) {
      rmt_del_encoder(led_encoder->copy_encoder);
    }
    free(led_encoder);
  }
  return ret;
}

#else
#error "Need to define either LIGHT_CUBE_RIOT or LIGHT_CUBE_ESP_IDF!"
#endif

// Cube related
Pixel_t ledStrip0Pixels[NUM_LEDS];
AddrLedStrip_t ledStrip0;
AddrLedPanel_t ledPanels[NUM_PANELS];
bool pixelChanged = true;
bool addrLedDriverInitialized = false;

char *positionStrings[NUM_SIDES] = {
  [NORTH] = "NORTH",
  [EAST] = "EAST",
  [SOUTH] = "SOUTH",
  [WEST] = "WEST",
  [TOP] = "TOP"
};

char *directionStrings[NUM_DIRECTIONS] = {
  [UP] = "UP",
  [UP_LEFT] = "UP_LEFT",
  [LEFT] = "LEFT",
  [DOWN_LEFT] = "DOWN_LEFT",
  [DOWN] = "DOWN",
  [DOWN_RIGHT] = "DOWN_RIGHT",
  [RIGHT] = "RIGHT",
  [UP_RIGHT] = "UP_RIGHT"
};

static void InitPanel(AddrLedPanel_t *p)
{
  // Set local coordinates of this panel
  Pixel_t *pixels = p->strip->pixels;
  for (int i = 0; i < p->numLeds; i++)
  {
    Pixel_t *pixel = &pixels[p->stripRange[0] + i];
    pixel->x = NUM_LEDS_PER_PANEL_SIDE - (i / NUM_LEDS_PER_PANEL_SIDE);
    pixel->y = NUM_LEDS_PER_PANEL_SIDE - (i % NUM_LEDS_PER_PANEL_SIDE);

    // TODO // HANDLE GLOBAL COORDINATES
  }

  // Set global coordinates of this panel
}

#if defined(LIGHT_CUBE_RIOT)
static void teststrips(ws281x_t *strip)
{
  // Color_t testHsv = Color_CreateFromHsv(50, 0.5, 0.5);
  color_rgb_t color2 = {10, 0, 0};
  color_rgb_t color1 = {10, 0, 10};
  static bool onColor1 = true;
  for(int i = 0; i < strip->params.numof; i++)
  {
    if (i % 2 == 0)
    {
      onColor1 ? ws281x_set(strip, i, color1) : ws281x_set(strip, i, color2);
    }
    else
  {
      onColor1 ? ws281x_set(strip, i, color2) : ws281x_set(strip, i, color1);
    }
  }
  // ws281x_set(strip, 0, color1);
  onColor1 = !onColor1;
}

static void testfirstled(ws281x_t *strip)
{
  color_rgb_t color2 = {10, 0, 0};
  color_rgb_t color1 = {10, 0, 10};
  color_rgb_t colorBlank = {0,0,0};
  color_rgb_t colorBasic = {1,0,0};
  static bool onColor1 = true;
  // ws281x_set(strip, 0, (onColor1) ? color1 : color2);
  ws281x_set(strip, 0, colorBlank);
  ws281x_set(strip, 1, colorBasic);
  ws281x_set(strip, 2, colorBlank);
  ws281x_set(strip, 3, colorBasic);
  onColor1 = !onColor1;
}

static void testsetpixel(ws281x_t *strip)
{
  Pixel_t *northTop = AddrLedDriver_GetPixelInPanel(SOUTH, 1, 1);
  AddrLedDriver_SetPixelRgbInPanel(NORTH, 0, 0, 20, 0, 0);
  AddrLedDriver_SetPixelRgbInPanel(SOUTH, 0, 0, 20, 0, 0);
  AddrLedDriver_SetPixelRgbInPanel(WEST, 0, 0, 20, 0, 0);
  AddrLedDriver_SetPixelRgbInPanel(EAST, 0, 0, 20, 0, 0);
  AddrLedDriver_SetPixelRgbInPanel(TOP, 0, 0, 20, 0, 0);
}
#endif

static Position_e CharToPosEnum(char c)
{
  Position_e pos = NUM_SIDES;
  switch (c)
  {
    case 'n':
      pos = NORTH;
      break;
    case 'e':
      pos = EAST;
      break;
    case 's':
      pos = SOUTH;
      break;
    case 'w':
      pos = WEST;
      break;
    case 't':
      pos = TOP;
      break;
    default:
      ESP_LOGE(TAG, "BAD SIDE DESCRIPTOR! %c\n", c);
  }
  return pos;
}

// TODO move to appropriate spots

static void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
  h %= 360; // h -> [0,360]
  uint32_t rgb_max = v * 2.55f;
  uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

  uint32_t i = h / 60;
  uint32_t diff = h % 60;

  // RGB adjustment amount by hue
  uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

  switch (i) {
    case 0:
      *r = rgb_max;
      *g = rgb_min + rgb_adj;
      *b = rgb_min;
      break;
    case 1:
      *r = rgb_max - rgb_adj;
      *g = rgb_max;
      *b = rgb_min;
      break;
    case 2:
      *r = rgb_min;
      *g = rgb_max;
      *b = rgb_min + rgb_adj;
      break;
    case 3:
      *r = rgb_min;
      *g = rgb_max - rgb_adj;
      *b = rgb_max;
      break;
    case 4:
      *r = rgb_min + rgb_adj;
      *g = rgb_min;
      *b = rgb_max;
      break;
    default:
      *r = rgb_max;
      *g = rgb_min;
      *b = rgb_max - rgb_adj;
      break;
  }
}
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      9
#define EXAMPLE_LED_NUMBERS         (4 * 4 * 5)
#define EXAMPLE_CHASE_SPEED_MS      100
static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

static rmt_transmit_config_t tx_config = {
  .loop_count = 0, // no transfer loop
};

static uint8_t HardwareInit(void)
{
#if defined(LIGHT_CUBE_RIOT)
  //Init the ws281x module 
  if(ws281x_init(&neopixelHandle, &neopixelParams) != 0) 
  {
    logprint("Failed to initialize ws281\n");
    return;
  }
  else 
{
    logprint("Initialized ws281x. Data length %d one length %d zero length %d\n", WS281X_T_DATA_NS, WS281X_T_DATA_ONE_NS, WS281X_T_DATA_ZERO_NS);
  }

#elif defined(LIGHT_CUBE_ESP_IDF)
  // Yanked out of the ESP IDF led strip example
  uint32_t red = 0;
  uint32_t green = 0;
  uint32_t blue = 0;
  uint16_t hue = 0;
  uint16_t start_rgb = 0;
  ESP_LOGI(TAG, "Create RMT TX channel");
  rmt_tx_channel_config_t tx_chan_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
    .gpio_num = RMT_LED_STRIP_GPIO_NUM,
    .mem_block_symbols = 64, // increase the block size can make the LED less flickering
    .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
    .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));
  ESP_LOGI(TAG, "Install led strip encoder");
  led_strip_encoder_config_t encoder_config = {
    .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
  };
  ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));
  ESP_LOGI(TAG, "Enable RMT TX channel");
  ESP_ERROR_CHECK(rmt_enable(led_chan));
  // ESP_LOGI(TAG, "Start LED rainbow chase");
  // static uint8_t idx = 0;
  while(false)
  {
    // #if 0
    // for (int i = 0; i < 3; i++) {
    //   for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
    //     // Build RGB pixels
    //     hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
    //     led_strip_hsv2rgb(hue, 100, 20, &red, &green, &blue);
    //     led_strip_pixels[j * 3 + 0] = green;
    //     led_strip_pixels[j * 3 + 1] = blue;
    //     led_strip_pixels[j * 3 + 2] = red;
    //   }
    //   // Flush RGB values to LEDs
    //   ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
    //   ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
    //   vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
    // }
    // start_rgb += 1;
  // #endif
  //   hue = idx * 360 / EXAMPLE_LED_NUMBERS;
  //   led_strip_hsv2rgb(hue, 100, 20, &red, &green, &blue);
  //   led_strip_pixels[idx * 3 + 0] = green;
  //   led_strip_pixels[idx * 3 + 1] = blue;
  //   led_strip_pixels[idx * 3 + 2] = red;
  //   idx = (idx + 1) % EXAMPLE_LED_NUMBERS;
  //   ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
  //   ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
  //   vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
  //   memset(led_strip_pixels, 0x00, sizeof(led_strip_pixels));
    //
    // led_strip_pixels[0] = 10;
    // led_strip_pixels[1] = 10;
    // led_strip_pixels[2] = 0;
    // Pixel_t *first = AddrLedDriver_GetPixelInPanel(TOP, 0, 0);
    AddrLedDriver_SetPixelRgbInPanel(SOUTH,1, 2, 10, 10, 0);
    AddrLedDriver_DisplayStrip();
    vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
  }
#endif
  return 0;
}

// this function looks fucking awful
bool AddrLedDriver_Init(void)
{
  // First init our data structures 
  // Initialize the strip(s). This initializes one continuous strip. 
  // If multiple panels are daisychained, that counts as one strip.
  ledStrip0 = (AddrLedStrip_t) {
    .numLeds                 = NUM_LEDS,
    .pixels                  = (Pixel_t *) &ledStrip0Pixels,
    // .neopixelHandle          = &neopixelHandle
  };

  // Initialize the panel structures
  for (int panelIdx = 0; panelIdx < NUM_PANELS; panelIdx++)
  {
    Position_e pos = (Position_e) panelIdx;
    AddrLedPanel_t p = {
      .strip = &ledStrip0,
      .numLeds = NUM_LEDS_PER_PANEL,
      .stripRange = {(panelIdx * NUM_LEDS_PER_PANEL), ((panelIdx + 1) * NUM_LEDS_PER_PANEL - 1)},
      .position = pos,
      .neighborPanels = {NULL, NULL, NULL, NULL}, // TODO
    };
    p.stripFirstPixel = &(p.strip->pixels[p.stripRange[0]]);
    InitPanel(&p);
    ledPanels[pos] = p;
  }

  // Set neighbors of our pixels. this needs to be done better. thankfully we do it once 
  for (int panelIdx = 0; panelIdx < NUM_PANELS; panelIdx++)
  {
    Position_e pos = (Position_e) panelIdx;

    // Initialize the neighbor fields of our pixels. lets see if it works
    for (int x = 0; x < NUM_LEDS_PER_PANEL_SIDE; x++)
    {
      for (int y = 0; y < NUM_LEDS_PER_PANEL_SIDE; y++)
      {
        Pixel_t *pix = AddrLedDriver_GetPixelInPanel(pos, x, y);
        pix->x = x; 
        pix->y = y;
        pix->pos = pos;
        memset(pix->neighborPixels, 0, sizeof(Pixel_t *) * NUM_DIRECTIONS);
        Position_e panelToTheRight = (pos + 1) % TOP;
        Position_e panelToTheLeft = (pos + 1) % TOP;
        // TODO refactor the block below.
        if (pos != TOP) // SIDE PANELS
        {
          if (x == 0) // leftmost column
          {
            // left neighbors are going to be in the panel to the left
            Position_e leftPanelPos = ((uint8_t) pos > 0) ? ((uint8_t) pos - 1) : EAST;
            pix->neighborPixels[LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(leftPanelPos, NUM_LEDS_PER_PANEL_SIDE-1, y);

            // if we're above the bottom
            if (y > 0)
            {
              pix->neighborPixels[DOWN_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(leftPanelPos, NUM_LEDS_PER_PANEL_SIDE-1, y-1);
              pix->neighborPixels[DOWN_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, 1, y-1);
            }

            // if we're a corner, there is no top left neighbor
            // if we're not in a corner then there is a top left neighbor
            if (y < NUM_LEDS_PER_PANEL_SIDE-1)
            {
              pix->neighborPixels[UP_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(leftPanelPos, NUM_LEDS_PER_PANEL_SIDE-1, y+1);
              pix->neighborPixels[UP_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, 1, y+1);
            }

            pix->neighborPixels[RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y);	
          }
          else if (x == NUM_LEDS_PER_PANEL_SIDE-1) // rightmost column
          {
            // right neighbors are going to be in the panel to the right 
            Position_e rightPanelPos = ((uint8_t) pos + 1) % TOP;
            pix->neighborPixels[RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(rightPanelPos, 0, y);

            // if we're above the bottom
            if (y > 0)
            {
              pix->neighborPixels[DOWN_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(rightPanelPos, 0, y-1);
              pix->neighborPixels[DOWN_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, 2, y-1);
            }

            // if we're a corner, there is no top right neighbor
            // if we're not in a corner then there is a top right neighbor
            if (y < NUM_LEDS_PER_PANEL_SIDE-1)
            {
              pix->neighborPixels[UP_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(rightPanelPos, 0, y+1);
              pix->neighborPixels[UP_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, 2, y+1);
            }

            pix->neighborPixels[LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, 2, y);
          }
          else // middle two columns
          {
            pix->neighborPixels[LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y);
            pix->neighborPixels[RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y);

            if (y < NUM_LEDS_PER_PANEL_SIDE-1)
            {
              pix->neighborPixels[UP_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y+1);
              pix->neighborPixels[UP_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y+1);
            }
            if (y > 0)
            {
              pix->neighborPixels[DOWN_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y-1);
              pix->neighborPixels[DOWN_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y-1);
            }
          }

          if (y == NUM_LEDS_PER_PANEL_SIDE - 1) // top
          {
            pix->neighborPixels[UP_LEFT] = (x > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanelRelative(TOP, pos, x-1, 0): NULL;
            pix->neighborPixels[UP_RIGHT] = (x < NUM_LEDS_PER_PANEL_SIDE - 1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanelRelative(TOP, pos, x+1, 0) : NULL;
          }

          // Upper neighbor
          pix->neighborPixels[UP] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x, y+1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanelRelative(TOP, pos, x, 0);

          // Lower neighbor
          pix->neighborPixels[DOWN] = (y > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x, y-1) : NULL;
        }
        else // TOP PANEL 
        {
          // we assume we're looking at the cube from the pov of S
          // So
          //   N   ^
          // W T E |
          //   S   |
          if (x == 0) // leftmost column
          {
            // Regardless of row, the left neighbor will be on the WEST panel and neighbor to the right will be on the TOP panel
            pix->neighborPixels[LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(WEST, NUM_LEDS_PER_PANEL_SIDE-1-y, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, 1, y);
            pix->neighborPixels[DOWN_LEFT] = (y > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(WEST, NUM_LEDS_PER_PANEL_SIDE-y, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;
            pix->neighborPixels[UP_LEFT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(WEST, NUM_LEDS_PER_PANEL_SIDE-2-y, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;
            if (y == 0) // bottom row
            {
              pix->neighborPixels[DOWN] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x, NUM_LEDS_PER_PANEL_SIDE-1);
              pix->neighborPixels[DOWN_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x+1, NUM_LEDS_PER_PANEL_SIDE-1);
              pix->neighborPixels[UP] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
              pix->neighborPixels[UP_RIGHT] = (struct Pixel_t *)AddrLedDriver_GetPixelInPanel(TOP, x+1, y+1);
            }
            else if (y < NUM_LEDS_PER_PANEL_SIDE-1) // mid rows
            {
              pix->neighborPixels[DOWN] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
              pix->neighborPixels[DOWN_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x+1, y-1);
              // pix->neighborPixels[DOWN_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y-1);
              pix->neighborPixels[UP] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
              pix->neighborPixels[UP_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x+1, y+1);
              // pix->neighborPixels[UP_LEFT] = AddrLedDriver_GetPixelInPanel(TOP, x-1, y+1);
            }
            else // top row
            {
              pix->neighborPixels[DOWN] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
              pix->neighborPixels[DOWN_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x+1, y-1);
              pix->neighborPixels[UP] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x, NUM_LEDS_PER_PANEL_SIDE-1);
              pix->neighborPixels[UP_RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-2, NUM_LEDS_PER_PANEL_SIDE-1);
            }
          }
          else if (x < NUM_LEDS_PER_PANEL_SIDE-1) // mid columns 
          {
            pix->neighborPixels[LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y);
            pix->neighborPixels[RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y);
            pix->neighborPixels[DOWN] = (y > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x, y-1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[DOWN_RIGHT] = (y > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y-1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x+1, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[DOWN_LEFT] = (y > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y-1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x-1, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[UP] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x, y+1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[UP_RIGHT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x+1, y+1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x-1, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[UP_LEFT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y+1) : (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x+1, NUM_LEDS_PER_PANEL_SIDE-1);
          }
          else // rightmost column
          {
            // Regardless of row, the right neighbor will be on the EAST panel and neighbor to the left will be on the TOP panel
            pix->neighborPixels[LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(pos, x-1, y);
            pix->neighborPixels[RIGHT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(EAST, y, NUM_LEDS_PER_PANEL_SIDE-1);
            pix->neighborPixels[DOWN_RIGHT] = (y > 0) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(EAST, y-1, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;
            pix->neighborPixels[UP_RIGHT] = (y < NUM_LEDS_PER_PANEL_SIDE-1) ? (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(EAST, y+1, NUM_LEDS_PER_PANEL_SIDE-1) : NULL;

            if (y == 0) // bottom row
            {
              pix->neighborPixels[DOWN] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x, NUM_LEDS_PER_PANEL_SIDE-1);
              pix->neighborPixels[DOWN_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(SOUTH, x-1, NUM_LEDS_PER_PANEL_SIDE-1);
              pix->neighborPixels[UP] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
              pix->neighborPixels[UP_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x-1, y+1);
            }
            else if (y < NUM_LEDS_PER_PANEL_SIDE-1) // mid rows
            {
              pix->neighborPixels[DOWN] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
              // pix->neighborPixels[DOWN_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y-1);
              pix->neighborPixels[DOWN_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x-1, y-1);
              pix->neighborPixels[UP] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y+1);
              // pix->neighborPixels[UP_RIGHT] = AddrLedDriver_GetPixelInPanel(TOP, x+1, y+1);
              pix->neighborPixels[UP_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x-1, y+1);
            }
            else // top row
            {
              pix->neighborPixels[DOWN] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x, y-1);
              pix->neighborPixels[DOWN_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(TOP, x-1, y-1);
              pix->neighborPixels[UP] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, NUM_LEDS_PER_PANEL_SIDE-1-x, NUM_LEDS_PER_PANEL_SIDE-1);
              pix->neighborPixels[UP_LEFT] = (struct Pixel_t *) AddrLedDriver_GetPixelInPanel(NORTH, 1, NUM_LEDS_PER_PANEL_SIDE-1);
            }
          }
        }
      }
    }
  }

  // Initialize our pixels // TODO find a better solution to this? this is for compatibility w the RIOT ws281x module
  // Lets see if its even needed lol
  for (int pIdx = 0; pIdx < NUM_LEDS; pIdx++)
  {
    ledStrip0Pixels[pIdx].stripIdx = pIdx;
  }

  addrLedDriverInitialized = (HardwareInit() == 0);
  return addrLedDriverInitialized;
}

void AddrLedDriver_Test(void)
{
  // teststrips(&neopixelHandle);
  // ws281x_write(&neopixelHandle);

  // testsetpixel(&neopixelHandle);
  // while(true)
  // {
  // testfirstled(&neopixelHandle);
  // AddrLedDriver_DisplayStrip(&ledStrip0);
  // ztimer_sleep(ZTIMER_USEC, 1 * US_PER_SEC);
  // }
}

void AddrLedDriver_DisplayStrip(void)
{
  ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
  ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
  pixelChanged = false;
}

// TODO redundant
void AddrLedDriver_DisplayCube(void)
{
  AddrLedDriver_DisplayStrip();
}

void AddrLedDriver_SetPixelRgb(Pixel_t *p, uint8_t r, uint8_t g, uint8_t b)
{
  // ESP_LOGI(TAG, "%s %d %d %d : r%d g%d b%d", __FUNCTION__, p->pos, p->x, p->y, r, g, b);
  if (p == NULL)
  {
    ESP_LOGE(TAG, "%s received null pixel_t pointer!\n", __FUNCTION__);
    return;
  }
  p->red = r;
  p->green = g;
  p->blue = b;
  led_strip_pixels[p->stripIdx * 3 + 0] = g;
  led_strip_pixels[p->stripIdx * 3 + 1] = b;
  led_strip_pixels[p->stripIdx * 3 + 2] = r;
  pixelChanged = true;
}

// ! Below depends on the LED structs that this file initializes. these could reside in manager code? but not now
// fuck it. this _is_ the manager code. until further notice.
void AddrLedDriver_SetPixelRgbInPanel(Position_e pos, uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b)
{
  // sanity checks
  if (pos >= NUM_SIDES || x >= NUM_LEDS_PER_PANEL_SIDE || y >= NUM_LEDS_PER_PANEL_SIDE)
  {
    ESP_LOGE(TAG, "Incorrect args to SetPixelRgb %d %d %d\n", pos, x, y);
    return;  // TODO have an error type
  }
  AddrLedPanel_t *panel = AddrLedDriver_GetPanelByLocation(pos);
  Pixel_t *pixel = AddrLedDriver_GetPixelInPanel(pos, x, y);
  AddrLedDriver_SetPixelRgb(pixel, r, g, b);
}

void AddrLedDriver_Clear(void)
{
  ESP_LOGI(TAG, "%s:d", __FUNCTION__, __LINE__);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    AddrLedDriver_SetPixelRgb(&ledStrip0.pixels[i], 0, 0, 0);
  }
}

Pixel_t* AddrLedDriver_GetPixelByIdx(uint16_t idx) // a bit strip/situaiton dependent but oh well
{
  if (idx >= NUM_LEDS)
  {
    ESP_LOGE(TAG, "%s bad pixel idx %d\n", idx);
    return NULL;
  }
  return &ledStrip0Pixels[idx];
}

Pixel_t* AddrLedDriver_GetPixelInPanel(Position_e pos, uint8_t x, uint8_t y)
{
  AddrLedPanel_t *panel = AddrLedDriver_GetPanelByLocation(pos);
  AddrLedStrip_t *strip = panel->strip;
  uint8_t ledIdx;

  if (pos != TOP) // HACK !
  {
    // #if LEDS_BEGIN_AT_BOTTOM
    // 		y = NUM_LEDS_PER_PANEL_SIDE - y - 1;
    // #endif
    // 		y = NUM_LEDS_PER_PANEL_SIDE - 1 - y;// HACK ! y seems inverted. flip it
    y = NUM_LEDS_PER_PANEL_SIDE - y - 1;
  }
  else
{
    // HAAAACK!
    uint8_t tmpx = y;
    y = x;
    x = tmpx;
  }

  if (y % 2 == 0)
  {
    ledIdx = x + (NUM_LEDS_PER_PANEL_SIDE * y);
  }
  else
{
    ledIdx = (NUM_LEDS_PER_PANEL_SIDE - 1 - x) + (NUM_LEDS_PER_PANEL_SIDE * y);
  }
  return &(panel->stripFirstPixel[ledIdx]);
}

// Idea behind this function is to retrieve a pixel from the point of view of a different position
// i.e. instead of looking at the panel from the south position, imagine transforming to another position
Pixel_t* AddrLedDriver_GetPixelInPanelRelative(Position_e pos, Position_e relativePos, uint8_t x, uint8_t y)
{
  uint8_t absX, absY;
  // HACK. meh it works 
  if (pos == TOP)
  {
    switch(relativePos)
    {
      case SOUTH:
        {
          absX = x;
          absY = y;
          break;
        }
      case EAST:
        {
          absX = NUM_LEDS_PER_PANEL_SIDE - 1 - y;;
          absY = x;
          break;
        }
      case NORTH:
        {
          absX = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
          absY = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
          break;
        }
      case WEST:
      default:
        {
          absX = y;
          absY = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
          break;
        }
    }
    // absY = NUM_LEDS_PER_PANEL_SIDE - 1 - absY; // HACK i really should refactor this stuff
  }
  else
{
    switch(relativePos)
    {
      case EAST:
        {
          absX = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
          absY = x;
          break;
        }
      case TOP:
      case NORTH:
        {
          absX = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
          absY = NUM_LEDS_PER_PANEL_SIDE - 1 - y;
          break;
        }
      case WEST:
        {
          absX = y;
          absY = NUM_LEDS_PER_PANEL_SIDE - 1 - x;
          break;
        }
      case SOUTH:
      default:
        {
          absX = x;
          absY = y;
          break;
        }
    }
  }
  return AddrLedDriver_GetPixelInPanel(pos, absX, absY);
}

Position_e AddrLedDriver_GetOppositePanel(Position_e pos)
{
  Position_e oppositePos = NUM_SIDES;
  switch(pos)
  {
    case NORTH:
      oppositePos = SOUTH;
      break;
    case SOUTH:
      oppositePos = NORTH;
      break;
    case EAST:
      oppositePos = WEST;
      break;
    case WEST:
      oppositePos = EAST;
      break;
    case TOP:
      oppositePos = TOP; // TODO? Should this return, like, BOTTOM or something? whjich i dont have rn? 
      break;
    default:
      ESP_LOGE(TAG, "Bad pos to %s\n", __FUNCTION__);
      break;
  }
  return oppositePos;
}

AddrLedPanel_t* AddrLedDriver_GetPanelByLocation(Position_e pos)
{
  if (pos >= NUM_SIDES)
  {
    ESP_LOGE(TAG, "Bad pos %d for %s\n", pos, __FUNCTION__);
    return NULL;
  }
  return &ledPanels[pos];
}

void AddrLedDriver_PrintPixel(Pixel_t *p)
{
  ESP_LOGI(TAG, "Pixel panel:%s x:%d y:%d\n neighbors: \n", AddrLedDriver_GetPositionString(p->pos), p->x, p->y);
  for (uint8_t dir = 0; dir < NUM_DIRECTIONS; dir++)
  {
    Pixel_t *n = (Pixel_t *) p->neighborPixels[dir];
    if (n == NULL)
    {
      ESP_LOGI(TAG, "%s \n", AddrLedDriver_GetDirectionString(dir));
      continue;
    }
    ESP_LOGI(TAG, "%s -> %s x:%d y:%d\n", AddrLedDriver_GetDirectionString(dir), AddrLedDriver_GetPositionString(n->pos), n->x, n->y);
  }
}

bool AddrLedDriver_ShouldRedraw(void)
{
  return pixelChanged;
}

char * AddrLedDriver_GetPositionString(Position_e pos)
{
  if (pos >= NUM_SIDES)
  {
    ESP_LOGE(TAG, "Bad pos %d to %s\n", pos, __FUNCTION__);
    return NULL;
  }
  return positionStrings[pos];
}

char * AddrLedDriver_GetDirectionString(Direction_e dir)
{
  if (dir >= NUM_DIRECTIONS)
  {
    ESP_LOGE(TAG, "Bad dir %d to %s\n", dir, __FUNCTION__);
  }
  return directionStrings[dir];
}

AddrLedStrip_t* AddrLedDriver_GetStrip(void)
{
  return &ledStrip0;
}

int AddrLedDriver_TakeUsrCommand(int argc, char **argv)
{
  if (!addrLedDriverInitialized)
  {
  	return 1;
  }
  ASSERT_ARGS(2);
  // if (strcmp(argv[1], "clear") == 0)
  // {
  // 	AddrLedDriver_Clear();
  // }
  // else if (strcmp(argv[1], "relset") == 0)
  // {
  // 	// aled relset <pos> <relpos> <x> <y> <r> <g> <b>
  // 	ASSERT_ARGS(9);
  // 	Position_e pos = CharToPosEnum(argv[2][0]);
  // 	Position_e relPos = CharToPosEnum(argv[3][0]);
  // 	if (pos < NUM_SIDES && relPos < NUM_SIDES)
  // 	{
  // 		uint8_t x = atoi(argv[4]);
  // 		uint8_t y = atoi(argv[5]);
  // 		uint8_t r = atoi(argv[6]);
  // 		uint8_t g = atoi(argv[7]);
  // 		uint8_t b = atoi(argv[8]);
  // 		logprint("Setting pixel %s relative to %s %d %d to %d %d %d\n", AddrLedDriver_GetPositionString(pos), AddrLedDriver_GetPositionString(relPos),x, y, r, g, b);
  // 		Pixel_t *relPixel = AddrLedDriver_GetPixelInPanelRelative(pos, relPos, x, y);
  // 		AddrLedDriver_SetPixelRgb(relPixel, r, g, b);
  // 	}
  // }
  // else if (strcmp(argv[1], "set") == 0)
  // {
  // 	// aled set <pos> <x> <y> <r> <g> <b>
  // 	ASSERT_ARGS(8);
  // 	Position_e pos = NUM_SIDES;
  // 	if (strcmp(argv[2], "n") == 0)
  // 	{
  // 		pos = NORTH;
  // 	}
  // 	else if (strcmp(argv[2], "e") == 0)
  // 	{
  // 		pos = EAST;
  // 	}
  // 	else if (strcmp(argv[2], "s") == 0)
  // 	{
  // 		pos = SOUTH;
  // 	}
  // 	else if (strcmp(argv[2], "w") == 0)
  // 	{
  // 		pos = WEST;
  // 	}
  // 	else if (strcmp(argv[2], "t") == 0)
  // 	{
  // 		pos = TOP;
  // 	}
  // 	else
  // 	{
  // 		logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
  // 		return 1;
  // 	}
  // 	uint8_t x = atoi(argv[3]);
  // 	uint8_t y = atoi(argv[4]);
  // 	uint8_t r = atoi(argv[5]);
  // 	uint8_t g = atoi(argv[6]);
  // 	uint8_t b = atoi(argv[7]);
  // 	// logprint("Setting pixel %s %d %d to %d %d %d\n", AddrLedDriver_GetPositionString(pos), x, y, r, g, b);
  // 	AddrLedDriver_SetPixelRgbInPanel(pos, x, y, r, g, b);
  // }
  // else if (strcmp(argv[1], "nei") == 0)
  // {
  // 	// aled print <pos> <x> <y>
  // 	ASSERT_ARGS(5);
  // 	Position_e pos = NUM_SIDES;
  // 	if (strcmp(argv[2], "n") == 0)
  // 	{
  // 		pos = NORTH;
  // 	}
  // 	else if (strcmp(argv[2], "e") == 0)
  // 	{
  // 		pos = EAST;
  // 	}
  // 	else if (strcmp(argv[2], "s") == 0)
  // 	{
  // 		pos = SOUTH;
  // 	}
  // 	else if (strcmp(argv[2], "w") == 0)
  // 	{
  // 		pos = WEST;
  // 	}
  // 	else if (strcmp(argv[2], "t") == 0)
  // 	{
  // 		pos = TOP;
  // 	}
  // 	else
  // 	{
  // 		logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
  // 		return 1;
  // 	}
  // 	uint8_t x = atoi(argv[3]);
  // 	uint8_t y = atoi(argv[4]);
  // 	AddrLedDriver_Clear();
  // 	Pixel_t *p = AddrLedDriver_GetPixelInPanel(pos, x, y);
  // 	AddrLedDriver_SetPixelRgb(p, 10, 0, 0);
  // 	for (uint8_t i = 0; i < NUM_DIRECTIONS; i++)
  // 	{
  // 		if (p->neighborPixels[i] == NULL)
  // 		{
  // 			continue;
  // 		}
  // 		AddrLedDriver_SetPixelRgb(p->neighborPixels[i], 10, 0, 10);
  // 	}
  // }
  // else if (strcmp(argv[1], "print") == 0)
  // {
  // 	// aled print <pos> <x> <y>
  // 	ASSERT_ARGS(5);
  // 	Position_e pos = NUM_SIDES;
  // 	if (strcmp(argv[2], "n") == 0)
  // 	{
  // 		pos = NORTH;
  // 	}
  // 	else if (strcmp(argv[2], "e") == 0)
  // 	{
  // 		pos = EAST;
  // 	}
  // 	else if (strcmp(argv[2], "s") == 0)
  // 	{
  // 		pos = SOUTH;
  // 	}
  // 	else if (strcmp(argv[2], "w") == 0)
  // 	{
  // 		pos = WEST;
  // 	}
  // 	else if (strcmp(argv[2], "t") == 0)
  // 	{
  // 		pos = TOP;
  // 	}
  // 	else
  // 	{
  // 		logprint("BAD SIDE DESCRIPTOR! %s\n", argv[0]);
  // 		return 1;
  // 	}
  // 	uint8_t x = atoi(argv[3]);
  // 	uint8_t y = atoi(argv[4]);
  // 	AddrLedDriver_PrintPixel(AddrLedDriver_GetPixelInPanel(pos, x, y));
  // }
  return 0;
}

bool AddrLedDriver_IsInitialized(void)
{
  return addrLedDriverInitialized;
}
