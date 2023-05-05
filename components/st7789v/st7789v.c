#include "st7789v.h"

#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ST7789V_PIN_SCL (3)
#define ST7789V_PIN_SDA (8)
#define ST7789V_PIN_RES (18)
#define ST7789V_PIN_DC (17)
#define ST7789V_PIN_CS (16)
#define ST7789V_PIN_BLK (15)

#define ST7789V_SPI_HOST (SPI2_HOST)
#define ST7789V_SPI_SPEED_MHZ (80)

#define MAX_ROWS 60
#define MAX_TRANSFER_SIZE (ST7789V_HOR_RES * MAX_ROWS * 2)

/*The LCD needs a bunch of command/argument values to be initialized. They are
 * stored in this struct. */
typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes;  // No of data in data; bit 7 = delay after set; 0xFF = end
                      // of cmds.
} lcd_init_cmd_t;

static const char *TAG = "ST7789V";

static spi_device_handle_t spi;

static void st7789v_send_cmd(uint8_t cmd) {
  esp_err_t ret;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.tx_buffer = &cmd;
  t.user = (void *)0;
  ret = spi_device_polling_transmit(spi, &t);
  assert(ret == ESP_OK);
}

static void st7789v_send_data(uint8_t *data, uint16_t length) {
  esp_err_t ret;
  spi_transaction_t t;
  if (length == 0) return;   // no need to send anything
  memset(&t, 0, sizeof(t));  // Zero out the transaction
  t.length = length * 8;     // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;        // Data
  t.user = (void *)1;        // D/C needs to be set to 1
  ret = spi_device_polling_transmit(spi, &t);  // Transmit!
  assert(ret == ESP_OK);
}

static void st7789v_set_orientation(uint8_t orientation) {
  const char *orientation_str[] = {"PORTRAIT", "PORTRAIT_INVERTED", "LANDSCAPE",
                                   "LANDSCAPE_INVERTED"};
  ESP_LOGI(TAG, "Display orientation: %s", orientation_str[orientation]);
  uint8_t data[] = {0x00, 0xC0, 0x60, 0xA0};
  ESP_LOGI(TAG, "0x36 command value: 0x%02X", data[orientation]);
  st7789v_send_cmd(0x36);
  st7789v_send_data(&data[orientation], 1);
}

static void spi_pre_transfer_callback(spi_transaction_t *t) {
  int dc = (int)t->user;
  gpio_set_level(ST7789V_PIN_DC, dc);
}

static void st7789v_gpio_init(void) {
  esp_err_t ret;
  spi_bus_config_t buscfg = {.miso_io_num = -1,
                             .mosi_io_num = ST7789V_PIN_SDA,
                             .sclk_io_num = ST7789V_PIN_SCL,
                             .quadwp_io_num = -1,
                             .quadhd_io_num = -1,
                             .max_transfer_sz = MAX_TRANSFER_SIZE + 8};

  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = ST7789V_SPI_SPEED_MHZ * 1000 * 1000,
      .mode = 0,
      .spics_io_num = ST7789V_PIN_CS,
      .queue_size = 12,
      .pre_cb = spi_pre_transfer_callback,
  };

  ret = spi_bus_initialize(ST7789V_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(ST7789V_SPI_HOST, &devcfg, &spi);
  ESP_ERROR_CHECK(ret);

  gpio_config_t io_conf = {0};
  io_conf.pin_bit_mask = ((1ULL << ST7789V_PIN_DC) | (1ULL << ST7789V_PIN_RES) |
                          (1ULL << ST7789V_PIN_BLK));
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = true;
  gpio_config(&io_conf);

  spi_transaction_t trans;
  ESP_LOGW(TAG, "length=%d", sizeof(trans.length));
}

