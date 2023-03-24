#include <errno.h>
#include <extra.h>
#include <fcntl.h>
#include <sound.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <escp.h>

#define NUM_CHANNELS 2

#define QOA_MIN_FILESIZE 16
#define QOA_LMS_LEN 4
#define QOA_SLICE_LEN 20

typedef struct {
    int history[QOA_LMS_LEN];
    int weights[QOA_LMS_LEN];
} qoa_lms_t;

static int qoa_dequant_tab[16][8] = {
    {1, -1, 3, -3, 5, -5, 7, -7},
    {5, -5, 18, -18, 32, -32, 49, -49},
    {16, -16, 53, -53, 95, -95, 147, -147},
    {34, -34, 113, -113, 203, -203, 315, -315},
    {63, -63, 210, -210, 378, -378, 588, -588},
    {104, -104, 345, -345, 621, -621, 966, -966},
    {158, -158, 528, -528, 950, -950, 1477, -1477},
    {228, -228, 760, -760, 1368, -1368, 2128, -2128},
    {316, -316, 1053, -1053, 1895, -1895, 2947, -2947},
    {422, -422, 1405, -1405, 2529, -2529, 3934, -3934},
    {548, -548, 1828, -1828, 3290, -3290, 5117, -5117},
    {696, -696, 2320, -2320, 4176, -4176, 6496, -6496},
    {868, -868, 2893, -2893, 5207, -5207, 8099, -8099},
    {1064, -1064, 3548, -3548, 6386, -6386, 9933, -9933},
    {1286, -1286, 4288, -4288, 7718, -7718, 12005, -12005},
    {1536, -1536, 5120, -5120, 9216, -9216, 14336, -14336},
};

static int qoa_lms_predict(const qoa_lms_t* lms) {
    int prediction = 0;
    for (int i = 0; i < QOA_LMS_LEN; i++) {
        prediction += lms->weights[i] * lms->history[i];
    }
    return prediction >> 13;
}

static void qoa_lms_update(qoa_lms_t* lms, int sample, int residual) {
    int delta = residual >> 4;
    for (int i = 0; i < QOA_LMS_LEN; i++) {
        lms->weights[i] += lms->history[i] < 0 ? -delta : delta;
    }
    for (int i = 0; i < QOA_LMS_LEN - 1; i++) {
        lms->history[i] = lms->history[i + 1];
    }
    lms->history[QOA_LMS_LEN - 1] = sample;
}

static uint64_t read_u64(const unsigned char* bytes, size_t* p) {
    uint64_t v = (uint64_t)bytes[*p + 0] << 56 | (uint64_t)bytes[*p + 1] << 48 |
                 (uint64_t)bytes[*p + 2] << 40 | (uint64_t)bytes[*p + 3] << 32 |
                 (uint64_t)bytes[*p + 4] << 24 | (uint64_t)bytes[*p + 5] << 16 |
                 (uint64_t)bytes[*p + 6] << 8 | (uint64_t)bytes[*p + 7];
    *p += 8;
    return v;
}

static int clamp(int v, int min, int max) {
    if (v < min)
        return min;
    if (v > max)
        return max;
    return v;
}

static void print_progress_bar(int width, int current, int max) {
    float progress = (float)current / (float)max;
    width -= 3;
    int threshold = (int)(progress * (float)width) + 1;
    printf("\r[");
    for (int i = 0; i < width; ++i) {
        char ch = ' ';
        if (i < threshold)
            ch = '=';
        else if (i == threshold)
            ch = '>';
        putchar(ch);
    }
    printf("]");
}

