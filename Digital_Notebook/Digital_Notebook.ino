#include "EspUsbHost.h"
#include <SPI.h>
#include <SD.h>

#define SD_SCK   D8
#define SD_MISO  D9
#define SD_MOSI  D10
#define SD_CS    D2

SPIClass sdSPI(FSPI);

class MyEspUsbHost : public EspUsbHost {
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    if (' ' <= ascii && ascii <= '~') {
      Serial.printf("%c", (char)ascii);

      File file = SD.open("/diary.txt", FILE_APPEND);
      if (file) {
      file.println((char)ascii);
      file.close();
      
      digitalWrite(21, LOW);
      delay(10);
      digitalWrite(21, HIGH);
      } else {
      Serial.println("SD Card write failed");
      }

    } else if (ascii == '\r') {
      Serial.println();
    }
  };
};

MyEspUsbHost usbHost;

void setup() {

  Serial.begin(115200);
  delay(1000);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI)) {
  Serial.println("SD card initialization failed!");
  return;
  }

  Serial.println("SD card initialized successfully!");
  File file = SD.open("/diary.txt", FILE_WRITE);
  if (file) {
  file.println("Starting text file");
  file.close();
  Serial.println("SD Card write complete");
  } else {
  Serial.println("SD Card write failed");
  }

  usbHost.begin();
  usbHost.setHIDLocal(HID_LOCAL_US);

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






