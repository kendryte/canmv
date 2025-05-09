#ifndef __ES8311_H
#define __ES8311_H

#include <stdint.h>
#include "sipeed_i2c.h"

typedef enum {
    ES8311_FORMAT_I2S_NORMAL,
    ES8311_FORMAT_I2S_LEFT,
    ES8311_FORMAT_I2S_RIGHT,
    ES8311_FORMAT_I2S_DSP,
    ES8311_FORMAT_MAX,
} es8311_format_t;

typedef enum {
    ES8311_MIC_GAIN_0DB,
    ES8311_MIC_GAIN_6DB,
    ES8311_MIC_GAIN_12DB,
    ES8311_MIC_GAIN_18DB,
    ES8311_MIC_GAIN_24DB,
    ES8311_MIC_GAIN_30DB,
    ES8311_MIC_GAIN_36DB,
    ES8311_MIC_GAIN_42DB,
    ES8311_MIC_GAIN_MAX,
} es8311_mic_gain_t;

typedef enum {
    ES8311_CODEC_MODE_ENCODE,
    ES8311_CODEC_MODE_DECODE,
    ES8311_CODEC_MODE_BOTH,
    ES8311_CODEC_MODE_MAX,
} es8311_codec_mode_t;

typedef struct {
    i2c_device_number_t i2c_num;
    uint8_t i2c_addr;
    uint32_t i2c_freq;
    uint8_t scl_pin;
    uint8_t sda_pin;
} es8311_t;

uint8_t es8311_init(es8311_t *obj, i2c_device_number_t i2c_num, uint8_t i2c_addr, uint32_t i2c_freq);
uint8_t es8311_config_sample_rate(es8311_t *obj, uint32_t sample_rate);
uint8_t es8311_config_format(es8311_t *obj, es8311_format_t format);
uint8_t es8311_config_bits_per_sample(es8311_t *obj, uint8_t bits_per_sample);
uint8_t es8311_config_volume(es8311_t *obj, uint8_t volume);
uint8_t es8311_config_mic_gain(es8311_t *obj, es8311_mic_gain_t gain);
uint8_t es8311_start(es8311_t *obj, es8311_codec_mode_t mode);

#endif
