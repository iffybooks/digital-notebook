#ifndef fileview_H_
#define fileview_H_

#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <string>

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

namespace dnb { // namespace digital notebook

  const int LINEINC = 32;
  class Fileview {
  public:
    Fileview();
    ~Fileview();

    int empty();
    int open(File& file);
    int save(File& file);

    void render(Adafruit_SSD1306& display, int width, int height);

    int processChar(uint8_t ch, int keycode);
    int numLines() const;
    void getCursorPos(int& row, int& col) const;

    const char* errorstr() const;
    friend void fvTests();

    struct line {
      char* buf;
      int maxlen;
      int len;
      line* next;
      line* prev;
    };
    line* getStartRow(int width, int height) const;

  private:

    line* newLine();
    void clear(); 
    int insertChar(line* row, int c, char ch); // add ch after c
    void deleteLine(line* row);
    void deleteChar(line* row, int c);
    line* addLine(line* row); // add new line after row
    int expandLine(Fileview::line* row); 

    int mError;
    line* mCursorRow;
    line* mScreenRow;
    int mCursorCol;
    int mCursorRowNum;
    int mNLines;
    line* mFirst;
    line* mLast;
  };

  Fileview::Fileview() : 
    mError(0),
    mCursorRow(NULL),
    mScreenRow(NULL),
    mCursorCol(0),
    mCursorRowNum(0),
    mNLines(0),
    mFirst(NULL),
    mLast(NULL) 
  {
    empty();
  }

  Fileview::~Fileview() { 
    clear(); 
  }

  void Fileview::clear() {
    line* ll = mFirst;
    while (ll) {
      delete[] ll->buf;
      line* current = ll;
      ll = ll->next;
      delete current;
    }
  }

  int Fileview::empty() {
    clear();

    line* line = newLine();
    mFirst = line;
    mLast = line;
    mNLines = 1;
    mCursorRow = mFirst;
    mCursorCol = 0;
    mCursorRowNum = 0;
    return 0;
  }

  int Fileview::open(File& file) {
    empty();
    int status = 0;
    while (file._available() && status == 0) {
      uint8_t byte = file._read();
      uint8_t keycode = 0;
      if (byte == '\n' || byte == '\r') keycode = 13;
      status = processChar(byte, keycode);
    }
    return status;
  }

  int Fileview::save(File& file) {
    for (Fileview::line* row = mFirst; row; row = row->next) {
      for (int col = 0; col < row->len; col++) {
        file._write(row->buf[col]);
      }
    }
    return 0;
  }

  int Fileview::numLines() const {
    return mNLines;
  }

  Fileview::line* Fileview::getStartRow(int width, int height) const {
    if (mCursorRowNum < height) return mFirst;

    Fileview::line* start = mCursorRow;
    for (int i = 0; i < height-1; i++, start = start->prev);
    return start;
  }

  int Fileview::processChar(uint8_t ch, int keycode) {
    int status = 0;
    if (keycode == 0x4F) { // right
      mCursorCol = std::min(mCursorRow->len-1, mCursorCol+1);
    }
    else if (keycode == 0x50) { // left 
      mCursorCol = std::max(0, mCursorCol-1);
    }
    else if (keycode == 0x51) { // down
      if (mCursorRow->next) {
        mCursorRow = mCursorRow->next;
        mCursorRowNum++;
        mCursorCol = std::min(mCursorRow->len-1, mCursorCol);
      }
      else { // add new empty line
        line* newRow = addLine(mCursorRow);
        if (!newRow) status = -1;
        mCursorRow = newRow;
        mCursorRowNum++;
        mCursorCol = 0;
      }
    }
    else if (keycode == 0x52) {  // up
      if (mCursorRow->prev) {
        mCursorRow = mCursorRow->prev;
        mCursorRowNum--;
        mCursorCol = std::min(mCursorRow->len-1, mCursorCol);
      }
    }
    else if (ch == 8) { // backspace
      if (mCursorCol == 0) { // delete line
        line* prevLine = mCursorRow->prev;
        if (prevLine) {

          mCursorCol = prevLine->len - 1; // set me first
          if (mCursorRow->len > 1) { // non-empty line
            int newSize = prevLine->len + mCursorRow->len - 1; 
            if (newSize > prevLine->maxlen) {
              status = expandLine(prevLine);
              if (status < 0) return -1;
            }
            strncpy(&(prevLine->buf[prevLine->len -1]), mCursorRow->buf, mCursorRow->len);
            prevLine->len = newSize;
          }

          Fileview::line* toDelete = prevLine->next;
          if (toDelete) prevLine->next = toDelete->next;
          deleteLine(toDelete);
          mCursorRow = prevLine;
          mCursorRowNum--;
        }
        else {
          mCursorRow = mFirst;
          mCursorCol = mFirst->len - 1;
          mCursorRowNum = 0;
        }
      }
      else { // delete char
        deleteChar(mCursorRow, mCursorCol);
        mCursorCol--;
      }
    }
    else if (ch == 13) {
      line* newRow = addLine(mCursorRow);
      if (newRow) {
        if (mCursorCol < mCursorRow->len - 1) { // non-empty line
          newRow->len = mCursorRow->len - mCursorCol;
          strncpy(newRow->buf, &(mCursorRow->buf[mCursorCol]), newRow->len);
          mCursorRow->buf[mCursorCol] = '\n';
          mCursorRow->len = mCursorCol + 1;
        }
        mCursorRow = newRow;
        mCursorRowNum++;
        mCursorCol = 0;
      }
      else {
        status = -1;
      }
    }
    else if (' ' <= ch && ch <= '~') {
      int status = insertChar(mCursorRow, mCursorCol, ch);
      mCursorCol++;
    }
    return status;
  }

