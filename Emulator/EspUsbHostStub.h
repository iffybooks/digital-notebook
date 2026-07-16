#ifndef EspUsbHostStub_H_
#define EspUsbHostStub_H_
// https://gist.github.com/mildsunrise/4e231346e2078f440969cdefb6d4caa3
class EspUsbHost {
public:
  void task() { 
    int ncode = getch();
    int keycode = 0;
    switch (ncode) {
        case KEY_RIGHT: keycode = 0x4F; break; 
        case KEY_LEFT: keycode = 0x50; break; 
        case KEY_DOWN: keycode = 0x51; break; 
        case KEY_UP: keycode = 0x52; break; 
        case KEY_BACKSPACE: ncode = 8; keycode = 8; break;
        case KEY_ENTER: ncode = 13; keycode = 13; break;
        case 10: ncode = 13; keycode = 13; break;
        case 27: keycode = 0x29; break;
    }
    onKeyboardKey((uint8_t) ncode, keycode, 0);
  }
  virtual void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {}

  void begin() {}
  void setHIDLocal(int v) {}
};

#endif