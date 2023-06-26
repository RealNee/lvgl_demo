# 说明
  * 屏幕驱动仅支持ST7789V 240*280
  * 主控：ESP32S3
  * IDF版本：v4.4

# 设置

```bash
idf.py menuconfig

Demo Configuration  --->
    Graphics config  ---> #图形缓冲区大小，是否使用SPIRAM作为图形缓冲
    ST7789V config  --->  #管脚、SPI速度、屏幕方向
```

# Q&A
* 没有看到SPIRAM作为图形缓冲的选项？
  ```
  需要先使能系统的SPIRAM
  ```

* 编译报错：未找到`lv_demo_benchmark();`
  ```
  在lvgl设置中使能Demo中的benchmark
  ```

* 极致跑分怎么得到？
  ```bash
  1.使用内部RAM作为图形缓冲
  2.图形缓冲区尽可能设置大一点
  3.主函数中的Delay尽可能小
  4.LVGL硬件设置中刷新时间尽可能小 #Default display refresh period
  5.[系统]编译优化等级-O2
  6.[LVGL]编译器设置中勾选 Set IRAM as LV_ATTRIBUTE_FAST_MEM
  7.开启 lv_demo_benchmark_set_max_speed(true);
  8.flash设置为QIO
  9.CPU主频设置为最大
  ```
