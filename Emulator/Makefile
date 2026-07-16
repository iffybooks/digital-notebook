CC=g++
FILES := emulator testFileview
FLAGS=-g -Wall -Wvla -Werror -Wno-unused-variable -Wno-unused-but-set-variable -DEMULATE
LIBS=-lcurses

all: $(FILES)

emulator :: emulator.cpp EspUsbHostStub.h AdafruitStub.h SDStub.h Fileview.h Menu.h
	$(CC) $(FLAGS) $< -o $@ -lcurses

testFileview :: testFileview.cpp fileview.h SDStub.h
	$(CC) $(FLAGS) $< -o $@ -lcurses

% :: %.cpp
	$(CC) $(FLAGS) $< -o $@ -lcurses

clean:
	rm -rf $(FILES)

