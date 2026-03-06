#include "EspUsbHost.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SD_SCK   D8
#define SD_MISO  D9
#define SD_MOSI  D10
#define SD_CS    D2

SPIClass sdSPI(FSPI);

void printToScreen(const char *s) {
  Serial.printf("Displaying: %s\n", s);
  display.clearDisplay();
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner
  display.print(s);
  display.display();
}

// TODO: need to think about supporting non-ascii characters
// TODO: could replace buffer with manually calculating where to draw incoming characters

// 168 comes from the font being a 6x8 font.
// Our 128px screen can fit 21 chars across (21 * 6px = 126px out of 128), 
// and can display 8 lines (8 * 8px = 64px out of 64).
const int BUFFER_SIZE = 168;
char buffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;

void addToBuffer(uint8_t ascii) {
  // wrap back around to the beginning.
  if (bufferIndex == BUFFER_SIZE) {
    buffer[0] = '\0';
    bufferIndex = 0;
    display.clearDisplay();
  }

  buffer[bufferIndex] = ascii;
  bufferIndex++;
}

void deleteLastFromBuffer() {
  if (bufferIndex <= 0) {
    return;
  }

  bufferIndex--;
  buffer[bufferIndex] = '\0';
}

class MyEspUsbHost : public EspUsbHost {
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    if (' ' <= ascii && ascii <= '~') {
      File file = SD.open("/diary.txt", FILE_APPEND);
      if (!file) {
        printToScreen("error: SD Card write failed");
        return;
      }

      file.print((char)ascii);
      file.close();

      digitalWrite(21, LOW);
      delay(10);
      digitalWrite(21, HIGH);

      addToBuffer(ascii);
      printToScreen(buffer);
      return;
    }

    if (ascii == '\r' || ascii == '\n') {
      File file = SD.open("/diary.txt", FILE_APPEND);
      if (!file) {
        printToScreen("error: SD Card write failed");
        return;
      }

      // TODO: this may cause double newlines when \r\n is printed (aka on windows)
      file.println();
      file.close();

      // TODO: need to clear screen if newlines are going to have the buffer extend past bottom of screen
      addToBuffer('\n');
      printToScreen(buffer);
      return;
    }
    
    if (ascii == 8 /* backspace */) {
      File file = SD.open("/diary.txt", FILE_READ);
      if (!file) {
        printToScreen("error opening in rw");
        return;
      }

      String content = file.readString();
      file.close();

      if (content.length() <= 0) {
        return;
      }

      // removes the last char from content
      // TODO: need to remove both /n and /r if they're there
      content.remove(content.length() - 1);

      // opening the file for write deletes the existing content
      file = SD.open("/diary.txt", FILE_WRITE);

      file.print(content);
      file.close();

      deleteLastFromBuffer();
      printToScreen(buffer);
      return;
    }
  };
};

MyEspUsbHost usbHost;

void setup() {

  Serial.begin(115200);
  delay(1000);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  printToScreen("setting up sd card");

  delay(1000);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI)) {
    printToScreen("SD card initialization failed!");
    return;
  }

  Serial.println("SD card initialized successfully!");
  File file = SD.open("/diary.txt", FILE_WRITE);
  if (!file) {
    printToScreen("error: could not create diary.txt");
  }

  usbHost.begin();
  usbHost.setHIDLocal(HID_LOCAL_US);

  delay(500);

  printToScreen("Digital Journal");

  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  delay(200);
  digitalWrite(21, HIGH);
  delay(200);
  digitalWrite(21, LOW);
  delay(200);
  digitalWrite(21, HIGH);
}

void loop() {
  usbHost.task();
}