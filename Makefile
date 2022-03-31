# -lm provides math library
LIBS=-lm
CFLAGS=-Wall $(LIBS)
DEBUG_CFLAGS=-ggdb $(CFLAGS) 
OPT_CFLAGS= -Ofast $(CFLAGS)

TARGET_DIR=target

.PHONY: build clean run

build: clean
	mkdir target
	cc $(DEBUG_CFLAGS) audio.c -o $(TARGET_DIR)/audio

profile: clean
	mkdir target
	cc $(DEBUG_CFLAGS) -pg audio.c -o $(TARGET_DIR)/audio

optimized: clean
	mkdir target
	cc $(OPT_CFLAGS) audio.c -o $(TARGET_DIR)/audio

clean:
	rm -rf target
	rm -f *.raw
	rm -f *.wav

run: build
	./$(TARGET_DIR)/audio
	sox -r 44.1k -e signed -b 32 -c 1 out.raw out.wav
	play out.wav

