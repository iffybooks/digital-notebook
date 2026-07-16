#include "fileview.h"
#ifdef EMULATE
#include "SDStub.h"
SDStub SD;
#endif
using namespace dnb;

namespace dnb {
  void fvTests() {
    int r, c;
    dnb::Fileview fv;
    fv.processChar('a', 0); // contents: a\n 
    fv.getCursorPos(r, c);
    assert(r == 0 && c == 1);
    assert(strncmp(fv.mFirst->buf, "a\n", 2) == 0);
    assert(fv.mFirst->len == 2);
    assert(fv.mNLines == 1);

    fv.processChar('b', 0);  // contents: ab\n
    fv.getCursorPos(r, c);
    assert(r == 0 && c == 2);
    assert(strncmp(fv.mFirst->buf, "ab\n", 3) == 0);
    assert(fv.mFirst->len == 3);
    assert(fv.mNLines == 1);

    fv.processChar(13, 13); // contents: ab\n\n
    fv.getCursorPos(r, c);
    assert(r == 1 && c == 0);
    assert(strncmp(fv.mFirst->next->buf, "\n", 1) == 0);
    assert(fv.mFirst->next->len == 1);
    assert(fv.mNLines == 2);

    fv.processChar(13, 13); // contents: ab\n\nxy\n12\n
    fv.processChar('x', 0);
    fv.processChar('y', 0);
    fv.processChar(13, 13);
    fv.processChar('1', 0);
    fv.processChar('2', 0);
    fv.getCursorPos(r, c);
    assert(r == 3 && c == 2);

    fv.processChar(0, 0x50); // move cursor left
    fv.getCursorPos(r, c);
    assert(r == 3 && c == 1);

    fv.processChar(0, 0x4F); // move cursor right
    fv.getCursorPos(r, c);
    assert(r == 3 && c == 2);

    fv.processChar(0, 0x52); // move cursor up
    fv.getCursorPos(r, c);
    assert(r == 2 && c == 2);

    fv.processChar(0, 0x51); // move cursor down
    fv.getCursorPos(r, c);
    assert(r == 3 && c == 2);

    fv.processChar(0, 0x50); // move left
    fv.processChar(8, 8);
    fv.getCursorPos(r, c); // contents: ab\n\nxy\n2\n
    assert(r == 3 && c == 0);
    assert(strncmp(fv.mCursorRow->buf, "2\n", 2) == 0);
    assert(fv.mCursorRow->len == 2);
    assert(fv.mNLines == 4);

    File f = SD._open("temp.txt", FILE_WRITE);
    fv.save(f);
    f._close();

    fv.empty();
    fv.getCursorPos(r, c);
    assert (r == 0 && c == 0);
    assert(strncmp(fv.mCursorRow->buf, "\n", 1) == 0);
    assert(fv.mCursorRow->len == 1);
    assert(fv.mNLines == 1);

    fv.processChar('t', 0);
    fv.processChar('e', 0);
    fv.processChar('s', 0);
    fv.processChar('t', 0);
    fv.processChar(0, 0x50); // left
    fv.processChar(0, 0x50); // left
    fv.processChar(13, 13); // CR
    fv.getCursorPos(r, c);
    assert (r == 1 && c == 0);
    assert(strncmp(fv.mFirst->buf, "te\n", 3) == 0);
    assert(strncmp(fv.mCursorRow->buf, "st\n", 3) == 0);
    assert(fv.mCursorRow->len == 3);
    assert(fv.mNLines == 2);

    fv.processChar(8, 8); // backspace
    fv.getCursorPos(r, c);
    assert (r == 0 && c == 2);
    assert(strncmp(fv.mFirst->buf, "test\n", 5) == 0);
    assert(strncmp(fv.mCursorRow->buf, "test\n", 5) == 0);
    assert(fv.mCursorRow->len == 5);
    assert(fv.mNLines == 1);

    // Test long lines
    fv.empty();
    for (int i = 0; i < 100; i++) {
      fv.processChar('z', 0);
    }
    fv.getCursorPos(r, c);
    assert (r == 0 && c == 100);
    assert(fv.mCursorRow->len == 101);
    assert(fv.mNLines == 1);

    File emily = SD._open("emilyd.txt", FILE_READ);
    emily._seek(0);
    fv.open(emily);
    emily._close();

    int i = 0;
    for (Fileview::line* row = fv.mFirst; row && i < 10; row = row->next, i++) {
      for (int col = 0; col < row->len; col++) {
        printf("%c", row->buf[col]);
      }
    }
  }
};

int main() {
  fvTests();
}