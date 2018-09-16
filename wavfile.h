#ifndef WAVFILE_H
#define WAVFILE_H

#include <cstdint>

struct wave_riff_header_t {
    uint32_t magic;
    uint32_t fileSize;
    uint32_t formatId;
};

static_assert (sizeof(wave_riff_header_t) == 12, "wave_riff_header_t should be 12 bytes, please check alignment!");

struct wave_fmt_header_t {
    uint32_t magic;
    uint32_t blockSize;
    uint16_t audioFormat;
    uint16_t channelCount;
    uint32_t sampleRate;
    uint32_t bytesPerSecond;
    uint16_t bytesPerBlock;
    uint16_t bitsPerSample;
} ;

static_assert (sizeof(wave_fmt_header_t) == 24, "wave_fmt_header_t should be 24 bytes, please check alignment!");

typedef struct {
    uint32_t magic;
    uint32_t dataSize;
} wave_data_header_t;

static_assert (sizeof(wave_data_header_t) == 8, "wave_data_header_t should be 8 bytes, please check alignment!");

#endif // WAVFILE_H
