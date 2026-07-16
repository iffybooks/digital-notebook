// https://github.com/espressif/esp-idf/blob/v6.0/examples/cxx/pthread/main/cpp_pthread.cpp
// https://docs.arduino.cc/libraries/sd/#File%20class

#ifdef EMULATE
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "EspUsbHostStub.h"
#include "AdafruitStub.h"
#include "SDStub.h"
#else
#include "EspUsbHost.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif
#include <string>
#include "fileview.h"
#include "menu.h"

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#ifdef EMULATE
#define SSD1306_WHITE 1
#define SSD1306_BLACK 2
#define NBDIR "./digital_notebook/"
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);
SDStub SD;
#else
#define NBDIR "/digital_notebook/"
#define SD_SCK D8
#define SD_MISO D9
#define SD_MOSI D10
#define SD_CS D2
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SPIClass sdSPI(FSPI);
#endif

const int COL_NUM = 21;
const int ROW_NUM = 8;

bool showmenu = 0;
std::string DIARY_FILE_NAME;

long lastKeyPress = millis();
long keyPressCount = 0;
long lastKeyPressCount = 0;

dnb::Fileview fv;
dnb::Menu menu;

void printToScreen(const char *s) {
  int r = 0;
  int c = 0;
  int n = (int) strlen(s);
  for (int i = 0; i < n; i++){
    display.drawChar(r, c++, s[i]);
    if (c >= COL_NUM) {
      r++;
      c = 0;
    }
  }
  display.setCursor(0,0);
  display.display();
}

void newFile() {
  char line[32];
  File settings = SD._open(NBDIR ".nbsettings", FILE_WRITE);
  settings._seek(0);
  settings._read((uint8_t*) line, 32);
  int num = atoi(line) + 1;
  snprintf(line, 32, "%d", num);
  settings._seek(0);
  settings._write((const uint8_t*) line, strlen(line));

  char filename[256];
  snprintf(filename, 256, NBDIR "%d.txt", num);
  DIARY_FILE_NAME = filename;

  fv.empty();
}

void saveFile() {
  File savefile = SD._open(DIARY_FILE_NAME.c_str(), FILE_WRITE);
  savefile._seek(0);
  fv.save(savefile);
  savefile._close();
}

void initializeNotebookDir() {
  if (!SD._exists(NBDIR)) {
    // initialize digital notebook files
    SD._mkdir(NBDIR);
    File settings = SD._open(NBDIR ".nbsettings", FILE_WRITE);
    settings._write((const uint8_t*) "0", 1);
    settings._close();
  }
}

class MyEspUsbHost : public EspUsbHost {
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    if (ascii == 27) { // escape
      showmenu = !showmenu;
    }
    else if (ascii == 13 && showmenu) {
      int option = menu.getOption();
      if (option == 0) newFile();
      else if (option == 1) saveFile();
      showmenu = false;
    }

    if (showmenu) {
      menu.processChar(ascii, keycode);
      menu.render(display, COL_NUM, ROW_NUM);
    }
    else {
      fv.processChar(ascii, keycode);
      fv.render(display, COL_NUM, ROW_NUM);
    }
    lastKeyPress = millis();
    keyPressCount++;
  };
};

MyEspUsbHost usbHost;

void setup() {

#ifndef EMULATE
  Serial.begin(115200);
  delay(1000);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  printToScreen("setting up sd card");
  delay(200);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI)) {
    printToScreen("SD card \ninitialization failed!");
    return;
  }

  usbHost.begin();
  usbHost.setHIDLocal(HID_LOCAL_US);
#else
  display.begin(0,0);
#endif

  initializeNotebookDir();
  newFile();

  delay(200);
  std::string message = "Digital Notebook\n\nSaving to ";
  message += DIARY_FILE_NAME + "\n\n:)";
  printToScreen(message.c_str());
}

void loop() {
  usbHost.task();
  if (((millis()-lastKeyPress) > 3000) && (keyPressCount != lastKeyPressCount)){
    saveFile(); // Put in different thread?
    lastKeyPressCount = keyPressCount;
  }
}

int main() {
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  start_color();
  cbreak();

  setup();
  while (1) {
    loop();
  }

  endwin();
  return 0;
}