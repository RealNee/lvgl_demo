#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lv_demos.h"
#include "lv_port_disp.h"
#include "lvgl.h"
#include "st7789v.h"

static void lv_tick_task(void *arg) {
  (void)arg;

  lv_tick_inc(1);
}

void app_main(void) {
  printf("hello world\n");
  lv_init();
  lv_port_disp_init();
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &lv_tick_task, .name = "periodic_gui"};
  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(
      esp_timer_start_periodic(periodic_timer, 1 * 1000));
  //lv_demo_benchmark_set_max_speed(true);
  lv_demo_benchmark();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10));
    lv_timer_handler();
  }
}
