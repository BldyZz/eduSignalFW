#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "lvgl.h"

#include <cstdint>
#include <ranges>
#include <span>

extern "C" {
extern void example_lvgl_demo_ui(lv_disp_t* disp);
}

static bool example_notify_lvgl_flush_ready(
  esp_lcd_panel_io_handle_t      panel_io,
  esp_lcd_panel_io_event_data_t* edata,
  void*                          user_ctx) {
    lv_disp_drv_t* disp_driver = (lv_disp_drv_t*)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void
example_lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int                    offsetx1     = area->x1;
    int                    offsetx2     = area->x2;
    int                    offsety1     = area->y1;
    int                    offsety2     = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(
      panel_handle,
      offsetx1,
      offsety1,
      offsetx2 + 1,
      offsety2 + 1,
      color_map);
}

static void example_lvgl_port_update_callback(lv_disp_drv_t* drv) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

    switch(drv->rotated) {
    case LV_DISP_ROT_NONE:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

static void example_increase_lvgl_tick(void* arg) {
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(2);
}

template<typename displayConfig>
struct Display {
    static constexpr auto                  numberOfPoints{100};
    std::array<lv_point_t, numberOfPoints> oxiRDDisplayData;
    std::array<lv_point_t, numberOfPoints> oxiIRDDisplayData;
    std::array<std::uint32_t , numberOfPoints>      oxiRDData;
    std::array<std::uint32_t, numberOfPoints>      oxiIRDData;
    std::size_t                            oxiDataPointer{};
    lv_obj_t*                              oxiRDLine;
    lv_obj_t*                              oxiIRDLine;

    Display() {
        static lv_disp_draw_buf_t
          disp_buf;   // contains internal graphic buffer(s) called draw buffer(s)
        static lv_disp_drv_t disp_drv;   // contains callback functions

        PRINTI("[Display:]", "Turn off LCD backlight\n");
        gpio_config_t bk_gpio_config{};
        bk_gpio_config.mode         = GPIO_MODE_OUTPUT;
        bk_gpio_config.pin_bit_mask = 1ULL << displayConfig::BACKLIGHTPin;
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

        PRINTI("[Display:]", "Initialize SPI bus\n");
        spi_bus_config_t buscfg{};
        buscfg.sclk_io_num     = displayConfig::SPIConfig::SCK;
        buscfg.mosi_io_num     = displayConfig::SPIConfig::MOSI;
        buscfg.miso_io_num     = displayConfig::SPIConfig::MISO;
        buscfg.quadwp_io_num   = -1;
        buscfg.quadhd_io_num   = -1;
        buscfg.max_transfer_sz = displayConfig::displayHeight * 80 * sizeof(uint16_t);
        ESP_ERROR_CHECK(
          spi_bus_initialize(displayConfig::SPIConfig::SPIHost, &buscfg, SPI_DMA_CH_AUTO));

        PRINTI("[Display:]", "Install panel IO\n");
        esp_lcd_panel_io_handle_t     io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config{};
        io_config.dc_gpio_num         = displayConfig::DCPin;
        io_config.cs_gpio_num         = displayConfig::CSPin;
        io_config.pclk_hz             = 20 * 1000 * 1000;
        io_config.lcd_cmd_bits        = 8;
        io_config.lcd_param_bits      = 8;
        io_config.spi_mode            = 0;
        io_config.trans_queue_depth   = 10;
        io_config.on_color_trans_done = example_notify_lvgl_flush_ready;
        io_config.user_ctx            = &disp_drv;
        // Attach the LCD to the SPI bus
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
          (esp_lcd_spi_bus_handle_t)displayConfig::SPIConfig::SPIHost,
          &io_config,
          &io_handle));

        PRINTI("[Display:]", "Install GC9A01 panel driver\n");
        esp_lcd_panel_handle_t     panel_handle = nullptr;
        esp_lcd_panel_dev_config_t panel_config{};
        panel_config.reset_gpio_num = displayConfig::RESETPin;
        panel_config.rgb_endian     = LCD_RGB_ENDIAN_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

        // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

        PRINTI("[Display:]", "Turn on LCD backlight\n");
        gpio_set_level(displayConfig::BACKLIGHTPin, 1);

        PRINTI("[Display:]", "Initialize LVGL library\n");
        lv_init();
        // alloc draw buffers used by LVGL
        // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
        lv_color_t* buf1 = reinterpret_cast<lv_color_t*>(
          heap_caps_malloc(displayConfig::displayHeight * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA));
        assert(buf1);
        lv_color_t* buf2 = reinterpret_cast<lv_color_t*>(
          heap_caps_malloc(displayConfig::displayHeight * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA));
        assert(buf2);
        // initialize LVGL draw buffers
        lv_disp_draw_buf_init(&disp_buf, buf1, buf2, displayConfig::displayHeight * 20);

        PRINTI("[Display:]", "Register display driver to LVGL\n");
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res       = displayConfig::displayHeight;
        disp_drv.ver_res       = displayConfig::displayWidth;
        disp_drv.flush_cb      = example_lvgl_flush_cb;
        disp_drv.drv_update_cb = example_lvgl_port_update_callback;
        disp_drv.draw_buf      = &disp_buf;
        disp_drv.user_data     = panel_handle;
        lv_disp_t* disp        = lv_disp_drv_register(&disp_drv);

        PRINTI("[Display:]", "Install LVGL tick timer\n");
        // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
        esp_timer_create_args_t lvgl_tick_timer_args{};
        lvgl_tick_timer_args.callback      = &example_increase_lvgl_tick;
        lvgl_tick_timer_args.name          = "lvgl_tick";
        esp_timer_handle_t lvgl_tick_timer = nullptr;
        ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));
        lv_disp_set_rotation(disp, LV_DISP_ROT_90);
        PRINTI("[Display:]", "Display Widget\n");
        //example_lvgl_demo_ui(disp);
        displayWidget(disp);
    }

    void displayWidget(lv_disp_t* disp) {
        static lv_style_t style_IRD;
        lv_style_init(&style_IRD);
        lv_style_set_line_width(&style_IRD, 5);
        lv_style_set_line_color(&style_IRD, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_line_rounded(&style_IRD, true);

        static lv_style_t style_RD;
        lv_style_init(&style_RD);
        lv_style_set_line_width(&style_RD, 5);
        lv_style_set_line_color(&style_RD, lv_palette_main(LV_PALETTE_RED));
        lv_style_set_line_rounded(&style_RD, true);

        oxiRDLine = lv_line_create(lv_scr_act());
        lv_obj_add_style(oxiRDLine, &style_RD, LV_PART_MAIN);
        lv_obj_align(oxiRDLine, LV_ALIGN_TOP_LEFT, 0, 0);

        oxiIRDLine = lv_line_create(lv_scr_act());
        lv_obj_add_style(oxiIRDLine, &style_IRD, LV_PART_MAIN);
        lv_obj_align(oxiIRDLine, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        lv_obj_t * label1 = lv_label_create(lv_scr_act());
        lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
        lv_label_set_recolor(label1, true);                      /*Enable re-coloring by commands in the text*/
        lv_label_set_text(label1, "#0000ff IRD#");
        lv_obj_align(label1, LV_ALIGN_BOTTOM_RIGHT,0,0);

        lv_obj_t * label2 = lv_label_create(lv_scr_act());
        lv_label_set_long_mode(label2, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
        lv_label_set_recolor(label2, true);                      /*Enable re-coloring by commands in the text*/
        lv_label_set_text(label2, "#ff0000 RD#");
        lv_obj_align(label2, LV_ALIGN_TOP_RIGHT,0,0);
    }

    void setOxiValues(std::uint32_t valueRD, std::uint32_t valueIRD) {
        if(oxiDataPointer > oxiIRDData.size()) {
            oxiDataPointer = 0;
        }
        oxiIRDData[oxiDataPointer] = valueIRD;
        oxiRDData[oxiDataPointer]  = valueRD;

        auto minimumRD      = std::ranges::min(oxiRDData);
        auto maximumRD      = std::ranges::max(oxiRDData);
        auto minimumIRD      = std::ranges::min(oxiIRDData);
        auto maximumIRD      = std::ranges::max(oxiIRDData);
        assert((maximumRD - minimumRD) != 0);
        assert((maximumIRD - minimumIRD) != 0);

        auto displayValueRD = ((static_cast<double>(valueRD - minimumRD)) / (maximumRD - minimumRD)) * 80;
        auto displayValueIRD = ((static_cast<double>(valueIRD - minimumIRD)) / (maximumIRD - minimumIRD)) * 80;
        oxiIRDDisplayData[oxiDataPointer] = lv_point_t{oxiDataPointer * 3, displayValueIRD};
        oxiRDDisplayData[oxiDataPointer] = lv_point_t{oxiDataPointer * 3, displayValueRD};
        ++oxiDataPointer;
    }

    void handler() {
        lv_line_set_points(oxiRDLine, oxiRDDisplayData.data(), oxiRDDisplayData.size());
        lv_line_set_points(oxiIRDLine, oxiIRDDisplayData.data(), oxiIRDDisplayData.size());
        lv_timer_handler();
    }
};