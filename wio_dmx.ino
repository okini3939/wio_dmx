/*
 * DMX Monitor
 * 
 * Board: Seeeduino Wio Terminal
 *
 * Library:
 *    https://github.com/lovyan03/LovyanGFX
 */

#include "SAMD51_TC.h"
#include "DMX.h"

//#include "TFT_eSPI.h"
//TFT_eSPI tft;

#define LGFX_WIO_TERMINAL
#define LGFX_USE_V1
#include "LovyanGFX.h"
#include "LGFX_AUTODETECT.hpp"

static LGFX tft;
DMX dmx;

volatile int timeout = 0;
int mode = 0, page = 0;
char rdm_uid[] = {0x7f, 0xf7, 0x00, 0x12, 0x34, 0x56};
unsigned char rdm_uid_buf[20][6];


void wait_us (int us) {
  delayMicroseconds(us);
}

void draw_dmx (int offset, int refresh) {
  int i, d, n, x, y, z;

  tft.startWrite();
  tft.setTextDatum(textdatum_t::top_left);

  if (refresh) {
    tft.fillScreen(TFT_BLACK);
    tft.setFont(&fonts::Font2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(4, 4);
    tft.printf("DMX Monitor");

    for (i = 1; i < 10; i ++) {
      x = 32 * i;
      tft.drawLine(x, 30, x, 239, TFT_DARKGREY);
    }
    for (i = 0; i < 7; i ++) {
      y = 30 + 30 * i;
      tft.drawLine(0, y, 319, y, TFT_DARKGREY);
      if (i < 10) {
      }
    }
  }

  for (i = 0; i < 70; i ++) {
    n = offset + i;
    x = 32 * (i % 10);
    y = 30 + 30 * (i / 10);
    if (n < 512) {
      d = dmx.get(n);

      if (refresh) {
        tft.setFont(&fonts::Font0);
        tft.setTextColor(TFT_GREENYELLOW);
        tft.setCursor(x + 2, y + 2);
        tft.printf("%d", n);
      } else {
        tft.fillRect(x + 1, y + 12, 31, 17, TFT_BLACK);
      }
      tft.setFont(&fonts::Font2);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(x + 5, y + 12);
      tft.printf("%3d", d);
      z = (d * 31) / 255;
      if (d > 0) {
        tft.fillRect(x + 1, y + 27, z, 2, TFT_GREEN);
      }
    }
  }

  tft.endWrite();
}

void setup() {

  Serial.begin(115200);
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  tft.begin();
  tft.setRotation(1); // 320 x 240
  tft.fillScreen(TFT_BLACK);
  tft.drawString("DMX", 10, 10);

  dmx.init();

  draw_dmx(0, 1);
}

void loop() {

  if (dmx.isReceived()) {
    digitalWrite(PIN_LED, HIGH);
    draw_dmx(page * 10, 0);
    digitalWrite(PIN_LED, LOW);
  }

  if ((digitalRead(WIO_5S_UP) == LOW) && (page > 1)) {
    page -= 3;
    if (page < 0) page = 0;
    draw_dmx(page * 10, 1);
    while (digitalRead(WIO_5S_UP) == LOW);
  }
  if ((digitalRead(WIO_5S_DOWN) == LOW) && (page < 45)) {
    page += 3;
    if (page > 45) page = 45;
    draw_dmx(page * 10, 1);
    while (digitalRead(WIO_5S_DOWN) == LOW);
  }

  delay(10);
}
