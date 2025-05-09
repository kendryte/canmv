#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"

#if CONFIG_MAIXPY_ES8311_ENABLE

#include "es8311.h"

typedef struct {
    mp_obj_base_t base;
    es8311_t es8311_obj;
} modules_es8311_obj_t;

const mp_obj_type_t modules_es8311_type;

STATIC void modules_es8311_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "ES8311 [%d]i2c addr 0x%X scl %d sda %d freq %d\n", self->es8311_obj.i2c_num, self->es8311_obj.i2c_addr, self->es8311_obj.scl_pin, self->es8311_obj.sda_pin, self->es8311_obj.i2c_freq);
}

STATIC mp_obj_t modules_es8311_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_i2c,
        ARG_i2c_addr,
        ARG_i2c_freq
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_i2c, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_i2c_addr, MP_ARG_INT, {.u_int = 0x18} },
        { MP_QSTR_i2c_freq, MP_ARG_INT, {.u_int = 400000} },
    };

    modules_es8311_obj_t *self = m_new_obj_with_finaliser(modules_es8311_obj_t);
    self->base.type = &modules_es8311_type;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args , all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t ret = es8311_init(&self->es8311_obj, args[ARG_i2c].u_int, args[ARG_i2c_addr].u_int, args[ARG_i2c_freq].u_int);
    if (ret != 0)
    {
        m_del_obj(modules_es8311_obj_t, self);
        mp_raise_OSError(ret);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t modules_es8311_del(mp_obj_t self_in)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(self_in);

    m_del_obj(modules_es8311_obj_t, self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modules_es8311_del_obj, modules_es8311_del);

STATIC mp_obj_t modules_es8311_sample_rate(size_t n_args, const mp_obj_t *args)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t sample_rate = mp_obj_get_int(args[1]);

    uint8_t ret = es8311_config_sample_rate(&self->es8311_obj, sample_rate);
    if (ret != 0)
    {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_es8311_sample_rate_obj, 2, 2, modules_es8311_sample_rate);

STATIC mp_obj_t modules_es8311_format(size_t n_args, const mp_obj_t *args)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t format = mp_obj_get_int(args[1]);

    if (format >= ES8311_FORMAT_MAX)
    {
        mp_raise_ValueError("Format error");
    }

    uint8_t ret = es8311_config_format(&self->es8311_obj, format);
    if (ret != 0)
    {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_es8311_format_obj, 2, 2, modules_es8311_format);

STATIC mp_obj_t modules_es8311_bits_per_sample(size_t n_args, const mp_obj_t *args)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t bits_per_sample = mp_obj_get_int(args[1]);

    if ((bits_per_sample != 16) && (bits_per_sample != 24) && (bits_per_sample != 32))
    {
        mp_raise_ValueError("Bits per sample error");
    }

    uint8_t ret = es8311_config_bits_per_sample(&self->es8311_obj, bits_per_sample);
    if (ret != 0)
    {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_es8311_bits_per_sample_obj, 2, 2, modules_es8311_bits_per_sample);

STATIC mp_obj_t modules_es8311_volume(size_t n_args, const mp_obj_t *args)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t volume = mp_obj_get_int(args[1]);

    if ((volume < 0) || (volume > 100))
    {
        mp_raise_ValueError("Volume error");
    }

    uint8_t ret = es8311_config_volume(&self->es8311_obj, volume);
    if (ret != 0)
    {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_es8311_volume_obj, 2, 2, modules_es8311_volume);

STATIC mp_obj_t modules_es8311_mic_gain(size_t n_args, const mp_obj_t *args)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t mic_gain = mp_obj_get_int(args[1]);

    if (mic_gain >= ES8311_MIC_GAIN_MAX)
    {
        mp_raise_ValueError("MIC gain error");
    }

    uint8_t ret = es8311_config_mic_gain(&self->es8311_obj, mic_gain);
    if (ret != 0)
    {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_es8311_mic_gain_obj, 2, 2, modules_es8311_mic_gain);

STATIC mp_obj_t modules_es8311_start(size_t n_args, const mp_obj_t *args)
{
    modules_es8311_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t codec_mode = mp_obj_get_int(args[1]);

    if (codec_mode >= ES8311_CODEC_MODE_MAX)
    {
        mp_raise_ValueError("Codec mode error");
    }

    uint8_t ret = es8311_start(&self->es8311_obj, codec_mode);
    if (ret != 0)
    {
        mp_raise_OSError(ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modules_es8311_start_obj, 2, 2, modules_es8311_start);

STATIC const mp_rom_map_elem_t modules_es8311_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___del__),             MP_ROM_PTR(&modules_es8311_del_obj) },
	{ MP_ROM_QSTR(MP_QSTR_sample_rate),         MP_ROM_PTR(&modules_es8311_sample_rate_obj) },
	{ MP_ROM_QSTR(MP_QSTR_format),              MP_ROM_PTR(&modules_es8311_format_obj) },
	{ MP_ROM_QSTR(MP_QSTR_bits_per_sample),     MP_ROM_PTR(&modules_es8311_bits_per_sample_obj) },
	{ MP_ROM_QSTR(MP_QSTR_volume),              MP_ROM_PTR(&modules_es8311_volume_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mic_gain),            MP_ROM_PTR(&modules_es8311_mic_gain_obj) },
	{ MP_ROM_QSTR(MP_QSTR_start),               MP_ROM_PTR(&modules_es8311_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT_I2S_NORMAL),   MP_ROM_INT(ES8311_FORMAT_I2S_NORMAL) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT_I2S_LEFT),     MP_ROM_INT(ES8311_FORMAT_I2S_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT_I2S_RIGHT),    MP_ROM_INT(ES8311_FORMAT_I2S_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_FORMAT_I2S_DSP),      MP_ROM_INT(ES8311_FORMAT_I2S_DSP) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_0DB),        MP_ROM_INT(ES8311_MIC_GAIN_0DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_6DB),        MP_ROM_INT(ES8311_MIC_GAIN_6DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_12DB),       MP_ROM_INT(ES8311_MIC_GAIN_12DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_18DB),       MP_ROM_INT(ES8311_MIC_GAIN_18DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_24DB),       MP_ROM_INT(ES8311_MIC_GAIN_24DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_30DB),       MP_ROM_INT(ES8311_MIC_GAIN_30DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_36DB),       MP_ROM_INT(ES8311_MIC_GAIN_36DB) },
    { MP_ROM_QSTR(MP_QSTR_MIC_GAIN_42DB),       MP_ROM_INT(ES8311_MIC_GAIN_42DB) },
    { MP_ROM_QSTR(MP_QSTR_CODEC_MODE_ENCODE),   MP_ROM_INT(ES8311_CODEC_MODE_ENCODE) },
    { MP_ROM_QSTR(MP_QSTR_CODEC_MODE_DECODE),   MP_ROM_INT(ES8311_CODEC_MODE_DECODE) },
    { MP_ROM_QSTR(MP_QSTR_CODEC_MODE_BOTH),     MP_ROM_INT(ES8311_CODEC_MODE_BOTH) },
};

STATIC MP_DEFINE_CONST_DICT(modules_es8311_locals_dict, modules_es8311_locals_dict_table);

const mp_obj_type_t modules_es8311_type = {
    { &mp_type_type },
    .name = MP_QSTR_ES8311,
    .print = modules_es8311_print,
    .make_new = modules_es8311_make_new,
    .locals_dict = (mp_obj_dict_t *)&modules_es8311_locals_dict,
};

#endif
