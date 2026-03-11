#include "EspUsbHost.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SD_SCK D8
#define SD_MISO D9
#define SD_MOSI D10
#define SD_CS D2

SPIClass sdSPI(FSPI);

// TODO: need to think about supporting non-ascii characters

// The default graphics font is a 6px by 8px font.
// Our 128px screen can fit 21 chars across (21 * 6px = 126px out of 128),
// and can display 8 lines (8 * 8px = 64px out of 64).
const int PAGE_SIZE = 10;
const int COL_NUM = 21;
const int ROW_NUM = 8 * PAGE_SIZE;

char buffer[ROW_NUM][COL_NUM];

uint8_t cursorRow = 0;
uint8_t cursorCol = 0;

void printToScreen(const char *s) {
  Serial.printf("Displaying: %s\n", s);
  display.clearDisplay();
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(0, 0);              // Start at top-left corner
  display.print(s);
  display.display();
}

void printBuffer() {
  display.clearDisplay();
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(0, 0);              // Start at top-left corner

  int startCol = 0;

  int startRow;
  if (cursorRow <= 7) {
    startRow = 0;
  } else {
    startRow = cursorRow - 7;
  }

  for (int row = startRow; (row < startRow + 8) && (row < ROW_NUM); row++) {
    for (int col = 0; col < COL_NUM; col++) {
      if (buffer[row][col] == '\0') {
        continue;
      }

      if (buffer[row][col] == 13) {
        display.println();
      } else {
        display.print(buffer[row][col]);
      }
    }
  }

  display.display();
}

void updateFile() {
  File file = SD.open("/diary.txt", FILE_WRITE);

  for (uint8_t row = 0; row < ROW_NUM; row++) {
    for (uint8_t col = 0; col < COL_NUM; col++) {
      if (buffer[row][col] == '\0') {
        continue;
      }

      if (buffer[row][col] == 13) {
        file.println();
      } else {
        file.print((char)(buffer[row][col]));
      }
    }
  }

  file.close();
}

void handleKeypress(uint8_t ascii) {
  if (' ' <= ascii && ascii <= '~') {
    buffer[cursorRow][cursorCol] = ascii;
    cursorCol++;
  } else if (ascii == 13 /* newline */) {
    buffer[cursorRow][cursorCol] = ascii;
    cursorCol = 0;
    cursorRow++;
  } else if (ascii == 8 /* backspace */) {
    if (cursorRow == 0 && cursorCol == 0) {
      return;
    }

    if (cursorCol == 0) {
      cursorRow--;
      cursorCol = COL_NUM - 1;
      while (cursorCol > 0) {
        if (buffer[cursorRow][cursorCol] != '\0') {
          break;
        }
        cursorCol--;
      }
    } else {
      cursorCol--;
    }

    buffer[cursorRow][cursorCol] = '\0';
  }

  // TODO: ideally would have a pageup/pagedown handler as well

  if (cursorCol >= COL_NUM) {
    cursorCol = 0;
    // TODO: need to handle reaching the end of cursorRow better. Clear out the buffer and go to the top?
    if (cursorRow < ROW_NUM) {
      cursorRow++;
    }
  }

  printBuffer();
  updateFile();
}

class MyEspUsbHost : public EspUsbHost {
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    handleKeypress(ascii);
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