#include "screen.h"

#include <Arduino.h>
#include <SPI.h>

void setup_tft(SPIClass &spi, Adafruit_ILI9341 &tft, int TFT_SLK, int TFT_MISO,
               int TFT_MOSI, int TFT_CS) {
  spi.begin(TFT_SLK, TFT_MISO, TFT_MOSI, TFT_CS);
  Serial.println("SPI init");

  Serial.println("TFT2");

  tft.begin();
  Serial.println("BEGIN");
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);

  tft.setCursor(250, 230);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK, CLEAR_COLOR);
  tft.print("CARACC 0.1");
}

void draw_skin(Adafruit_ILI9341 &tft) {
  tft.setTextSize(2);
  tft.setCursor(120, 10);
  tft.print("Max v");
  tft.drawLine(120, 25, 230, 25, ILI9341_BLUE);

  tft.setCursor(120, 30);
  tft.print("0-100");
  tft.setTextSize(1);
  tft.print("km/h");
  tft.setTextSize(2);
  tft.drawLine(120, 45, 230, 45, ILI9341_BLUE);

  tft.setCursor(120, 50);
  tft.print("80-100");
  tft.setTextSize(1);
  tft.print("km/h");
  tft.setTextSize(2);
  tft.drawLine(120, 65, 230, 65, ILI9341_BLUE);
  tft.setCursor(120, 70);
  tft.print("80-120");
  tft.setTextSize(1);
  tft.print("km/h");
  tft.setTextSize(2);
  tft.drawLine(120, 85, 230, 85, ILI9341_BLUE);
}

void draw_circle_acc(Adafruit_ILI9341 &tft) {
  tft.fillRect(160, 160, 30, 30, CLEAR_COLOR);
  tft.drawCircle(190, 190, 30, ILI9341_RED);
  tft.drawCircle(190, 190, 20, ILI9341_RED);
  tft.drawCircle(190, 190, 10, ILI9341_RED);
  tft.fillCircle(190, 190, 3, ILI9341_RED);
}