  void Fileview::getCursorPos(int& row, int& col) const {
    row = mCursorRowNum;
    col = mCursorCol;
  }

  int Fileview::expandLine(Fileview::line* row) {
    char* newBuf = new char[row->maxlen + LINEINC]; // TODO: Error codes
    if (!newBuf) {
      mError = ENOMEM;
      return -1;
    }
    strncpy(newBuf, row->buf, row->maxlen);
    delete[] row->buf;
    row->buf = newBuf;
    row->maxlen = row->maxlen + LINEINC;
    return 0;
  }

  int Fileview::insertChar(Fileview::line* row, int c, char ch) {
    if (row->len >= row->maxlen) { // allocate more space
      if (expandLine(row) < 0) return -1;
    }
    for (int i = row->len; i > c; i--){ // shift right
      row->buf[i] = row->buf[i-1];
    }
    row->buf[c] = ch;
    row->len++;
    return 0;
  }

  void Fileview::deleteLine(Fileview::line* row) {
    Fileview::line* prev = row->prev;
    Fileview::line* next = row->next;

    if (!prev && !next) return; // never delete all lines
    else if (!prev) { // change head of the list
      mFirst = next;
    }
    else {
      prev->next = next;
      if (next) next->prev = prev;
    }
    delete[] row->buf;
    delete row;
    mNLines--;
  }

  void Fileview::deleteChar(Fileview::line* row, int c) {
    assert(c > 0);
    for (int i = c-1; i < row->len; i++) { // shift right
      row->buf[i] = row->buf[i+1];
    }
    row->len--;
  }

  Fileview::line* Fileview::addLine(Fileview::line* row) {
    assert(row != NULL);
    line* ll = newLine();
    if (!ll) {
      mError = ENOMEM;
      return NULL;
    }
    ll->next = row->next;
    ll->prev = row;
    if (row->next) row->next->prev = ll;
    row->next = ll;
    mNLines++;

    return ll;
  }

  const char* Fileview::errorstr() const {
    switch (mError) {
      case ENOENT: return "File not found";
      case ENOMEM: return "Out of memory";
      default: return "";
    } 
    return "";
  }
    
  Fileview::line* Fileview::newLine() {
    line* line = new Fileview::line();
    if (!line) {
      mError = ENOMEM;
      return NULL;
    }
    line->buf = new char[LINEINC];
    line->maxlen = LINEINC;
    line->len = 1;
    line->buf[0] = '\n';
    line->next = line->prev = NULL;
    return line;
  }

  void Fileview::render(Adafruit_SSD1306& display, int width, int height) {
    display.clearDisplay();
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0,0);

    int i, j, cr, cc;
    Fileview::line* row = NULL;
    Fileview::line* startRow = getStartRow(width, height);
    int startCol = mCursorCol >= width-1? mCursorCol - width + 1: 0;

    for (row = startRow, i = 0; row && i < height; row = row->next, i++) {
      if (row == mCursorRow) cr = i;
      for (j = 0; j < width; j++) {
        int idx = j + startCol;
        if (idx < row->len-1) {
          display.drawChar(i, j, (uint8_t) row->buf[idx]);
        }
        if (idx == mCursorCol) cc = j;
      }
    }
    
    display.setCursor(cr, cc);
    display.display();
  }



}; // end namespace dnb
#endif