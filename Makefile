# -lm provides math library
CFLAGS=-Wall -ggdb -lm

TARGET_DIR=target

.PHONY: build clean run

build: clean
	mkdir target
	cc $(CFLAGS) audio.c -o $(TARGET_DIR)/audio

clean:
	rm -rf target
	rm -f *.raw
	rm -f *.wav

run: build
	./$(TARGET_DIR)/audio
	sox -r 44.1k -e signed -b 32 -c 1 out.raw out.wav
	play out.wav

