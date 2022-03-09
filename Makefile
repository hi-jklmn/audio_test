# -lm provides math library
CFLAGS=-Wall -ggdb -lm

all: clean audio

clean:
	rm -f audio
	rm -f out.raw
	rm -f out.wav

run: clean audio
	./audio
	sox -r 44.1k -e signed -b 32 -c 1 out.raw out.wav
	play out.wav
