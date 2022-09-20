//
// Created by patrick on 5/29/22.
//
#pragma once
#include <cstddef>
#include <vector>

//TODO: Change vector to array with template

struct lcdCommandType {
    std::byte              cmd;
    std::vector<std::byte> data;
    bool                   waitDelay;
};

DRAM_ATTR static const std::array lcdInitCommmands{
lcdCommandType{std::byte{0xCF}, {std::byte{0x00}, std::byte{0x83}, std::byte{0x30}}, false},
lcdCommandType{std::byte{0xED}, {std::byte{0x64}, std::byte{0x03}, std::byte{0x12}, std::byte{0x81}}, false},
lcdCommandType{std::byte{0xE8}, {std::byte{0x85}, std::byte{0x01}, std::byte{0x79}}, false},
lcdCommandType{std::byte{0xCB},
   {std::byte{0x39}, std::byte{0x2C}, std::byte{0x00}, std::byte{0x34}, std::byte{0x02}},
   false},
  /* Pump ratio control, DDVDH=2xVCl */
  lcdCommandType{std::byte{0xF7}, {std::byte{0x20}}, false},
  /* Driver timing control, all=0 unit */
  lcdCommandType{std::byte{0xEA}, {std::byte{0x00}, std::byte{0x00}}, false},
  /* Power control 1, GVDD=4.75V */
  lcdCommandType{std::byte{0xC0}, {std::byte{0x26}}, false},
  /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
  lcdCommandType{std::byte{0xC1}, {std::byte{0x11}}, false},
  /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
  lcdCommandType{std::byte{0xC5}, {std::byte{0x35}, std::byte{0x3E}}, false},
  /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
  lcdCommandType{std::byte{0xC7}, {std::byte{0xBE}}, false},
  /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
  lcdCommandType{std::byte{0x36}, {std::byte{0x28}}, false},
  /* Pixel format, 16bits/pixel for RGB/MCU interface */
  lcdCommandType{std::byte{0x3A}, {std::byte{0x55}}, false},
  /* Frame rate control, f=fosc, 70Hz fps */
  lcdCommandType{std::byte{0xB1}, {std::byte{0x00}, std::byte{0x1B}}, false},
  /* Enable 3G, disabled */
  lcdCommandType{std::byte{0xF2}, {std::byte{0x08}}, false},
  /* Gamma set, curve 1 */
  lcdCommandType{std::byte{0x26}, {std::byte{0x01}}, false},
  /* Positive gamma correction */
  lcdCommandType{std::byte{0xE0},
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
  lcdCommandType{std::byte{0XE1},
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
  lcdCommandType{std::byte{0x2A}, {std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0xEF}}, false},
  /* Page address set, SP=0, EP=0x013F */
  lcdCommandType{std::byte{0x2B}, {std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x3f}}, false},
  /* Memory write */
  lcdCommandType{std::byte{0x2C}, {}, false},
  /* Entry mode set, Low vol detect disabled, normal display */
  lcdCommandType{std::byte{0xB7}, {std::byte{0x07}}, false},
  /* Display function control */
  lcdCommandType{std::byte{0xB6}, {std::byte{0x0A}, std::byte{0x82}, std::byte{0x27}, std::byte{0x00}}, false},
  /* Sleep out */
  lcdCommandType{std::byte{0x11}, {}, true},
  /* Display on */
  lcdCommandType{std::byte{0x29}, {}, true}};