int main(int argc, char* const argv[]) {
    if (argc != 2) {
        dprintf(STDERR_FILENO, "%susage: %splay %s<%sfile%s>%s\n", F_MAGENTA, F_GREEN, F_BLUE, F_GREEN, F_BLUE, RESET);
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    struct stat st;
    if (stat(filename, &st) < 0) {
        perror("stat");
        return EXIT_FAILURE;
    }
    int input_fd = open(filename, O_RDONLY);
    if (input_fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }
    size_t num_bytes = st.st_size * sizeof(unsigned char);
    if (num_bytes < QOA_MIN_FILESIZE) {
        dprintf(STDERR_FILENO, "Not a QOA file\n");
        close(input_fd);
        return EXIT_FAILURE;
    }

    unsigned char* bytes = malloc(num_bytes);
    if (!bytes) {
        perror("malloc");
        close(input_fd);
        return EXIT_FAILURE;
    }

    size_t total_nread = 0;
    while (total_nread < num_bytes) {
        ssize_t nread = read(input_fd, bytes, num_bytes);
        if (nread < 0) {
            perror("read");
            close(input_fd);
            free(bytes);
            return EXIT_FAILURE;
        }
        total_nread += nread;
    }
    close(input_fd);

    if (strncmp((char*)bytes, "qoaf", 4) != 0) {
        dprintf(STDERR_FILENO, "Not a QOA file\n");
        free(bytes);
        return EXIT_FAILURE;
    }

    size_t header_pos = 0;
    uint32_t num_samples = read_u64(bytes, &header_pos) & 0xffffffff;
    uint64_t first_header = read_u64(bytes, &header_pos);
    uint8_t num_channels = first_header >> 56;
    uint32_t sample_rate = (first_header >> 32) & 0xffffff;
    if (num_samples == 0 || num_channels == 0 || sample_rate == 0) {
        dprintf(STDERR_FILENO, "Invalid format\n");
        free(bytes);
        return EXIT_FAILURE;
    }
    if (num_channels != NUM_CHANNELS || sample_rate > UINT16_MAX) {
        dprintf(STDERR_FILENO,
                "Unsupported number of channels or sample rate\n");
        free(bytes);
        return EXIT_FAILURE;
    }

    size_t total_samples = num_samples * num_channels;
    size_t num_sample_bytes = total_samples * sizeof(int16_t);
    int16_t* samples = malloc(num_sample_bytes);
    if (!samples) {
        free(bytes);
        return EXIT_FAILURE;
    }

    size_t pos = 8; // skip file header
    for (size_t sample_idx = 0; sample_idx < total_samples;) {
        uint64_t header = read_u64(bytes, &pos);
        if (((header >> 56) & 0xff) != num_channels ||
            ((header >> 32) & 0xffffff) != sample_rate) {
            dprintf(STDERR_FILENO, "Invalid format\n");
            free(bytes);
            free(samples);
            return EXIT_FAILURE;
        }

        qoa_lms_t lms[NUM_CHANNELS];
        for (int c = 0; c < num_channels; ++c) {
            uint64_t history = read_u64(bytes, &pos);
            uint64_t weights = read_u64(bytes, &pos);
            for (int i = 0; i < QOA_LMS_LEN; ++i) {
                lms[c].history[i] = (int16_t)(history >> 48);
                history <<= 16;
                lms[c].weights[i] = (int16_t)(weights >> 48);
                weights <<= 16;
            }
        }

        uint32_t num_samples_in_frame = (header >> 16) & 0xffff;
        for (uint32_t i = 0; i < num_samples_in_frame; i += QOA_SLICE_LEN) {
            for (int c = 0; c < num_channels; ++c) {
                uint64_t slice = read_u64(bytes, &pos);

                int scale = (slice >> 60) & 0xf;
                size_t start = num_channels * i + c;
                size_t end = num_channels * clamp(i + QOA_SLICE_LEN, 0,
                                                  num_samples_in_frame) +
                             c;

                for (size_t si = start; si < end; si += num_channels) {
                    int predicted = qoa_lms_predict(&lms[c]);
                    int quantized = (slice >> 57) & 0x7;
                    int dequantized = qoa_dequant_tab[scale][quantized];
                    int reconstructed =
                        clamp(predicted + dequantized, -32768, 32767);

                    samples[sample_idx + si] = reconstructed;
                    slice <<= 3;

                    qoa_lms_update(&lms[c], reconstructed, dequantized);
                }
            }
        }

        sample_idx += num_samples_in_frame * num_channels;
    }
    free(bytes);

    int dsp_fd = open("/dev/dsp", O_WRONLY);
    if (dsp_fd < 0) {
        if (errno == ENOENT)
            dprintf(STDERR_FILENO, "Audio device is not available\n");
        else
            perror("open");
        free(samples);
        return EXIT_FAILURE;
    }

    uint16_t inout_sample_rate = sample_rate;
    if (ioctl(dsp_fd, SOUND_SET_SAMPLE_RATE, &inout_sample_rate) < 0) {
        perror("ioctl");
        close(dsp_fd);
        free(samples);
        return EXIT_FAILURE;
    }
    if (inout_sample_rate != sample_rate) {
        dprintf(STDERR_FILENO, "Failed to set sample rate\n");
        close(dsp_fd);
        free(samples);
        return EXIT_FAILURE;
    }

    struct winsize winsize;
    if (ioctl(STDERR_FILENO, TIOCGWINSZ, &winsize) < 0)
        winsize.ws_col = 80;

    size_t max_bytes_per_write = MAX(sample_rate * sizeof(int16_t), 1 << 16);
    size_t total_nwritten = 0;
    while (total_nwritten < num_sample_bytes) {
        print_progress_bar(winsize.ws_col, total_nwritten, num_sample_bytes);

        size_t count = num_sample_bytes - total_nwritten;
        if (count > max_bytes_per_write)
            count = max_bytes_per_write;
        ssize_t nwritten =
            write(dsp_fd, (unsigned char*)samples + total_nwritten, count);
        if (nwritten < 0) {
            perror("write");
            close(dsp_fd);
            free(samples);
            return EXIT_FAILURE;
        }
        total_nwritten += nwritten;
    }
    print_progress_bar(winsize.ws_col, num_sample_bytes, num_sample_bytes);
    putchar('\n');

    close(dsp_fd);
    free(samples);

    return EXIT_SUCCESS;
}
