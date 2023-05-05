/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable
 * content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"

#include <stdbool.h>

#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "st7789v.h"
/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES ST7789V_HOR_RES
#define MY_DISP_VER_RES ST7789V_VER_RES

#define DISP_BUF_SIZE (MY_DISP_HOR_RES * 40)
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                       lv_color_t *color_p);
// static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf,
// lv_coord_t dest_width,
//         const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_disp_draw_buf_t draw_buf_dsc;
static lv_color_t buf_1[DISP_BUF_SIZE];
static lv_color_t buf_2[DISP_BUF_SIZE];
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void) {
  /*-------------------------
   * Initialize your display
   * -----------------------*/
  disp_init();

  /*-----------------------------
   * Create a buffer for drawing
   *----------------------------*/

  /**
   * LVGL requires a buffer where it internally draws the widgets.
   * Later this buffer will passed to your display driver's `flush_cb` to copy
   * its content to your display. The buffer has to be greater than 1 display
   * row
   *
   * There are 3 buffering configurations:
   * 1. Create ONE buffer:
   *      LVGL will draw the display's content here and writes it to your
   * display
   *
   * 2. Create TWO buffer:
   *      LVGL will draw the display's content to a buffer and writes it your
   * display. You should use DMA to write the buffer's content to the display.
   *      It will enable LVGL to draw the next part of the screen to the other
   * buffer while the data is being sent form the first buffer. It makes
   * rendering and flushing parallel.
   *
   * 3. Double buffering
   *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
   *      This way LVGL will always provide the whole rendered screen in
   * `flush_cb` and you only need to change the frame buffer's address.
   */
  //lv_color_t *buf_1 = heap_caps_malloc(DISP_BUF_SIZE, MALLOC_CAP_SPIRAM);
  //lv_color_t *buf_2 = heap_caps_malloc(DISP_BUF_SIZE, MALLOC_CAP_SPIRAM);

  lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, buf_2,
                        DISP_BUF_SIZE); /*Initialize the display buffer*/

  /*-----------------------------------
   * Register the display in LVGL
   *----------------------------------*/

  static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
  lv_disp_drv_init(&disp_drv);   /*Basic initialization*/

  /*Set up the functions to access to your display*/

  /*Set the resolution of the display*/
  disp_drv.hor_res = MY_DISP_HOR_RES;
  disp_drv.ver_res = MY_DISP_VER_RES;

  /*Used to copy the buffer's content to the display*/
  disp_drv.flush_cb = disp_flush;

  /*Set a display buffer*/
  disp_drv.draw_buf = &draw_buf_dsc;

  /*Required for Example 3)*/
  //disp_drv.full_refresh = 1;

  /* Fill a memory array with a color if you have GPU.
   * Note that, in lv_conf.h you can enable GPUs that has built-in support in
   * LVGL. But if you have a different GPU you can use with this callback.*/
  // disp_drv.gpu_fill_cb = gpu_fill;

  /*Finally register the driver*/
  lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void) { st7789v_init(); }

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called
 * by LVGL
 */
void disp_enable_update(void) { disp_flush_enabled = true; }

/* Disable updating the screen (the flushing process) when disp_flush() is
 * called by LVGL
 */
void disp_disable_update(void) { disp_flush_enabled = false; }

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the
 *background but 'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                       lv_color_t *color_p) {
  if (disp_flush_enabled) {
    /*The most simple case (but also the slowest) to put all pixels to the
     * screen one-by-one*/
    st7789v_flush(area->x1, area->x2, area->y1, area->y2, (void *)color_p);
  }

  /*IMPORTANT!!!
   *Inform the graphics library that you are ready with the flushing*/
  lv_disp_flush_ready(disp_drv);
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a
 * memory with a color*/
// static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf,
// lv_coord_t dest_width,
//                     const lv_area_t * fill_area, lv_color_t color)
//{
//     /*It's an example code which should be done by your GPU*/
//     int32_t x, y;
//     dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//     for(y = fill_area->y1; y <= fill_area->y2; y++) {
//         for(x = fill_area->x1; x <= fill_area->x2; x++) {
//             dest_buf[x] = color;
//         }
//         dest_buf+=dest_width;    /*Go to the next line*/
//     }
// }

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