void st7789v_init(void) {
  st7789v_gpio_init();

  lcd_init_cmd_t st7789v_init_cmds[] = {
      {0x11, {0}, 0x08},

      {0x3A, {0x05}, 1},
      {0xC5, {0x1A}, 1},
      {0x36, {0x00}, 1},

      {0xB2, {0x05, 0x05, 0x00, 0x33, 0x33}, 5},
      {0xB7, {0x05}, 1},

      {0xBB, {0x3F}, 1},
      {0xC0, {0x2C}, 1},
      {0xC2, {0x01}, 1},
      {0xC3, {0x0F}, 1},
      {0xC4, {0x20}, 1},
      {0xC6, {0X01}, 1},  // Set to 0x28 if your display is flipped
      {0xD0, {0xa4, 0xa1}, 2},

      {0xE8, {0x03}, 1},
      {0xE9, {0x09, 0x09, 0x08}, 3},

      {0xE0,
       {0xD0, 0x05, 0x09, 0x09, 0x08, 0x14, 0x28, 0x33, 0x3F, 0x07, 0x13, 0x14,
        0x28, 0x30},
       14},
      {0xE1,
       {0xD0, 0x05, 0x09, 0x09, 0x08, 0x03, 0x24, 0x32, 0x32, 0x3B, 0x14, 0x13,
        0x28, 0x2F},
       14},

      {0x21, {0}, 0},
      {0x29, {0}, 0},

      {0, {0}, 0xff},
  };

  ESP_LOGI(TAG, "ST7789 initialization starts.");

  gpio_set_level(ST7789V_PIN_RES, 0);
  vTaskDelay(pdMS_TO_TICKS(200));
  gpio_set_level(ST7789V_PIN_RES, 1);
  vTaskDelay(pdMS_TO_TICKS(200));

  uint16_t cmd = 0;
  while (st7789v_init_cmds[cmd].databytes != 0xff) {
    st7789v_send_cmd(st7789v_init_cmds[cmd].cmd);
    st7789v_send_data(st7789v_init_cmds[cmd].data,
                      st7789v_init_cmds[cmd].databytes & 0x1F);
    if (st7789v_init_cmds[cmd].databytes & 0x80) {
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    cmd++;
  }
  st7789v_set_orientation(0);
  gpio_set_level(ST7789V_PIN_BLK, 1);
}
void st7789v_backlight_set(uint8_t brightness) {
  //
}
void st7789v_flush(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2,
                   void *color_map) {
  static uint32_t transfer_num = 0;
  static bool first_flush = true;
  if (first_flush) {
    first_flush = false;
  } else {
    spi_transaction_t *rtrans;
    for (int x = 0; x < transfer_num; x++) {
      esp_err_t ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
      if (ret != ESP_OK) {
        ESP_LOGW(TAG, "1. transfer_num = %d", transfer_num);
      }
      assert(ret == ESP_OK);
    }
    transfer_num = 0;
  }

  uint32_t data_len = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
  uint32_t data_count = data_len / MAX_TRANSFER_SIZE;
  uint32_t remain_size = data_len - data_count * MAX_TRANSFER_SIZE;
  uint32_t remain_rows = (y2 - y1 + 1) - MAX_ROWS * data_count;

  static spi_transaction_t trans[6][6];
  for (int i = 0; i < (remain_size ? data_count + 1 : data_count); i++) {
    for (int x = 0; x < 6; x++) {
      memset(&trans[i][x], 0, sizeof(spi_transaction_t));
      if ((x & 1) == 0) {
        // Even transfers are commands
        trans[i][x].length = 8;
        trans[i][x].user = (void *)0;
      } else {
        // Odd transfers are data
        trans[i][x].length = 8 * 4;
        trans[i][x].user = (void *)1;
      }
      trans[i][x].flags = SPI_TRANS_USE_TXDATA;
    }
  }

  y1 += 20;
  y2 += 20;

  for (int i = 0; i < (remain_size ? data_count + 1 : data_count); i++) {
    if (i < data_count) {
      trans[i][0].tx_data[0] = ST7789V_CASET;
      trans[i][1].tx_data[0] = (x1 >> 8) & 0xFF;  // Start Col High
      trans[i][1].tx_data[1] = (x1)&0xFF;         // Start Col Low
      trans[i][1].tx_data[2] = (x2 >> 8) & 0xFF;  // End Col High
      trans[i][1].tx_data[3] = (x2)&0xFF;         // End Col Low
      trans[i][2].tx_data[0] = ST7789V_RASET;     // Page address set
      trans[i][3].tx_data[0] = ((y1 + i * MAX_ROWS) >> 8) & 0xFF;
      trans[i][3].tx_data[1] = (y1 + i * MAX_ROWS) & 0xFF;  // start page low
      trans[i][3].tx_data[2] = ((y1 + (i + 1) * MAX_ROWS - 1) >> 8) & 0xFF;
      trans[i][3].tx_data[3] = (y1 + (i + 1) * MAX_ROWS - 1) & 0xFF;
      trans[i][4].tx_data[0] = ST7789V_RAMWR;  // memory write
    } else {
      trans[i][0].tx_data[0] = ST7789V_CASET;
      trans[i][1].tx_data[0] = (x1 >> 8) & 0xFF;  // Start Col High
      trans[i][1].tx_data[1] = (x1)&0xFF;         // Start Col Low
      trans[i][1].tx_data[2] = (x2 >> 8) & 0xFF;  // End Col High
      trans[i][1].tx_data[3] = (x2)&0xFF;         // End Col Low
      trans[i][2].tx_data[0] = ST7789V_RASET;     // Page address set
      trans[i][3].tx_data[0] = ((y1 + i * MAX_ROWS) >> 8) & 0xFF;
      trans[i][3].tx_data[1] = (y1 + i * MAX_ROWS) & 0xFF;  // start page low
      trans[i][3].tx_data[2] =
          ((y1 + i * MAX_ROWS + remain_rows - 1) >> 8) & 0xFF;
      trans[i][3].tx_data[3] = (y1 + i * MAX_ROWS + remain_rows - 1) & 0xFF;
      trans[i][4].tx_data[0] = ST7789V_RAMWR;  // memory write
    }
  }

  // trans[5].tx_buffer = (void *)color_map;  // finally send the color data
  // trans[5].length =
  //     (x2 - x1 + 1) * (y2 - y1 + 1) * 2 * 8;  // Data length, in bits
  // trans[5].flags = 0;                         // undo SPI_TRANS_USE_TXDATA
  // flag
  uint8_t *color_map_ptr = (uint8_t *)color_map;

  for (int i = 0; i < (remain_size ? data_count + 1 : data_count); i++) {
    if (i < data_count) {
      trans[i][5].tx_buffer = (void *)(color_map_ptr + i * MAX_TRANSFER_SIZE);
      trans[i][5].length = MAX_TRANSFER_SIZE * 8;
      trans[i][5].flags = 0;
    } else {
      trans[i][5].tx_buffer =
          (void *)(color_map_ptr + data_count * MAX_TRANSFER_SIZE);
      trans[i][5].length = remain_size * 8;
      trans[i][5].flags = 0;
    }
  }

  // uint8_t *color_map_ptr = (uint8_t *)color_map;
  // for (int i = 0; i < data_count; i++) {
  //   trans[5 + i].tx_buffer = (void *)(color_map_ptr + i * MAX_TRANSFER_SIZE);
  //   trans[5 + i].length = MAX_TRANSFER_SIZE * 8;
  //   trans[5 + i].flags = 0;
  // }

  // if (remain_size > 0) {
  //   trans[5 + data_count].tx_buffer =
  //       (void *)(color_map_ptr + data_count * MAX_TRANSFER_SIZE);
  //   trans[5 + data_count].length = remain_size * 8;
  //   trans[5 + data_count].flags = 0;
  //   transfer_num += 1;
  // }

  // Queue all transactions.
  for (int i = 0; i < (remain_size ? data_count + 1 : data_count); i++) {
    for (int x = 0; x < 6; x++) {
      esp_err_t ret = spi_device_queue_trans(spi, &trans[i][x], portMAX_DELAY);
      assert(ret == ESP_OK);
      transfer_num++;
    }
  }
}