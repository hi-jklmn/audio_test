#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define SAMPLE_RATE 44100

typedef struct {
    size_t rate;
    size_t num_samples;
    double* samples;
} audio_buf;

static audio_buf new_audio_buf(size_t millis, size_t sample_rate) {
    size_t num_samples = millis * (sample_rate / 1000);
    size_t n_bytes = sizeof(double) * num_samples;
    audio_buf ret = {
        .rate = sample_rate,
        .num_samples = num_samples,
        .samples = malloc(n_bytes),
    };
    memset(ret.samples, 0, n_bytes);
    return ret;
}

static void fwrite_audio_buf(audio_buf buf, FILE *stream) {
    int32_t *int_buf = malloc(buf.num_samples * sizeof(int32_t));

    for (size_t i = 0; i < buf.num_samples; i++) {
        int_buf[i] = INT32_MAX * fmin(fmax(buf.samples[i], -1.0), 1.0);
    }

    fwrite(int_buf, sizeof(int32_t), buf.num_samples, stream);
}

static void apply_fade_audio_buf(
    audio_buf buf, 
    double start_fade_pct, 
    double end_fade_pct
) {
    // Make sure percentages are within [0.0, 1.0]
    start_fade_pct = fmin(fmax(start_fade_pct, 0.0), 1.0);
    end_fade_pct   = fmin(fmax(end_fade_pct,   0.0), 1.0);
    // Determine number of samples to fade
    size_t start_fade_samples = (size_t) (buf.num_samples * start_fade_pct);
    size_t end_fade_samples   = (size_t) (buf.num_samples * end_fade_pct);

    for (size_t i = 0; i < start_fade_samples; i++) {
        double amplitude = i / (double) start_fade_samples;
        buf.samples[i] = buf.samples[i] * amplitude;
    }

    for (size_t i = 1; i <= end_fade_samples; i++) {
        double amplitude = (i - 1) / (double) end_fade_samples;
        buf.samples[buf.num_samples - i] 
            = buf.samples[buf.num_samples - i] * amplitude;
    }
}

static void add_audio_buf(
    audio_buf source_buf,
    size_t source_sample_start,
    audio_buf dest_buf,
    size_t dest_sample_start,
    size_t nsamples
) { 
    // Bounds check
    assert(source_buf.num_samples >= source_sample_start + nsamples);
    assert(dest_buf.num_samples   >= dest_sample_start   + nsamples);

    for (size_t i = 0; i < nsamples; i++) {
        dest_buf.samples[dest_sample_start + i]
            += source_buf.samples[source_sample_start + i];
    }
}

typedef enum {
    SYNTH_TYPE_SIN,
} synth_type;

typedef struct {
    double freq;
    double amplitude;
    synth_type type;
} synth;

// https://www.musicdsp.org/en/latest/Synthesis/9-fast-sine-wave-calculation.html 
typedef struct {
    double b1;
    double y[3];
} sin_gen;

void sin_gen_step(sin_gen *g) {
    g->y[0] = g->b1 * g->y[1] - g->y[2];
    g->y[2] = g->y[1];
    g->y[1] = g->y[0];
}

double sin_gen_peek(sin_gen *g) {
    return g->y[0];
}

sin_gen new_sin_gen(
    double phase, 
    double freq, 
    double samplerate
) {
    sin_gen ret = {0};
    double w = freq * 2.0 * M_PI / samplerate;
    ret.b1 = 2.0 * cos(w);
    ret.y[1] = sin(phase - w);
    ret.y[2] = sin(phase - 2 * w);
    sin_gen_step(&ret);
    return ret;
}

static void synth_play(synth a_synth, audio_buf buf) {
    double d_sample_rate = (double) buf.rate;
    switch (a_synth.type) {
        case SYNTH_TYPE_SIN: {
                double a = a_synth.amplitude;
                sin_gen sg = new_sin_gen(0.0, a_synth.freq, d_sample_rate);
                for (size_t i = 0; i < buf.num_samples; i++) {
                    sin_gen_step(&sg);
                    buf.samples[i] += a * sin_gen_peek(&sg);
                }
        } break;
    }
}

static double harmonic_series_quantize(double base_freq, double input) {
    double lower_harmonic_index = floor(input / base_freq);
    return lower_harmonic_index * base_freq;
}

int main () {
    srand(0);

    size_t buf_size_millis = 1 << 12;

    size_t n_bufs = 8;

    audio_buf final_buf =
        new_audio_buf((((float) n_bufs + 1) / 2) * buf_size_millis,SAMPLE_RATE);

    audio_buf *my_bufs = (audio_buf*) malloc(sizeof(audio_buf) * n_bufs);

    size_t n_sines = 16;

    synth my_sin = {
        .amplitude = 1.0 / n_sines,
        .type = SYNTH_TYPE_SIN,
    };

    for (size_t i = 0; i < n_bufs; i++) {
        my_bufs[i] = new_audio_buf(buf_size_millis, SAMPLE_RATE);

        size_t min_freq = 80;
        size_t max_freq = 12000;

        for (size_t j = 1; j <= n_sines; j++) {
            double r = (rand() / (double) RAND_MAX);
            // Dirty way to bias distribution towards lower frequencies
            r = (r * r * r * r * r * r);

            my_sin.freq = (max_freq - min_freq) * r + min_freq;
            my_sin.freq = harmonic_series_quantize(55.0, my_sin.freq);

            synth_play(my_sin, my_bufs[i]);
        }

        apply_fade_audio_buf(
            my_bufs[i],
            // Do not want to fade start of first buffer
            // Nor want to fade end of last buffer
            (i != 0)            ? 0.5 : 0.0,
            (i != (n_bufs - 1)) ? 0.5 : 0.0
        );
    }

    size_t num_samples = my_bufs[0].num_samples;

    for (size_t i = 0; i < n_bufs; i++) {
        add_audio_buf(
                my_bufs[i],
                0,
                final_buf,
                (num_samples / 2) * i,
                num_samples
        );
    }

    FILE *file = fopen("out.raw", "w+");

    fwrite_audio_buf(final_buf, file);

    fclose(file);

    return 0;
}
