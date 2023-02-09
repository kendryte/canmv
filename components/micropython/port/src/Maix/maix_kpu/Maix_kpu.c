/*
* Copyright (c) 2022, Canaan Bright Sight Co., Ltd

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "maix_kpu.h"

#include <mp.h>
#include "py_assert.h"
#include "py/qstr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "sleep.h"

#include "vfs_wrapper.h"
#include "py_image.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #define LP_RECOG_EN                 // Disable/Enable LPR Support
#define MAX_FEATURE_LEN     256     // Face Feature Calculate and Compare length

const mp_obj_type_t k210_kpu_type;

STATIC const mp_obj_type_t kpu_post_process_act_type;
STATIC const mp_obj_type_t kpu_post_process_face_type;
#ifdef LP_RECOG_EN
STATIC const mp_obj_type_t kpu_post_process_lpr_type;
#endif //LP_RECOG_EN
// STATIC const mp_obj_type_t kpu_post_process_yolo2_type;

struct model_header
{
    uint32_t identifier;
    uint32_t version;
    uint32_t flags;
    uint32_t target; // enum model_target : uint32_t
    uint32_t constants;
    uint32_t main_mem;
    uint32_t nodes;
    uint32_t inputs;
    uint32_t outputs;
    uint32_t reserved0;
} __attribute__((aligned(8)));

volatile uint32_t wait_kpu_done = 0;
volatile uint32_t g_ai_done_flag = 0;

static void ai_done(void *ctx)
{
    g_ai_done_flag = 1;
}

STATIC mp_obj_t k210_kpu_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    k210_kpu_obj_t *kpu_obj = (k210_kpu_obj_t*)malloc(sizeof(k210_kpu_obj_t));

    memset(kpu_obj, 0, sizeof(k210_kpu_obj_t));
    kpu_obj->base.type = &k210_kpu_type;

    kpu_used_mem_info_t mem = {(void *)kpu_obj, MEM_TYPE_PTR};

    if(0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        mp_raise_msg(&mp_type_MemoryError, "too many mem to list");
    }

    return MP_OBJ_FROM_PTR(kpu_obj);
}

STATIC void k210_kpu_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    k210_kpu_obj_t *kpu_obj = MP_OBJ_TO_PTR(self_in);
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);

    mp_printf(print, "type                  : kmodel (K210) \n");
    mp_printf(print, "model_type            : %s\n", kpu_obj->model.ctx.is_nncase ? "NNCASE" : "KMODELV3");
    mp_printf(print, "model_path            : %s\n", kpu_obj->model.path);
    mp_printf(print, "model_size            : %ld\n", kpu_obj->model.size);
    mp_printf(print, "model_buffer          : 0x%08X\n", kpu_obj->model.buffer);

    if(kpu_obj->model.ctx.is_nncase)
    {

    }
    else
    {
    mp_printf(print, "model_input(CHW)      : %dx%dx%d\n", kpu_obj->shape.input.chn, kpu_obj->shape.input.h, kpu_obj->shape.input.w);
    mp_printf(print, "model_output_count    : %d\n", kpu_obj->model.ctx.output_count);
    }

    // TODO: input and output shape.
}

STATIC mp_obj_t k210_kpu_load_kmodel(mp_obj_t self_in, mp_obj_t input)
{
    k210_kpu_obj_t *kpu_obj = MP_OBJ_TO_PTR(self_in);
    mp_obj_t model_path = input;

    PY_ASSERT_TYPE(self_in, &k210_kpu_type);

    if (&mp_type_str == mp_obj_get_type(model_path)) // load model from filesystem
    {
        const char *path = mp_obj_str_get_str(model_path);

        kpu_obj->model.size = maix_kpu_helper_get_mode_size_from_filesystem(path);
        strncpy(kpu_obj->model.path, path, sizeof(kpu_obj->model.path));

        if(kpu_obj->model.buffer)
        {
            maix_kpu_heler_del_mem_from_list(kpu_obj->model.buffer);
        }

        kpu_obj->model.buffer = malloc(kpu_obj->model.size);
        if (NULL == kpu_obj->model.buffer)
        {
            mp_raise_msg(&mp_type_MemoryError, "Model buffer memory allocation failed");
        }

        if (maix_kpu_helper_load_file_from_filesystem(path, kpu_obj->model.buffer, kpu_obj->model.size) != 0)
        {
            free(kpu_obj->model.buffer);
            kpu_obj->model.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file from filesystem");
        }

        if(kpu_obj->model.size != maix_kpu_helper_probe_model_size(kpu_obj->model.buffer))
        {
            free(kpu_obj->model.buffer);
            kpu_obj->model.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Model in filesystem maybe damaged");
        }
    }
    else if (&mp_type_int == mp_obj_get_type(model_path))
    {
        mp_int_t offset = mp_obj_get_int(model_path);

        if(0 >= offset)
        {
            mp_raise_ValueError("Model offset error!");
        }

        kpu_obj->model.size = maix_kpu_helper_get_mode_size_from_rawflash(offset);
        snprintf(kpu_obj->model.path, sizeof(kpu_obj->model.path), "flash:0x%08lX", offset);

        if(kpu_obj->model.buffer)
        {
            maix_kpu_heler_del_mem_from_list(kpu_obj->model.buffer);
        }

        kpu_obj->model.buffer = malloc(kpu_obj->model.size);
        if (NULL == kpu_obj->model.buffer)
        {
            mp_raise_msg(&mp_type_MemoryError, "Model buffer memory allocation failed");
        }

        if (maix_kpu_helper_load_file_from_rawflash(offset, kpu_obj->model.buffer, kpu_obj->model.size) != 0)
        {
            free(kpu_obj->model.buffer);
            kpu_obj->model.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file from rawflash");
        }
    }
    else
    {
        mp_raise_ValueError("Load_kmodel need one parameter, path(string) or offset(int)");
    }

    if (0x00 != kpu_load_kmodel(&kpu_obj->model.ctx, kpu_obj->model.buffer))
    {
        free(kpu_obj->model.buffer);
        kpu_obj->model.buffer = NULL;

        mp_raise_msg(&mp_type_OSError, "Failed to load model");
    }

    kpu_used_mem_info_t mem = {kpu_obj->model.buffer, MEM_TYPE_PTR};
    if(0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        mp_raise_msg(&mp_type_MemoryError, "Too many mem to list");
    }

    maix_kpu_helper_get_input_shape(&kpu_obj->model.ctx, \
                                    &kpu_obj->shape.input.chn, \
                                    &kpu_obj->shape.input.h, \
                                    &kpu_obj->shape.input.w);

    maix_kpu_helper_get_output_shape(&kpu_obj->model.ctx, \
                                    &kpu_obj->shape.output.chn, \
                                    &kpu_obj->shape.output.h, \
                                    &kpu_obj->shape.output.w);

    kpu_obj->state.load_kmodel = 1;
    kpu_obj->state.run_kmodel = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(k210_kpu_load_kmodel_obj, k210_kpu_load_kmodel);

STATIC mp_obj_t k210_kpu_run_kmodel(mp_obj_t self_in, mp_obj_t input)
{
    extern const mp_obj_type_t py_image_type;

    k210_kpu_obj_t *kpu_obj = MP_OBJ_TO_PTR(self_in);

    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    PY_ASSERT_TYPE(input, &py_image_type);

    image_t *kimage = py_image_cobj(input);
    if (kimage->pix_ai == NULL)
    {
        mp_raise_msg(&mp_type_ValueError, "Image formart error, use pix_to_ai() method to convert for kpu");
    }
    const uint8_t *image_buffer = kimage->pix_ai;

    if(0x00 == kpu_obj->state.load_kmodel)
    {
        mp_raise_msg(&mp_type_OSError, "Please load kmodel before");
    }

    dmac_channel_number_t dma_ch = DMAC_CHANNEL5;

    wait_kpu_done = 1;
    g_ai_done_flag = 0;

    if (0x00 != kpu_run_kmodel(&kpu_obj->model.ctx, image_buffer, dma_ch, ai_done, NULL))
    {
        wait_kpu_done = 0;
        mp_raise_msg(&mp_type_OSError, "Model Buffer maybe dirty!");
    }

    while (!g_ai_done_flag)
        ;
    g_ai_done_flag = 0;

    dmac_free_irq(dma_ch);

    wait_kpu_done = 0;

    kpu_obj->state.run_kmodel = 1;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(k210_kpu_run_kmodel_obj, k210_kpu_run_kmodel);

STATIC mp_obj_t k210_kpu_get_outputs(mp_obj_t self_in)
{
    k210_kpu_obj_t *kpu_obj = MP_OBJ_TO_PTR(self_in);

    PY_ASSERT_TYPE(self_in, &k210_kpu_type);

    if((0x00 == kpu_obj->state.load_kmodel) || (0x00 == kpu_obj->state.run_kmodel))
    {
        mp_raise_msg(&mp_type_OSError, "Please run kmodel before");
    }

    float *output = NULL;
    size_t output_size = 0;
    if(0x00 != kpu_get_output(&kpu_obj->model.ctx, 0, (uint8_t **)&output, &output_size))
    {
        mp_raise_OSError("Failed to get kpu outputs");
    }

    mp_obj_list_t *lo = m_new(mp_obj_list_t, 1);

    mp_obj_list_init(lo, 0);

    for (size_t i = 0; i < (output_size / sizeof(float)); i++)
    {
        mp_obj_list_append(lo, mp_obj_new_float(output[i]));
    }

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_get_outputs_obj, k210_kpu_get_outputs);

STATIC mp_obj_t k210_kpu_deinit(mp_obj_t self_in)
{
    k210_kpu_obj_t *kpu_obj = MP_OBJ_TO_PTR(self_in);

    PY_ASSERT_TYPE(self_in, &k210_kpu_type);

    maix_kpu_heler_del_mem_from_list(kpu_obj->model.buffer);

    kpu_obj->state._data = 0;
    memset(&kpu_obj->model, 0, sizeof(kpu_obj->model));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_deinit_obj, k210_kpu_deinit);

STATIC const mp_rom_map_elem_t k210_kpu_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_load),            MP_ROM_PTR(&k210_kpu_load_kmodel_obj) },
    { MP_ROM_QSTR(MP_QSTR_run),             MP_ROM_PTR(&k210_kpu_run_kmodel_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_outputs),     MP_ROM_PTR(&k210_kpu_get_outputs_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),          MP_ROM_PTR(&k210_kpu_deinit_obj) },

    { MP_ROM_QSTR(MP_QSTR_act),             MP_ROM_PTR(&kpu_post_process_act_type) },
    { MP_ROM_QSTR(MP_QSTR_face),            MP_ROM_PTR(&kpu_post_process_face_type) },
#ifdef LP_RECOG_EN
    { MP_ROM_QSTR(MP_QSTR_lpr),             MP_ROM_PTR(&kpu_post_process_lpr_type) },
#endif // LP_RECOG_EN
    // { MP_ROM_QSTR(MP_QSTR_yolo2),           MP_ROM_PTR(&kpu_post_process_yolo2_type) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_dict, k210_kpu_locals_dict_table);

const mp_obj_type_t k210_kpu_type = {
    {&mp_type_type},
    .name = MP_QSTR_KPU,
    .print = k210_kpu_print,
    .make_new = k210_kpu_make_new,
    .locals_dict = (mp_obj_dict_t *)&k210_kpu_dict,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Activation Function /////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _mp_obj_kpu_post_process_act {
    mp_obj_base_t base;
} mp_obj_kpu_post_process_act_t;

STATIC mp_obj_t kpu_post_process_act_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_kpu_post_process_act_t *o = m_new_obj(mp_obj_kpu_post_process_act_t);
    o->base.type = type;

    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t kpu_post_process_act_sigmoid(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_obj_t dati;

    if(1 == n_args)
    {
        dati = pos_args[0];
    }
    else if((2 == n_args) && (&kpu_post_process_act_type == mp_obj_get_type(pos_args[0])))
    {
        dati = pos_args[1];
    }

    PY_ASSERT_TYPE(dati, &mp_type_float);

    mp_float_t x = mp_obj_get_float(dati);

    return mp_obj_new_float(sigmoid(x));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(kpu_post_process_act_sigmoid_obj, 1, kpu_post_process_act_sigmoid);

STATIC mp_obj_t kpu_post_process_act_softmax(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    size_t nitems = 0;
    const mp_obj_t *items = 0;
    mp_obj_list_t *lo = NULL;
    float *dati = NULL, *dato = NULL;

    mp_obj_t li;

    if(1 == n_args)
    {
        li = pos_args[0];
    }
    else if((2 == n_args) && (&kpu_post_process_act_type == mp_obj_get_type(pos_args[0])))
    {
        li = pos_args[1];
    }

    PY_ASSERT_TYPE(li, &mp_type_list);

    mp_obj_get_array(li, &nitems, (mp_obj_t **)&items);

    if(1 == nitems)
    {
        mp_raise_ValueError("list length can not be 1");
    }

    dati = m_new(float, nitems);
    dato = m_new(float, nitems);

    for (int i = 0; i < nitems; i++)
    {
        dati[i] = mp_obj_get_float(*items++);
    }

    maix_kpu_alg_softmax(dati, dato, nitems);

    lo = m_new(mp_obj_list_t, 1);
    mp_obj_list_init(lo, 0);

    for (int i = 0; i < nitems; i++)
    {
        mp_obj_list_append(lo, mp_obj_new_float(dato[i]));
    }

    m_del(float, dati, nitems);
    m_del(float, dato, nitems);

    return MP_OBJ_FROM_PTR(lo);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(kpu_post_process_act_softmax_obj, 1, kpu_post_process_act_softmax);

STATIC const mp_rom_map_elem_t kpu_post_process_act_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_sigmoid), MP_ROM_PTR(&kpu_post_process_act_sigmoid_obj) },
    { MP_ROM_QSTR(MP_QSTR_softmax), MP_ROM_PTR(&kpu_post_process_act_softmax_obj) },
};
STATIC MP_DEFINE_CONST_DICT(kpu_post_process_act_locals_dict, kpu_post_process_act_locals_dict_table);

STATIC const mp_obj_type_t kpu_post_process_act_type = {
    { &mp_type_type },
    .name = MP_QSTR_act,
    .make_new = kpu_post_process_act_make_new,
    .locals_dict = (void*)&kpu_post_process_act_locals_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Face Post-Processing Function ///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _mp_obj_kpu_post_process_face {
    mp_obj_base_t base;
} mp_obj_kpu_post_process_face_t;

STATIC mp_obj_t kpu_post_process_face_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_kpu_post_process_face_t *o = m_new_obj(mp_obj_kpu_post_process_face_t);
    o->base.type = type;

    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t kpu_post_process_face_calc_feature(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_obj_t li;

    if (1 == n_args)
    {
        li = pos_args[0];
    }
    else if ((2 == n_args) && (&kpu_post_process_face_type == mp_obj_get_type(pos_args[0])))
    {
        li = pos_args[1];
    }

    PY_ASSERT_TYPE(li, &mp_type_list);

    float *feat_in = NULL, *feat_out = NULL;
    size_t nitems = 0;
    const mp_obj_t *items = 0;

    mp_obj_get_array(li, &nitems, (mp_obj_t **)&items);

    if(MAX_FEATURE_LEN < nitems)
    {
        mp_raise_ValueError("feature length bigger than 256");
    }

    feat_in = m_new(float, nitems);
    feat_out = m_new(float, nitems);
    for (int i = 0; i < nitems; i++)
    {
        feat_in[i] = mp_obj_get_float(*items++);
    }
    maix_kpu_alg_l2normalize(feat_in, feat_out, nitems);

    mp_obj_list_t *lo = m_new(mp_obj_list_t, 1);

    mp_obj_list_init(lo, 0);

    for (int i = 0; i < nitems; i++)
    {
        mp_obj_list_append(lo, mp_obj_new_float(feat_out[i]));
    }

    m_del(float, feat_in, nitems);
    m_del(float, feat_out, nitems);

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(kpu_post_process_face_calc_feature_obj, 1, kpu_post_process_face_calc_feature);

STATIC mp_obj_t kpu_post_process_face_feature_compare(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_obj_t feature0_obj, feature1_obj;

    if (2 == n_args)
    {
        feature0_obj = pos_args[0];
        feature1_obj = pos_args[1];
    }
    else if ((3 == n_args) && (&kpu_post_process_face_type == mp_obj_get_type(pos_args[0])))
    {
        feature0_obj = pos_args[1];
        feature1_obj = pos_args[2];
    }

    PY_ASSERT_TYPE(feature0_obj, &mp_type_list);
    PY_ASSERT_TYPE(feature1_obj, &mp_type_list);

    float *feature0 = NULL;
    float *feature1 = NULL;
    size_t feature0_len = 0;
    size_t feature1_len = 0;
    const mp_obj_t *f0_items = 0;
    const mp_obj_t *f1_items = 0;

    mp_obj_get_array(feature0_obj, &feature0_len, (mp_obj_t **)&f0_items);
    mp_obj_get_array(feature1_obj, &feature1_len, (mp_obj_t **)&f1_items);

    if (feature0_len != feature1_len)
    {
        mp_raise_ValueError("feature len not equal");
    }

    feature0 = m_new(float, feature0_len);
    for (int i = 0; i < feature0_len; i++)
    {
        feature0[i] = mp_obj_get_float(*f0_items++);
    }

    feature1 = m_new(float, feature1_len);
    for (int i = 0; i < feature1_len; i++)
    {
        feature1[i] = mp_obj_get_float(*f1_items++);
    }

    float score = calCosinDistance(feature0, feature1, feature0_len);

    m_del(float, feature0, feature0_len);
    m_del(float, feature1, feature1_len);

    return mp_obj_new_float(score);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(kpu_post_process_face_feature_compare_obj, 2, kpu_post_process_face_feature_compare);

STATIC const mp_rom_map_elem_t kpu_post_process_face_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_calculate),   MP_ROM_PTR(&kpu_post_process_face_calc_feature_obj) },
    { MP_ROM_QSTR(MP_QSTR_compare),     MP_ROM_PTR(&kpu_post_process_face_feature_compare_obj) },
};
STATIC MP_DEFINE_CONST_DICT(kpu_post_process_face_locals_dict, kpu_post_process_face_locals_dict_table);

STATIC const mp_obj_type_t kpu_post_process_face_type = {
    { &mp_type_type },
    .name = MP_QSTR_face,
    .make_new = kpu_post_process_face_make_new,
    .locals_dict = (void*)&kpu_post_process_face_locals_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//LPR Post-Processing Function ////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef LP_RECOG_EN
typedef struct _mp_obj_kpu_post_process_lpr {
    mp_obj_base_t base;
} mp_obj_kpu_post_process_lpr_t;

STATIC mp_obj_t kpu_post_process_lpr_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_kpu_post_process_lpr_t *o = m_new_obj(mp_obj_kpu_post_process_lpr_t);
    o->base.type = type;

    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t kpu_post_process_lpr_load_weight(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)pos_args[0];

    if (mp_obj_get_type(pos_args[1]) == &mp_type_str)
    {
        mp_obj_t path_obj = pos_args[1];
        const char *path = mp_obj_str_get_str(path_obj);
        size_t weight_data_size = maix_kpu_helper_get_file_size(path);

        mp_printf(&mp_plat_print, "weight_data_size: %d\r\n", weight_data_size);
        km->user_buffer = malloc(weight_data_size);
        if (!km->user_buffer)
        {
            mp_raise_msg(&mp_type_MemoryError, "weight data buffer memory allocation failed");
            return mp_const_none;
        }

        if (maix_kpu_helper_load_file_from_filesystem(path, km->user_buffer, weight_data_size) != 0)
        {
            free(km->user_buffer);
            km->user_buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file");
        }
    }
    else if (mp_obj_get_type(pos_args[1]) == &mp_type_int)
    {
        mp_obj_t path_obj = pos_args[1];
        mp_obj_t size_obj = pos_args[2];

        if (mp_obj_get_int(path_obj) <= 0)
        {
            mp_raise_ValueError("path error!");
        }
        if (mp_obj_get_int(size_obj) <= 0)
        {
            mp_raise_ValueError("size error!");
        }
        uint32_t path_addr;
        path_addr = mp_obj_get_int(path_obj);
        size_t weight_data_size = mp_obj_get_int(size_obj);
        km->user_buffer = malloc(weight_data_size);
        mp_printf(&mp_plat_print, "weight_data_size: %d\r\n", weight_data_size);
        if (!km->user_buffer)
        {
            mp_raise_msg(&mp_type_MemoryError, "weight data buffer memory allocation failed");
            return mp_const_none;
        }
        if (maix_kpu_helper_load_file_from_rawflash(path_addr, km->user_buffer, weight_data_size) != 0)
        {
            free(km->user_buffer);
            km->user_buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file");
        }
    }
    else
    {
        mp_raise_ValueError("path error!");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(kpu_post_process_lpr_load_weight_obj, 2, kpu_post_process_lpr_load_weight);

STATIC mp_obj_t kpu_post_process_lpr_recog(mp_obj_t self_in)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)self_in;

    mp_obj_list_t *ret_list = m_new(mp_obj_list_t, sizeof(mp_obj_list_t));
    mp_obj_list_init(ret_list, 0);
    if (km->user_buffer) {
        lp_recog_process((float *)km->output[0], km->output_size[0] / sizeof(float), (float *)km->user_buffer, ret_list);
    }
    else {
        mp_raise_msg(&mp_type_OSError, "not find lp weight data, please load");
    }

    return MP_OBJ_FROM_PTR(ret_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(kpu_post_process_lpr_recog_obj, kpu_post_process_lpr_recog);

STATIC const mp_rom_map_elem_t kpu_post_process_lpr_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_load_weight),      MP_ROM_PTR(&kpu_post_process_lpr_load_weight_obj) },
    {MP_ROM_QSTR(MP_QSTR_recognize),        MP_ROM_PTR(&kpu_post_process_lpr_recog_obj) },
};
STATIC MP_DEFINE_CONST_DICT(kpu_post_process_lpr_locals_dict, kpu_post_process_lpr_locals_dict_table);

STATIC const mp_obj_type_t kpu_post_process_lpr_type = {
    { &mp_type_type },
    .name = MP_QSTR_lpr,
    .make_new = kpu_post_process_lpr_make_new,
    .locals_dict = (void*)&kpu_post_process_lpr_locals_dict,
};
#endif // LP_RECOG_EN
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//YoloV2 Function /////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
typedef struct _mp_obj_kpu_post_process_yolo2 {
    mp_obj_base_t base;
} mp_obj_kpu_post_process_yolo2_t;

STATIC mp_obj_t kpu_post_process_yolo2_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_kpu_post_process_yolo2_t *o = m_new_obj(mp_obj_kpu_post_process_yolo2_t);
    o->base.type = type;

    return MP_OBJ_FROM_PTR(o);
}

STATIC void kpu_post_process_yolo2_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_obj_kpu_post_process_yolo2_t *yolo2 = MP_OBJ_TO_PTR(self_in);
    PY_ASSERT_TYPE(self_in, &kpu_post_process_yolo2_type);

    mp_printf(print, "%s->%d\n", __func__, __LINE__);
}

STATIC mp_obj_t kpu_post_process_yolo2_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_obj_kpu_post_process_yolo2_t *yolo2 = MP_OBJ_TO_PTR(self_in);
    PY_ASSERT_TYPE(self_in, &kpu_post_process_yolo2_type);

    enum
    {
        ARG_anchor,
        ARG_anchor_num,
        ARG_img_w,
        ARG_img_h,
        ARG_net_w,
        ARG_net_h,
        ARG_layer_w,
        ARG_layer_h,
        ARG_threshold,
        ARG_nms_value,
        ARG_classes,
    };

    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_anchor, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        {MP_QSTR_anchor_num, MP_ARG_INT, {.u_int = 5}},
        {MP_QSTR_img_w, MP_ARG_INT, {.u_int = 320}},
        {MP_QSTR_img_h, MP_ARG_INT, {.u_int = 240}},
        {MP_QSTR_net_w, MP_ARG_INT, {.u_int = 320}},
        {MP_QSTR_net_h, MP_ARG_INT, {.u_int = 240}},
        {MP_QSTR_layer_w, MP_ARG_INT, {.u_int = 10}},
        {MP_QSTR_layer_h, MP_ARG_INT, {.u_int = 8}},
        {MP_QSTR_threshold, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        {MP_QSTR_nms_value, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        {MP_QSTR_classes, MP_ARG_INT, {.u_int = 1}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    float threshold = 0.7;
    float nms_value = 0.4;
    float *anchor = NULL;

    if (args[ARG_threshold].u_obj != mp_const_none)
    {
        threshold = mp_obj_get_float(args[ARG_threshold].u_obj);
    }
    if (args[ARG_nms_value].u_obj != mp_const_none)
    {
        nms_value = mp_obj_get_float(args[ARG_nms_value].u_obj);
    }
    if (args[ARG_anchor].u_obj != mp_const_none)
    {
        size_t nitems = 0;
        const mp_obj_t *items = 0;
        mp_obj_get_array(args[ARG_anchor].u_obj, &nitems, (mp_obj_t **)&items);
        anchor = m_new(float, nitems);
        for (int i = 0; i < nitems; i++)
        {
            anchor[i] = mp_obj_get_float(*items++);
        }
    }
    else
    {
        // anchor = default_anchor;
        mp_raise_ValueError("need input anchor");
    }
    km->yolo2_rl = m_new(yolo2_region_layer_t, 1);
    km->yolo2_rl->anchor_number = args[ARG_anchor_num].u_int;
    km->yolo2_rl->anchor = anchor;
    km->yolo2_rl->threshold = threshold;
    km->yolo2_rl->nms_value = nms_value;
    km->yolo2_rl->classes = args[ARG_classes].u_int;
    km->yolo2_rl->image_width = args[ARG_img_w].u_int;
    km->yolo2_rl->image_height = args[ARG_img_h].u_int;
    yolo_region_layer_init(km->yolo2_rl,
                           args[ARG_layer_w].u_int,
                           args[ARG_layer_h].u_int,
                           0,
                           args[ARG_net_w].u_int,
                           args[ARG_net_h].u_int);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(kpu_post_process_yolo2_init_obj, 2, kpu_post_process_yolo2_init);

STATIC mp_obj_t py_regionlayer_deinit(mp_obj_t self_in)
{
    mp_obj_kpu_post_process_yolo2_t *yolo2 = MP_OBJ_TO_PTR(self_in);
    PY_ASSERT_TYPE(self_in, &kpu_post_process_yolo2_type);

    mp_printf(print, "%s->%d\n", __func__, __LINE__);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_regionlayer_deinit_obj, py_regionlayer_deinit);

STATIC mp_obj_t kpu_post_process_yolo2_region_layer(mp_obj_t self_in, mp_obj_t input)
{
    mp_obj_kpu_post_process_yolo2_t *yolo2 = MP_OBJ_TO_PTR(self_in);
    PY_ASSERT_TYPE(self_in, &kpu_post_process_yolo2_type);

    PY_ASSERT_TYPE(input, &mp_type_list);

    mp_obj_list_t *rect = m_new(mp_obj_list_t, 1);
    mp_obj_list_init(rect, 0);

    km->yolo2_rl->input = (float *)(km->output[0]);
    yolo_region_layer_run(km->yolo2_rl);
    yolo_region_layer_get_rect(km->yolo2_rl, rect);

    return MP_OBJ_FROM_PTR(rect);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(kpu_post_process_yolo2_region_layer_obj, kpu_post_process_yolo2_region_layer);

STATIC const mp_rom_map_elem_t kpu_post_process_yolo2_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init),             MP_ROM_PTR(&kpu_post_process_yolo2_init_obj) },
    {MP_ROM_QSTR(MP_QSTR_deinit),           MP_ROM_PTR(&py_regionlayer_deinit_obj) },
    {MP_ROM_QSTR(MP_QSTR_region_layer),     MP_ROM_PTR(&kpu_post_process_yolo2_region_layer_obj) },
};
STATIC MP_DEFINE_CONST_DICT(kpu_post_process_yolo2_locals_dict, kpu_post_process_yolo2_locals_dict_table);

STATIC const mp_obj_type_t kpu_post_process_yolo2_type = {
    { &mp_type_type },
    .name = MP_QSTR_yolo2,
    .print = kpu_post_process_yolo2_print,
    .make_new = kpu_post_process_yolo2_make_new,
    .locals_dict = (void*)&kpu_post_process_yolo2_locals_dict,
};
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
