#pragma once

#include "esp_util/spiDevice.hpp"
#include "esp_util/spiHost.hpp"
#include "freertos/task.h"
#include "pixel.h"

#include <fmt/format.h>
#include <vector>

struct lcdCommandType {
    std::byte              cmd;
    std::vector<std::byte> data;
    bool                   waitDelay;
};

DRAM_ATTR static const std::vector<lcdCommandType> lcdInitCommmands{
  {std::byte{0xCF}, {std::byte{0x00}, std::byte{0x83}, std::byte{0x30}}, false},
  {std::byte{0xED}, {std::byte{0x64}, std::byte{0x03}, std::byte{0x12}, std::byte{0x81}}, false},
  {std::byte{0xE8}, {std::byte{0x85}, std::byte{0x01}, std::byte{0x79}}, false},
  {std::byte{0xCB},
   {std::byte{0x39}, std::byte{0x2C}, std::byte{0x00}, std::byte{0x34}, std::byte{0x02}},
   false},
  /* Pump ratio control, DDVDH=2xVCl */
  {std::byte{0xF7}, {std::byte{0x20}}, false},
  /* Driver timing control, all=0 unit */
  {std::byte{0xEA}, {std::byte{0x00}, std::byte{0x00}}, false},
  /* Power control 1, GVDD=4.75V */
  {std::byte{0xC0}, {std::byte{0x26}}, false},
  /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
  {std::byte{0xC1}, {std::byte{0x11}}, false},
  /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
  {std::byte{0xC5}, {std::byte{0x35}, std::byte{0x3E}}, false},
  /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
  {std::byte{0xC7}, {std::byte{0xBE}}, false},
  /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
  {std::byte{0x36}, {std::byte{0x28}}, false},
  /* Pixel format, 16bits/pixel for RGB/MCU interface */
  {std::byte{0x3A}, {std::byte{0x55}}, false},
  /* Frame rate control, f=fosc, 70Hz fps */
  {std::byte{0xB1}, {std::byte{0x00}, std::byte{0x1B}}, false},
  /* Enable 3G, disabled */
  {std::byte{0xF2}, {std::byte{0x08}}, false},
  /* Gamma set, curve 1 */
  {std::byte{0x26}, {std::byte{0x01}}, false},
  /* Positive gamma correction */
  {std::byte{0xE0},
   {std::byte{0x1F},
    std::byte{0x1A},
    std::byte{0x18},
    std::byte{0x0A},
    std::byte{0x0F},
    std::byte{0x06},
    std::byte{0x45},
    std::byte{0X87},
    std::byte{0x32},
    std::byte{0x0A},
    std::byte{0x07},
    std::byte{0x02},
    std::byte{0x07},
    std::byte{0x05},
    std::byte{0x00}},
   false},
  /* Negative gamma correction */
  {std::byte{0XE1},
   {std::byte{0x00},
    std::byte{0x25},
    std::byte{0x27},
    std::byte{0x05},
    std::byte{0x10},
    std::byte{0x09},
    std::byte{0x3A},
    std::byte{0x78},
    std::byte{0x4D},
    std::byte{0x05},
    std::byte{0x18},
    std::byte{0x0D},
    std::byte{0x38},
    std::byte{0x3A},
    std::byte{0x1F}},
   false},
  /* Column address set, SC=0, EC=0xEF */
  {std::byte{0x2A}, {std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xEF}}, false},
  /* Page address set, SP=0, EP=0x013F */
  {std::byte{0x2B}, {std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x3f}}, false},
  /* Memory write */
  {std::byte{0x2C}, {}, false},
  /* Entry mode set, Low vol detect disabled, normal display */
  {std::byte{0xB7}, {std::byte{0x07}}, false},
  /* Display function control */
  {std::byte{0xB6}, {std::byte{0x0A}, std::byte{0x82}, std::byte{0x27}, std::byte{0x00}}, false},
  /* Sleep out */
  {std::byte{0x11}, {}, true},
  /* Display on */
  {std::byte{0x29}, {}, true}};

struct Display : private esp::spiDevice {
private:
    gpio_num_t DC;
    gpio_num_t RST;
    gpio_num_t BACKL;
    DRAM_ATTR static const std::array<std::array<Pixel, 420*320>,2> pixelBuffer;

public:
    explicit Display(
      esp::spiHost const& bus,
      gpio_num_t          CS,
      gpio_num_t          DC,
      gpio_num_t          RST,
      gpio_num_t          BACKL)
      : esp::spiDevice(bus, 10 * 1000 * 1000, CS, 7, 0)
      , DC{DC}
      , RST{RST}
      , BACKL{BACKL} {
        fmt::print("Initializing Display...\n");
        gpio_set_direction(DC, GPIO_MODE_OUTPUT);
        gpio_set_direction(RST, GPIO_MODE_OUTPUT);
        gpio_set_direction(BACKL, GPIO_MODE_OUTPUT);
        reset();
    }

    void sendData(std::vector<std::byte> const& data) {
        gpio_set_level(DC, 1);
        sendBlocking(data);
        gpio_set_level(DC, 0);
    }

    void sendCommand(std::byte const& data) {
        gpio_set_level(DC, 0);
        std::vector<std::byte> dataVec{data};
        sendBlocking(dataVec);
    }

    void reset() {
        fmt::print("Resetting Display...\n");
        gpio_set_level(RST, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(RST, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        sendConfig();
    }

    void sendConfig() {
        for(auto c : lcdInitCommmands) {
            sendCommand(c.cmd);
            sendData(c.data);
            if(c.waitDelay) {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
        gpio_set_level(BACKL, 1);
        fmt::print("Display is fully configured!\n");
    }
};