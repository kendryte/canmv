#include "es8311.h"
#include "es8311_reg.h"
#include "fpioa.h"
#include "sipeed_i2c.h"

typedef struct {
    uint32_t mclk;
    uint32_t rate;
    uint8_t pre_div;
    uint8_t pre_multi;
    uint8_t adc_div;
    uint8_t dac_div;
    uint8_t fs_mode;
    uint8_t lrck_h;
    uint8_t lrck_l;
    uint8_t bclk_div;
    uint8_t adc_osr;
    uint8_t dac_osr;
} es8311_coeff_div_t;

static const es8311_coeff_div_t es8311_coeff_div[] = {
    {12288000,  8000, 0x06, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    {18432000,  8000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x05, 0xFF, 0x18, 0x10, 0x20},
    {16384000,  8000, 0x08, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 8192000,  8000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 6144000,  8000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 4096000,  8000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 3072000,  8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 2048000,  8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 1536000,  8000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 1024000,  8000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    {11289600, 11025, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 5644800, 11025, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 2822400, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 1411200, 11025, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    {12288000, 12000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 6144000, 12000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 3072000, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 1536000, 12000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    {12288000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    {18432000, 16000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xFF, 0x0C, 0x10, 0x20},
    {16384000, 16000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 8192000, 16000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 6144000, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 4096000, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 3072000, 16000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 2048000, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 1536000, 16000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    { 1024000, 16000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20},
    {11289600, 22050, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 5644800, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 2822400, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1411200, 22050, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {12288000, 24000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {18432000, 24000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 6144000, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 3072000, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1536000, 24000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {12288000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {18432000, 32000, 0x03, 0x04, 0x03, 0x03, 0x00, 0x02, 0xFF, 0x0C, 0x10, 0x10},
    {16384000, 32000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 8192000, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 6144000, 32000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 4096000, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 3072000, 32000, 0x03, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 2048000, 32000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1536000, 32000, 0x03, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7F, 0x02, 0x10, 0x10},
    { 1024000, 32000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {11289600, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 5644800, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 2822400, 44100, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1411200, 44100, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {12288000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {18432000, 48000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 6144000, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 3072000, 48000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1536000, 48000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {12288000, 64000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {18432000, 64000, 0x03, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7F, 0x06, 0x10, 0x10},
    {16384000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 8192000, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 6144000, 64000, 0x01, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7F, 0x06, 0x10, 0x10},
    { 4096000, 64000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 3072000, 64000, 0x01, 0x08, 0x03, 0x03, 0x01, 0x01, 0x7F, 0x06, 0x10, 0x10},
    { 2048000, 64000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1536000, 64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0xBF, 0x03, 0x18, 0x18},
    { 1024000, 64000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7F, 0x02, 0x10, 0x10},
    {11289600, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 5644800, 88200, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 2822400, 88200, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1411200, 88200, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7F, 0x02, 0x10, 0x10},
    {24576000, 96000, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {12288000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    {18432000, 96000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 6144000, 96000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 3072000, 96000, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10},
    { 1536000, 96000, 0x01, 0x08, 0x01, 0x01, 0x01, 0x00, 0x7F, 0x02, 0x10, 0x10},
};

static uint8_t es8311_read_reg(es8311_t *obj, uint8_t reg, uint8_t *data);
static uint8_t es8311_write_reg(es8311_t *obj, uint8_t reg, uint8_t data);
static uint16_t es8311_get_chip_id(es8311_t *obj);
static uint8_t es8311_config_mute(es8311_t *obj, uint8_t mute);

uint8_t es8311_init(es8311_t *obj, i2c_device_number_t i2c_num, uint8_t i2c_addr, uint32_t i2c_freq)
{
    uint8_t ret = 0;
    uint8_t data[1];

    obj->i2c_num = i2c_num;
    obj->i2c_addr = i2c_addr;
    obj->i2c_freq = i2c_freq;
    obj->scl_pin = fpioa_get_io_by_function(FUNC_I2C0_SCLK + i2c_num * 2);
    obj->sda_pin = fpioa_get_io_by_function(FUNC_I2C0_SDA + i2c_num * 2);

    maix_i2c_init(obj->i2c_num, 7, obj->i2c_freq);

    if (es8311_get_chip_id(obj) != ES8311_CHIP_ID)
    {
        return 1;
    }

    ret |= es8311_write_reg(obj, ES8311_GPIO_REG44, 0x08);
    ret |= es8311_write_reg(obj, ES8311_GPIO_REG44, 0x08);

    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG01, 0x30);
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG02, 0x00);
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG03, 0x10);
    ret |= es8311_write_reg(obj, ES8311_ADC_REG16, 0x24);
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG04, 0x10);
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG05, 0x00);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG0B, 0x00);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG0C, 0x00);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG10, 0x1F);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG11, 0x7F);
    ret |= es8311_write_reg(obj, ES8311_RESET_REG00, 0x80);

    ret |= es8311_read_reg(obj, ES8311_RESET_REG00, &data[0]);
    // For Master Mode
    // {
    //     data[0] |= 0x40;
    // }
    // For Slave Mode
    {
        data[0] &= 0xBF;
    }
    ret |= es8311_write_reg(obj, ES8311_RESET_REG00, data[0]);

    data[0] = 0x3F;
    // For Internal MCLK From MCLK Pin
    {
        data[0] &= 0x7F;
    }
    // For Internal MCLK From SCLK Pin
    // {
    //     data[0] |= 0x80;
    // }
    // For MCLK Inverted
    // {
    //     data[0] |= 0x40;
    // }
    // For MCLK Not Inverted
    {
        data[0] &= 0xBF;
    }
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG01, data[0]);

    ret |= es8311_read_reg(obj, ES8311_CLK_MANAGER_REG06, &data[0]);
    // For SCLK Inverted
    // {
    //     data[0] |= 0x20;
    // }
    // For SCLK Not Inverted
    {
        data[0] &= 0xDF;
    }
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG06, data[0]);

    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG13, 0x10);
    ret |= es8311_write_reg(obj, ES8311_ADC_REG1B, 0x0A);
    ret |= es8311_write_reg(obj, ES8311_ADC_REG1C, 0x6A);

    // For right channel filled width adc output while record 2 channel
    // {
    //     ret |= es8311_write_reg(ES8311_GPIO_REG44, 0x08);
    // }
    // For right channel leave empty while record 2 channel
    {
        ret |= es8311_write_reg(obj, ES8311_GPIO_REG44, 0x58);
    }

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

uint8_t es8311_config_sample_rate(es8311_t *obj, uint32_t sample_rate)
{
    uint8_t ret = 0;
    uint32_t coeff = UINT32_MAX;
    uint32_t i;
    uint8_t data[1];

    for (i = 0; i < (sizeof(es8311_coeff_div) / sizeof(es8311_coeff_div[0])); i++)
    {
        if ((es8311_coeff_div[i].rate == sample_rate) && (es8311_coeff_div[i].mclk == (sample_rate * 256)))
        {
            coeff = i;
            break;
        }
    }

    if (coeff >= (sizeof(es8311_coeff_div) / sizeof(es8311_coeff_div[0])))
    {
        return 1;
    }

    ret |= es8311_read_reg(obj, ES8311_CLK_MANAGER_REG02, &data[0]);
    data[0] &= 0x07;
    data[0] |= ((es8311_coeff_div[coeff].pre_div - 1) << 5);
    // For Internal MCLK From MCLK Pin
    {
        if (es8311_coeff_div[coeff].pre_multi == 1)
        {
            data[0] |= (0 << 3);
        }
        else if (es8311_coeff_div[coeff].pre_multi == 2)
        {
            data[0] |= (1 << 3);
        }
        else if (es8311_coeff_div[coeff].pre_multi == 4)
        {
            data[0] |= (2 << 3);
        }
        else if (es8311_coeff_div[coeff].pre_multi == 8)
        {
            data[0] |= (3 << 3);
        }
        else
        {
            data[0] |= (0 << 3);
        }
    }
    // For Internal MCLK From SCLK Pin
    // {
    //     data[0] |= (3 << 3);
    // }
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG02, data[0]);

    data[0] = 0x00;
    data[0] |= (es8311_coeff_div[coeff].adc_div - 1) << 4;
    data[0] |= (es8311_coeff_div[coeff].dac_div - 1) << 0;
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG05, data[0]);

    ret |= es8311_read_reg(obj, ES8311_CLK_MANAGER_REG03, &data[0]);
    data[0] &= 0x80;
    data[0] |= es8311_coeff_div[coeff].fs_mode << 6;
    data[0] |= es8311_coeff_div[coeff].adc_osr << 0;
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG03, data[0]);

    ret |= es8311_read_reg(obj, ES8311_CLK_MANAGER_REG04, &data[0]);
    data[0] &= 0x80;
    data[0] |= es8311_coeff_div[coeff].dac_osr << 0;
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG04, data[0]);

    ret |= es8311_read_reg(obj, ES8311_CLK_MANAGER_REG07, &data[0]);
    data[0] &= 0xC0;
    data[0] |= es8311_coeff_div[coeff].lrck_h << 0;
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG07, data[0]);

    data[0] = 0x00;
    data[0] |= es8311_coeff_div[coeff].lrck_l << 0;
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG08, data[0]);

    ret |= es8311_read_reg(obj, ES8311_CLK_MANAGER_REG06, &data[0]);
    data[0] &= 0xE0;
    if (es8311_coeff_div[coeff].bclk_div < 19)
    {
        data[0] |= (es8311_coeff_div[coeff].bclk_div - 1) << 0;
    }
    else
    {
        data[0] |= (es8311_coeff_div[coeff].bclk_div) << 0;
    }
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG06, data[0]);

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

