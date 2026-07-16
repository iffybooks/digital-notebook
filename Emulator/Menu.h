#ifndef Menu_H_
#define Menu_H_

#ifdef EMULATE
#define SSD1306_WHITE 1
#define SSD1306_BLACK 2
#include "AdafruitStub.h"
#include "SDStub.h"
#else
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

namespace dnb {
  const int MENU_W = 12;
  const int MENU_H = 4;
  const int NUM_OPTIONS = 2;
  const char MENU[4][13] = { // +1 is for trailing `\0'
    " __________ ",
    "|  NEW     |",
    "|  SAVE    |",
    "|__________|"
  };

  class Menu {
    public:
      Menu() : mSelected(0) {}
      ~Menu() {}
      void render(Adafruit_SSD1306& display, int width, int height) const; 
      void processChar(uint8_t ascii, uint8_t keycode);
      int getOption() const { return mSelected; }
  
    private:
      int mSelected;
  };

  void Menu::processChar(uint8_t ascii, uint8_t keycode) {
    if (keycode == 0x51) { // down
      mSelected = (mSelected + 1) % NUM_OPTIONS;
    }
    else if (keycode == 0x52) {  // up
      mSelected = mSelected - 1 < 0? NUM_OPTIONS : mSelected - 1;
    }
  }

  void Menu::render(Adafruit_SSD1306& display, int width, int height) const {
    display.clearDisplay();
    display.setTextSize(1);  // Normal 1:1 pixel scale

    int sr = (height - MENU_H)/2;
    int sc = (width - MENU_W)/2;
    for (int row = 0; row < MENU_H; row++) {
      for (int col = 0; col < MENU_W; col++) {
        char c = MENU[row][col];
        if (mSelected+1 == row && c >= 'A' && c <= 'Z') {
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // Draw black text
        }
        else {
          display.setTextColor(SSD1306_WHITE);  // Draw white text
        }
        display.drawChar(sr+row, sc+col, c);
      }
    }

    display.display();
  }

};

#endif