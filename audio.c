#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#define SAMPLE_RATE 44100

typedef struct {
    int num_samples;
    int rate;
    int* samples;
} audio_buf;

audio_buf new_audio_buf(int millis, int sample_rate) {
    // Trying to avoid overflow
    int num_samples = 
        (int) (((double) millis * (double) sample_rate) / 1000.0);

    audio_buf ret = {
        .rate = sample_rate,
        .num_samples = num_samples,
    };

    size_t n_bytes = sizeof(int) * num_samples;

    ret.samples = (int*) malloc(n_bytes);

    memset(ret.samples, 0, n_bytes);

    return ret;
}

void write_audio_buf(audio_buf buf, FILE *stream) {
    fwrite(buf.samples, sizeof(int), buf.num_samples, stream);
}

typedef enum {
    SYNTH_TYPE_SIN,
} synth_type;

typedef struct {
    double freq;
    double amplitude;
    synth_type type;
} synth;

void synth_play(synth a_synth, audio_buf buf) {
    double d_sample_rate = (double) buf.rate;
    switch (a_synth.type) {
        case SYNTH_TYPE_SIN:
            {
                for (int i = 0; i < buf.num_samples; i++) {
                    double d_i = (double) i;
                    double sample =
                        a_synth.amplitude 
                        * sin((2 * M_PI * d_i) * (a_synth.freq / d_sample_rate));
                    buf.samples[i] += (int) (INT_MAX * sample);
                }
            }
            break;
    }
}

int fib(int n) {
    int a = 0, b = 1;

    for (int i = 1; i <= n; i++ ) {
        int c = a + b;
        a = b;
        b = c;
    }

    return a;
}

int main () {
    audio_buf my_buf = new_audio_buf(2000, SAMPLE_RATE);

    srand(0);

    int n_sines = 100;

    synth my_sin = {
        .amplitude = 1.0 / n_sines,
        .type = SYNTH_TYPE_SIN,
    };

    srand(5);

    for (int i = 1; i <= n_sines; i++) {
        my_sin.freq = rand() % 10000;
        synth_play(my_sin, my_buf);
    }

    FILE *file = fopen("out.raw", "w+");

    write_audio_buf(my_buf, file);

    fclose(file);

    return 0;
}