uint8_t es8311_config_format(es8311_t *obj, es8311_format_t format)
{
    uint8_t ret = 0;
    uint8_t data[2];

    ret |= es8311_read_reg(obj, ES8311_SDPIN_REG09, &data[0]);
    ret |= es8311_read_reg(obj, ES8311_SDPOUT_REG0A, &data[1]);

    switch (format)
    {
        case ES8311_FORMAT_I2S_NORMAL:
        default:
        {
            data[0] &= 0xFC;
            data[1] &= 0xFC;
            break;
        }
        case ES8311_FORMAT_I2S_LEFT:
        case ES8311_FORMAT_I2S_RIGHT:
        {
            data[0] &= 0xFC;
            data[1] &= 0xFC;
            data[0] |= 0x01;
            data[1] |= 0x01;
            break;
        }
        case ES8311_FORMAT_I2S_DSP:
        {
            data[0] &= 0xDC;
            data[1] &= 0xDC;
            data[0] |= 0x03;
            data[1] |= 0x03;
            break;
        }
    }

    ret |= es8311_write_reg(obj, ES8311_SDPIN_REG09, data[0]);
    ret |= es8311_write_reg(obj, ES8311_SDPOUT_REG0A, data[1]);

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

uint8_t es8311_config_bits_per_sample(es8311_t *obj, uint8_t bits_per_sample)
{
    uint8_t ret = 0;
    uint8_t data[2];

    ret |= es8311_read_reg(obj, ES8311_SDPIN_REG09, &data[0]);
    ret |= es8311_read_reg(obj, ES8311_SDPOUT_REG0A, &data[1]);

    data[0] &= 0xE3;
    data[1] &= 0xE3;

    switch (bits_per_sample)
    {
        case 16:
        default:
        {
            data[0] |= 0x0C;
            data[1] |= 0x0C;
            break;
        }
        case 24:
        {
            break;
        }
        case 32:
        {
            data[0] |= 0x10;
            data[1] |= 0x10;
            break;
        }
    }

    ret |= es8311_write_reg(obj, ES8311_SDPIN_REG09, data[0]);
    ret |= es8311_write_reg(obj, ES8311_SDPOUT_REG0A, data[1]);

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

uint8_t es8311_config_volume(es8311_t *obj, uint8_t volume)
{
    uint8_t ret = 0;
    if (volume > 100)
    {
        volume = 100;
    }

    if (volume == 0)
    {
        ret |= es8311_config_mute(obj, 1);
    }

    ret |= es8311_write_reg(obj, ES8311_DAC_REG32, (uint8_t)(2.55f * volume));

    if (volume != 0)
    {
        ret |= es8311_config_mute(obj, 0);
    }

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

uint8_t es8311_config_mic_gain(es8311_t *obj, es8311_mic_gain_t gain)
{
    uint8_t res = 0;
    uint8_t data[1];

    data[0] = gain;
    res |= es8311_write_reg(obj, ES8311_ADC_REG16, data[0]);
    
    if (res != 0)
    {
        return 1;
    }

    return 0;
}

uint8_t es8311_start(es8311_t *obj, es8311_codec_mode_t mode)
{
    uint8_t ret = 0;
    uint8_t data[2];

    data[0] = 0x80;
    // For Master Mode
    // {
    //     data[0] |= 0x40;
    // }
    // For Slave Mode
    {
        data[0] &= 0xBF;
    }
    ret |= es8311_write_reg(obj, ES8311_RESET_REG00, data[0]);

    data[0] = 0x3F;
    // For Internal MCLK From MCLK Pin
    {
        data[0] &= 0x7F;
    }
    // For Internal MCLK From SCLK Pin
    // {
    //     data[0] |= 0x80;
    // }
    // For MCLK Inverted
    // {
    //     data[0] |= 0x40;
    // }
    // For MCLK Not Inverted
    {
        data[0] &= 0xBF;
    }
    ret |= es8311_write_reg(obj, ES8311_CLK_MANAGER_REG01, data[0]);

    ret |= es8311_read_reg(obj, ES8311_SDPIN_REG09, &data[0]);
    ret |= es8311_read_reg(obj, ES8311_SDPOUT_REG0A, &data[1]);

    data[0] &= 0xBF;
    data[1] &= 0xBF;
    data[0] |= 0x40;
    data[1] |= 0x40;

    if ((mode == ES8311_CODEC_MODE_DECODE) || (mode == ES8311_CODEC_MODE_BOTH))
    {
        data[0] &= 0xBF;
    }

    if ((mode == ES8311_CODEC_MODE_ENCODE) || (mode == ES8311_CODEC_MODE_BOTH))
    {
        data[1] &= 0xBF;
    }

    ret |= es8311_write_reg(obj, ES8311_SDPIN_REG09, data[0]);
    ret |= es8311_write_reg(obj, ES8311_SDPOUT_REG0A, data[1]);

    ret |= es8311_write_reg(obj, ES8311_ADC_REG17, 0xBF);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG0E, 0x02);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG12, 0x00);
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG14, 0x1A);

    ret |= es8311_read_reg(obj, ES8311_SYSTEM_REG14, &data[0]);
    // For Digital Microphone
    // {
    //     data[0] |= 0x40;
    // }
    // For Analog Microphone
    {
        data[0] &= 0xBF;
    }
    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG14, data[0]);

    ret |= es8311_write_reg(obj, ES8311_SYSTEM_REG0D, 0x01);
    ret |= es8311_write_reg(obj, ES8311_ADC_REG15, 0x40);
    ret |= es8311_write_reg(obj, ES8311_DAC_REG37, 0x08);
    ret |= es8311_write_reg(obj, ES8311_GP_REG45, 0x00);

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}

