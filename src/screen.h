#pragma once

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define CLEAR_COLOR ILI9341_WHITE

enum class Screen { kMain, kAccelerations };

void setup_tft(SPIClass &spi, Adafruit_ILI9341 &tft, int TFT_SLK, int TFT_MISO,
               int TFT_MOSI, int TFT_CS);

void draw_skin(Adafruit_ILI9341 &tft);

void draw_circle_acc(Adafruit_ILI9341 &tft);
