#include "EspUsbHost.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

// TODO: need to figure out how many characters fit on the screen and allocate
// the buffer size that way. Also need to think about supporting non-ascii characters...
const int BUFFER_SIZE = 1000;
char buffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;

class MyEspUsbHost : public EspUsbHost {
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    File file = SD.open("/diary.txt", FILE_APPEND);
    if (!file) {
      printToScreen("error: SD Card write failed");
      return;
    }
 
    if (' ' <= ascii && ascii <= '~') {
        file.print((char)ascii);
      
        digitalWrite(21, LOW);
        delay(10);
        digitalWrite(21, HIGH);

        // wrap back around to the beginning. Screen will be cleared by the later printToScreen call.
        if (bufferIndex == BUFFER_SIZE) {
          buffer[0] = '\0';
          bufferIndex = 0;
        }

        buffer[bufferIndex] = ascii;
        bufferIndex++;

        printToScreen(buffer);
      } else if (ascii == '\r' || ascii == '\n') {
      file.println();
      // TODO: this isn't working -- need to tinker more to figure out how to do a newline on the screen.
      display.print('\n');
    }
    
    file.close();
  };
};

MyEspUsbHost usbHost;

void setup() {

  Serial.begin(115200);
  delay(1000);

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
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
  if (file) {
    file.println("Starting text file");
    file.close();
    Serial.println("SD Card write complete");
  } else {
    printToScreen("error: SD Card write failed");
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