static uint8_t es8311_read_reg(es8311_t *obj, uint8_t reg, uint8_t *data)
{
    if (maix_i2c_recv_data(obj->i2c_num, obj->i2c_addr, &reg, 1, data, 1, 100) != 0)
    {
        return 1;
    }

    return 0;
}

static uint8_t es8311_write_reg(es8311_t *obj, uint8_t reg, uint8_t data)
{
    uint8_t buffer[2] = {reg, data};

    if (maix_i2c_send_data(obj->i2c_num, obj->i2c_addr, buffer, 2, 100) != 0)
    {
        return 1;
    }

    return 0;
}

static uint16_t es8311_get_chip_id(es8311_t *obj)
{
    uint8_t ret = 0;
    uint8_t data[2];

    ret |= es8311_read_reg(obj, ES8311_CHD1_REGFD, &data[0]);
    ret |= es8311_read_reg(obj, ES8311_CHD2_REGFE, &data[1]);

    if (ret != 0)
    {
        return 0;
    }

    return (((uint16_t)data[0] << 8) | data[1]);
}

static uint8_t es8311_config_mute(es8311_t *obj, uint8_t mute)
{
    uint8_t ret = 0;
    uint8_t data[1];

    ret |= es8311_read_reg(obj, ES8311_DAC_REG31, &data[0]);
    data[0] &= 0x9F;
    if (mute != 0)
    {
        data[0] |= 0x60;
    }
    ret |= es8311_write_reg(obj, ES8311_DAC_REG31, data[0]);

    if (ret != 0)
    {
        return 1;
    }

    return 0;
}
