#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lcd_ili9341.h"


#include "devices/ADS1299.hpp"
#include "devices/BHI160.hpp"
#include "devices/MAX30102.hpp"
#include "devices/MCP3561.hpp"
#include "devices/PCF8574.hpp"
#include "devices/TSC2003.hpp"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_util/i2cMaster.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display/display.hpp"
#include "display/displayConfig.hpp"
#include "lvgl.h"

#include "esp_util/i2cDevice.hpp"
#include "i2cConfig.hpp"
#include "spiConfig.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <thread>

extern "C" {
extern void example_lvgl_demo_ui(lv_disp_t *disp);
}

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    switch (drv->rotated) {
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

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(2);
}

extern "C" void app_main() {
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    fmt::print("Turn off LCD backlight\n");
    gpio_config_t bk_gpio_config{};
            bk_gpio_config.mode = GPIO_MODE_OUTPUT;
            bk_gpio_config.pin_bit_mask = 1ULL << displayConfig::BACKLIGHTPin;
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    fmt::print("Initialize SPI bus\n");
    spi_bus_config_t buscfg {};
    buscfg.sclk_io_num = displayConfig::SPIConfig::SCK;
            buscfg.mosi_io_num = displayConfig::SPIConfig::MOSI;
            buscfg.miso_io_num = displayConfig::SPIConfig::MISO;
            buscfg.quadwp_io_num = -1;
            buscfg.quadhd_io_num = -1;
            buscfg.max_transfer_sz = displayConfig::displayHeight * 80 * sizeof(uint16_t);
    ESP_ERROR_CHECK(spi_bus_initialize(displayConfig::SPIConfig::SPIHost, &buscfg, SPI_DMA_CH_AUTO));

    fmt::print("Install panel IO\n");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config {};
            io_config.dc_gpio_num = displayConfig::DCPin;
            io_config.cs_gpio_num = displayConfig::CSPin;
            io_config.pclk_hz = 20*1000*1000;
            io_config.lcd_cmd_bits = 8;
            io_config.lcd_param_bits = 8;
            io_config.spi_mode = 0;
            io_config.trans_queue_depth = 10;
            io_config.on_color_trans_done = example_notify_lvgl_flush_ready;
            io_config.user_ctx = &disp_drv;
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)displayConfig::SPIConfig::SPIHost, &io_config, &io_handle));

    fmt::print("Install GC9A01 panel driver\n");
    esp_lcd_panel_handle_t panel_handle = nullptr;
    esp_lcd_panel_dev_config_t panel_config{};
            panel_config.reset_gpio_num = displayConfig::RESETPin;
            panel_config.rgb_endian = LCD_RGB_ENDIAN_RGB;
            panel_config.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));


    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    fmt::print("Turn on LCD backlight\n");
    gpio_set_level(displayConfig::BACKLIGHTPin, 1);

    fmt::print("Initialize LVGL library\n");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = reinterpret_cast<lv_color_t*>(heap_caps_malloc(displayConfig::displayHeight * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA));
    assert(buf1);
    lv_color_t *buf2 = reinterpret_cast<lv_color_t*>(heap_caps_malloc(displayConfig::displayHeight  * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA));
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, displayConfig::displayHeight * 20);

    fmt::print("Register display driver to LVGL\n");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = displayConfig::displayHeight;
    disp_drv.ver_res = displayConfig::displayWidth;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    fmt::print("Install LVGL tick timer\n");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    esp_timer_create_args_t lvgl_tick_timer_args {};
    lvgl_tick_timer_args.callback = &example_increase_lvgl_tick;
            lvgl_tick_timer_args.name = "lvgl_tick";
    esp_timer_handle_t lvgl_tick_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));

    fmt::print("Display LVGL Meter Widget\nś");
    example_lvgl_demo_ui(disp);

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }

    esp::i2cMaster<I2C0_Config>      boardI2C;
    //BHI160<I2C0_Config, GPIO_NUM_39> imu;
    //MAX30102<I2C0_Config>            pulseOxiMeter;
    //PCF8574<I2C0_Config>             ioExpander;
    //TSC2003<I2C0_Config, GPIO_NUM_2> touchScreenController;

    esp::spiHost<displayConfig::SPIConfig> displaySPI;
    Display<displayConfig> display{displaySPI};

    esp::spiHost<BoardSPIConfig>                                                boardSPI;
    //ADS1299<BoardSPIConfig, 4, GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_0, GPIO_NUM_36> ecg{boardSPI};
    //MCP3561<BoardSPIConfig, 8, GPIO_NUM_33, GPIO_NUM_34>                        adc{boardSPI};

    while(1) {
        auto now = std::chrono::system_clock::now();


        /*
      if(ecg.noiseData.has_value()){
            //fmt::print("Noise Data: {:#032b}\n",ecg.noiseData.value()[0]);
            fmt::print("Noise Data: {}\n",ecg.noiseData.value()[0]);
            ecg.noiseData = {};
        }

        if(ecg.ecgData.has_value()){
            //fmt::print("{} {:#034b}\n", std::chrono::steady_clock::now().time_since_epoch() ,ecg.ecgData.value()[0]);
            fmt::print("Data: {} {}\n", std::chrono::steady_clock::now().time_since_epoch() ,fmt::join(ecg.ecgData.value(), ", "));
            ecg.ecgData = {};
        }

        if(pulseOxiMeter.IRDValue.has_value() && pulseOxiMeter.RDValue.has_value()){
            fmt::print("Data: {} {} {}\n",std::chrono::steady_clock::now().time_since_epoch() , pulseOxiMeter.IRDValue.value(), pulseOxiMeter.RDValue.value());
            pulseOxiMeter.IRDValue = {};
            pulseOxiMeter.RDValue = {};
        }



        if(imu.accData.X.has_value() && imu.accData.Y.has_value() && imu.accData.Z.has_value()){
            fmt::print("IMU: {:05}, {:05}, {:05}\n", imu.accData.X.value(),imu.accData.Y.value(),imu.accData.Z.value());
            imu.accData.X = {};
            imu.accData.Y = {};
            imu.accData.Z = {};
        }
*/
        //ecg.handler();
        //adc.handler();

        //ioExpander.handler();
        //imu.handler();
        //pulseOxiMeter.handler();
        //touchScreenController.handler();
        //display.handler();
        //display.buffer.setPixel(10,12,Pixel{255,0,0});
        display.flush();
    }

    return;
}
