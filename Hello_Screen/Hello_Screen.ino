// ASN NOTE: On windows, I needed to install the USb drivers from here: https://ftdichip.com/drivers/vcp-drivers/
// https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/get-started/establish-serial-connection.html

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

// The default graphics font is a 6px by 8px font.
// Our 128px screen can fit 21 chars across (21 * 6px = 126px out of 128),
// and can display 8 lines (8 * 8px = 64px out of 64).
const int PAGE_SIZE = 10;
const int COL_NUM = 21;
const int ROW_NUM = 8 * PAGE_SIZE;

char buffer[ROW_NUM][COL_NUM];

void printToScreen(const char *s) {
  Serial.printf("Displaying: %s\n", s);
  display.clearDisplay();
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // Draw white text
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
  int startRow = 0;

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

void setup() {

  Serial.begin(115200);
  delay(1000);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }

  printToScreen("A Simple Demo");

  delay(2000);

  // 8 x 21
  strncpy(buffer[0], "hello", 5);
  strncpy(buffer[1], " hello", 6);
  strncpy(buffer[2], "  hello", 7);
  strncpy(buffer[3], "   hello", 8);

  printBuffer();
}

void loop() {
  display.display();
}