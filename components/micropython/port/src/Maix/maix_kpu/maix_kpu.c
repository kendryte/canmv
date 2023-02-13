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
#include "yolo2_region_layer.h"

#include <mp.h>
#include "py_assert.h"
#include "py/qstr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "sleep.h"
#include "sha256.h"

#include "vfs_wrapper.h"
#include "py_image.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_FEATURE_LEN     256     // Face Feature Calculate and Compare length

STATIC const mp_obj_type_t k210_kpu_type;
STATIC const mp_obj_type_t k210_kpu_act_type;
STATIC const mp_obj_type_t k210_kpu_face_type;
STATIC const mp_obj_type_t k210_kpu_lpr_type;
STATIC const mp_obj_type_t k210_kpu_yolo2_type;

STATIC mp_obj_t k210_kpu_deinit(mp_obj_t self_in);
STATIC mp_obj_t k210_kpu_lpr_deinit(mp_obj_t self_in);
STATIC mp_obj_t k210_kpu_yolo2_deinit(mp_obj_t self_in);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type definitions ///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _mp_obj_k210_kpu mp_obj_k210_kpu_t;

typedef struct _mp_obj_k210_kpu_lpr {
    mp_obj_base_t base;

    mp_obj_k210_kpu_t *parent;

    struct
    {
        char path[128];
        size_t size;
        uint8_t *buffer;
        uint8_t sha256[32];
    } weight;
} __attribute__((aligned(8))) mp_obj_k210_kpu_lpr_t;

typedef struct _mp_obj_k210_kpu_yolo2 {
    mp_obj_base_t base;

    mp_obj_k210_kpu_t *parent;

    struct
    {
        int32_t vaild;
        uint32_t anchor_number;
        float *anchor;

        float threshold;
        float nms_value;
    } args;
} __attribute__((aligned(8))) mp_obj_k210_kpu_yolo2_t;

struct _mp_obj_k210_kpu {
    mp_obj_base_t base;

    struct {
        mp_obj_k210_kpu_lpr_t *lpr;
        mp_obj_k210_kpu_yolo2_t *yolo;
    } child;

    struct {
        kpu_model_context_t ctx;
        char path[128];
        size_t size;
        uint8_t *buffer;
        uint8_t sha256[32];
    } model;

    union
    {
        uint32_t u32;
        struct 
        {
            uint32_t load_kmodel : 1;
            uint32_t run_kmodel : 1;
        };
    } state;

    struct {
        k210_kpu_shape_t input;
        k210_kpu_shape_t output;
    } shape;
} __attribute__((aligned(8)));

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interpreter Functions //////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
volatile uint32_t wait_kpu_done = 0;
volatile uint32_t g_ai_done_flag = 0;

static void ai_done(void *ctx)
{
    g_ai_done_flag = 1;
}

mp_obj_t k210_kpu_make_new(void)
{
    mp_obj_k210_kpu_t *self = (mp_obj_k210_kpu_t*)malloc(sizeof(mp_obj_k210_kpu_t));
    if (NULL == self)
    {
        mp_raise_msg(&mp_type_MemoryError, "No memory space");
    }

    memset(self, 0, sizeof(mp_obj_k210_kpu_t));
    self->base.type = &k210_kpu_type;

    kpu_used_mem_info_t mem = {(void *)self, MEM_TYPE_PTR};
    if (0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        free(self);
        mp_raise_msg(&mp_type_MemoryError, "Too many mem to list");
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC void k210_kpu_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "KPU (K210)\n");
    mp_printf(print, "self                  : 0x%lx\n", self);
    mp_printf(print, "parent                : (null)\n");
    mp_printf(print, "child_Lpr             : 0x%lx\n", self->child.lpr);
    mp_printf(print, "child_Yolo2           : 0x%lx\n", self->child.yolo);

    if (0x00 == self->state.load_kmodel)
    {
    mp_printf(print, "model_type            : (null)\n");
        return;
    }

    mp_printf(print, "model_type            : %s\n", self->model.ctx.is_nncase ? "NNCASE" : "KMODELV3");
    mp_printf(print, "model_path            : %s\n", self->model.path);
    mp_printf(print, "model_size            : %ld\n", self->model.size);
    mp_printf(print, "model_buffer          : 0x%08X\n", self->model.buffer);

    mp_printf(print, "model_sha256          : ");
    for(int i = 0; i < 32; i++)
    {
        mp_printf(print, "%02X", self->model.sha256[i]);
    }
    mp_printf(print, "\n");

    if (0x01 != self->shape.input.vaild)
    {
    mp_printf(print, "model_input(CHW)      : invaild\n");
    }
    else
    {
    mp_printf(print, "model_input(CHW)      : %dx%dx%d\n", self->shape.input.chn, self->shape.input.h, self->shape.input.w);
    }
    if (0x01 != self->shape.output.vaild)
    {
    mp_printf(print, "model_output(CHW)     : invaild\n");
    }
    else
    {
    mp_printf(print, "model_output(CHW)     : %dx%dx%d\n", self->shape.output.chn, self->shape.output.h, self->shape.output.w);
    }

    mp_printf(print, "model_output_count    : %d\n", maix_kpu_helper_get_output_count(&self->model.ctx));
}

STATIC mp_obj_t k210_kpu_load_kmodel(mp_obj_t self_in, mp_obj_t input)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    int load_from = 0; // 0: filesystem; 1: rawflash
    const char *path = NULL;
    mp_int_t offset = 0;

    if (&mp_type_str == mp_obj_get_type(input)) // load model from filesystem
    {
        load_from = 0;

        path = mp_obj_str_get_str(input);

        self->model.size = maix_kpu_helper_get_file_size_from_filesystem(path);
        strncpy(self->model.path, path, sizeof(self->model.path));
    }
    else if (&mp_type_int == mp_obj_get_type(input))
    {
        load_from = 1;

        if (0 >= (offset = mp_obj_get_int(input)))
        {
            mp_raise_ValueError("Model offset error!");
        }

        self->model.size = maix_kpu_helper_get_mode_size_from_rawflash(offset);
        snprintf(self->model.path, sizeof(self->model.path), "flash:0x%08lX", offset);
    }
    else
    {
        mp_raise_ValueError("Load_kmodel need one parameter, path(string) or offset(int)");
    }

    if (self->model.buffer)
    {
        maix_kpu_heler_del_mem_from_list(self->model.buffer);
    }

    self->model.buffer = malloc(self->model.size);
    if (NULL == self->model.buffer)
    {
        self->model.size = 0;
        mp_raise_msg(&mp_type_MemoryError, "Model buffer memory allocation failed");
    }

    if (0x00 == load_from)
    {
        if (maix_kpu_helper_load_file_from_filesystem(path, self->model.buffer, self->model.size) != 0)
        {
            self->model.size = 0;
            free(self->model.buffer);
            self->model.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file from filesystem");
        }

        if (self->model.size != maix_kpu_helper_probe_model_size(self->model.buffer))
        {
            self->model.size = 0;
            free(self->model.buffer);
            self->model.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Model in filesystem maybe damaged");
        }
    }
    else if (0x01 == load_from)
    {
        if (maix_kpu_helper_load_file_from_rawflash(offset, self->model.buffer, self->model.size) != 0)
        {
            self->model.size = 0;
            free(self->model.buffer);
            self->model.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file from rawflash");
        }
    }
    else
    {
        mp_raise_OSError(-MP_EINVAL);
    }

    if (0x00 != kpu_load_kmodel(&self->model.ctx, self->model.buffer))
    {
        self->model.size = 0;
        free(self->model.buffer);
        self->model.buffer = NULL;

        mp_raise_msg(&mp_type_OSError, "Failed to load model");
    }

    kpu_used_mem_info_t mem = {self->model.buffer, MEM_TYPE_PTR};
    if (0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        self->model.size = 0;
        free(self->model.buffer);
        self->model.buffer = NULL;

        mp_raise_msg(&mp_type_MemoryError, "Too many mem to list");
    }

    sha256_hard_calculate(self->model.buffer, self->model.size, self->model.sha256);

    self->shape.input.vaild = 1 + maix_kpu_helper_get_input_shape(&self->model.ctx, \
                                    &self->shape.input.chn, \
                                    &self->shape.input.h, \
                                    &self->shape.input.w);

    self->shape.output.vaild = 1 + maix_kpu_helper_get_output_shape(&self->model.ctx, \
                                    &self->shape.output.chn, \
                                    &self->shape.output.h, \
                                    &self->shape.output.w);

    self->state.load_kmodel = 1;
    self->state.run_kmodel = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(k210_kpu_load_kmodel_obj, k210_kpu_load_kmodel);

STATIC mp_obj_t k210_kpu_run_kmodel(mp_obj_t self_in, mp_obj_t input)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    if (0x00 == self->state.load_kmodel)
    {
        mp_raise_msg(&mp_type_OSError, "Please load kmodel before");
    }

    image_t *kimage = py_image_cobj(input);
    if (kimage->pix_ai == NULL)
    {
        mp_raise_msg(&mp_type_ValueError, "Image formart error, use pix_to_ai() method to convert for kpu");
    }

    if ((self->shape.input.w != kimage->w) || \
        (self->shape.input.h != kimage->h))
    {
        mp_printf(&mp_plat_print, "model input %dx%d but image is %dx%d", \
                self->shape.input.h, self->shape.input.w, kimage->h, kimage->w);

        mp_raise_OSError(MP_ERANGE);
    }

    dmac_channel_number_t dma_ch = DMAC_CHANNEL5;
    const uint8_t *image_buffer = kimage->pix_ai;

    wait_kpu_done = 1;
    g_ai_done_flag = 0;

    if (0x00 != kpu_run_kmodel(&self->model.ctx, image_buffer, dma_ch, ai_done, NULL))
    {
        wait_kpu_done = 0;
        mp_raise_msg(&mp_type_OSError, "Model Buffer maybe dirty!");
    }

    while (!g_ai_done_flag)
        ;
    g_ai_done_flag = 0;

    dmac_free_irq(dma_ch);

    wait_kpu_done = 0;

    self->state.run_kmodel = 1;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(k210_kpu_run_kmodel_obj, k210_kpu_run_kmodel);

STATIC mp_obj_t k210_kpu_get_outputs(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    if ((0x00 == self->state.load_kmodel) || (0x00 == self->state.run_kmodel))
    {
        mp_raise_msg(&mp_type_OSError, "Please load/run kmodel before");
    }

    float *output = NULL;
    size_t output_size = 0;
    if (0x00 != kpu_get_output(&self->model.ctx, 0, (uint8_t **)&output, &output_size))
    {
        mp_raise_msg(&mp_type_OSError, "Failed to get kpu outputs");
    }
    output_size /= sizeof(float);

    mp_obj_t lo = mp_obj_new_list(output_size, NULL);
    for (size_t i = 0; i < output_size; i++)
    {
        mp_obj_list_store(lo, mp_obj_new_int(i), mp_obj_new_float(output[i]));
    }

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_get_outputs_obj, k210_kpu_get_outputs);

STATIC mp_obj_t k210_kpu_get_output_shape(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t lo = mp_obj_new_list(3, NULL);
    mp_obj_list_store(lo, mp_obj_new_int(0), mp_obj_new_int(self->shape.output.chn));
    mp_obj_list_store(lo, mp_obj_new_int(1), mp_obj_new_int(self->shape.output.h));
    mp_obj_list_store(lo, mp_obj_new_int(2), mp_obj_new_int(self->shape.output.w));

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_get_output_shape_obj, k210_kpu_get_output_shape);

// set_outputs([c,h,w])
// set_outputs((c,h,w))
STATIC mp_obj_t k210_kpu_set_output_shape(mp_obj_t self_in, mp_obj_t output)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    size_t nitems = 0;
    mp_obj_t *items = MP_OBJ_NULL;

    mp_obj_get_array(output, &nitems, (mp_obj_t **)&items);

    if (0x03 != nitems)
    {
        mp_raise_ValueError("outpu shape length not 3");
    }

    self->shape.output.chn = mp_obj_get_int(*items++);
    self->shape.output.h = mp_obj_get_int(*items++);
    self->shape.output.w = mp_obj_get_int(*items++);

    self->shape.output.vaild = 1;

    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(k210_kpu_set_output_shape_obj, k210_kpu_set_output_shape);

STATIC mp_obj_t k210_kpu_deinit(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->model.buffer)
    {
        maix_kpu_heler_del_mem_from_list(self->model.buffer);
    }

    if (self->child.lpr)
    {
        k210_kpu_lpr_deinit(self->child.lpr);
    }

    if (self->child.yolo)
    {
        k210_kpu_yolo2_deinit(self->child.yolo);
    }

    memset(&self->model, 0, sizeof(self->model));
    memset(&self->state, 0, sizeof(self->state));
    memset(&self->shape, 0, sizeof(self->shape));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_deinit_obj, k210_kpu_deinit);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// New types //////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t k210_kpu_lpr_make_new(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_k210_kpu_lpr_t *o = (mp_obj_k210_kpu_lpr_t*)malloc(sizeof(mp_obj_k210_kpu_lpr_t));
    if (NULL == o)
    {
        mp_raise_msg(&mp_type_MemoryError, "No memory space");
    }
    memset(o, 0, sizeof(mp_obj_k210_kpu_lpr_t));

    kpu_used_mem_info_t mem = {(void *)o, MEM_TYPE_PTR};
    if (0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        free(o);
        mp_raise_msg(&mp_type_MemoryError, "too many mem to list");
    }

    o->base.type = &k210_kpu_lpr_type;
    o->parent = self;
    self->child.lpr = o;

    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_lpr_make_new_obj, k210_kpu_lpr_make_new);

STATIC mp_obj_t k210_kpu_yolo2_make_new(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_k210_kpu_yolo2_t *o = (mp_obj_k210_kpu_yolo2_t*)malloc(sizeof(mp_obj_k210_kpu_yolo2_t));
    if (NULL == o)
    {
        mp_raise_msg(&mp_type_MemoryError, "No memory space");
    }
    memset(o, 0, sizeof(mp_obj_k210_kpu_yolo2_t));

    kpu_used_mem_info_t mem = {(void *)o, MEM_TYPE_PTR};
    if (0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        free(o);
        mp_raise_msg(&mp_type_MemoryError, "too many mem to list");
    }

    o->base.type = &k210_kpu_yolo2_type;
    o->parent = self;
    self->child.yolo = o;

    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_yolo2_make_new_obj, k210_kpu_yolo2_make_new);

STATIC mp_obj_t k210_kpu_attr(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_type);
    mp_obj_k210_kpu_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t model = mp_obj_new_dict(0);
    mp_obj_dict_store(model, MP_OBJ_NEW_QSTR(MP_QSTR_size), mp_obj_new_int(self->model.size));
    mp_obj_dict_store(model, MP_OBJ_NEW_QSTR(MP_QSTR_name), mp_obj_new_str((const char *)self->model.path, strlen(self->model.path)));
    mp_obj_dict_store(model, MP_OBJ_NEW_QSTR(MP_QSTR_sha256), mp_obj_new_bytearray(32, self->model.sha256));

    mp_obj_t input = mp_obj_new_list(3, NULL);
    mp_obj_list_store(input, mp_obj_new_int(0), mp_obj_new_int(self->shape.input.chn));
    mp_obj_list_store(input, mp_obj_new_int(1), mp_obj_new_int(self->shape.input.h));
    mp_obj_list_store(input, mp_obj_new_int(2), mp_obj_new_int(self->shape.input.w));

    mp_obj_t output = mp_obj_new_list(3, NULL);
    mp_obj_list_store(output, mp_obj_new_int(0), mp_obj_new_int(self->shape.output.chn));
    mp_obj_list_store(output, mp_obj_new_int(1), mp_obj_new_int(self->shape.output.h));
    mp_obj_list_store(output, mp_obj_new_int(2), mp_obj_new_int(self->shape.output.w));

    mp_obj_t shape = mp_obj_new_dict(0);
    mp_obj_dict_store(shape, MP_OBJ_NEW_QSTR(MP_QSTR_input), input);
    mp_obj_dict_store(shape, MP_OBJ_NEW_QSTR(MP_QSTR_output), output);

    mp_obj_t child = mp_obj_new_dict(0);
    mp_obj_dict_store(child, MP_OBJ_NEW_QSTR(MP_QSTR_Lpr), mp_obj_new_int((mp_uint_t)self->child.lpr));
    mp_obj_dict_store(child, MP_OBJ_NEW_QSTR(MP_QSTR_Yolo2), mp_obj_new_int((mp_uint_t)self->child.yolo));

    mp_obj_t o = mp_obj_new_dict(0);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_model), model);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_shape), shape);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_child), child);

    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_self), mp_obj_new_int((mp_uint_t)self));
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_parent), mp_const_none);

    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_attr_obj, k210_kpu_attr);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC const mp_rom_map_elem_t k210_kpu_locals_dict_table[] = {
    // methods
    { MP_ROM_QSTR(MP_QSTR_load),                    MP_ROM_PTR(&k210_kpu_load_kmodel_obj) },
    { MP_ROM_QSTR(MP_QSTR_run),                     MP_ROM_PTR(&k210_kpu_run_kmodel_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_outputs),             MP_ROM_PTR(&k210_kpu_get_outputs_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_output_shape),        MP_ROM_PTR(&k210_kpu_get_output_shape_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_output_shape),        MP_ROM_PTR(&k210_kpu_set_output_shape_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),                  MP_ROM_PTR(&k210_kpu_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_attr),                    MP_ROM_PTR(&k210_kpu_attr_obj) },

    // make_new types
    { MP_ROM_QSTR(MP_QSTR_Lpr),                     MP_ROM_PTR(&k210_kpu_lpr_make_new_obj) },
    { MP_ROM_QSTR(MP_QSTR_Yolo2),                   MP_ROM_PTR(&k210_kpu_yolo2_make_new_obj) },

    // const types
    { MP_ROM_QSTR(MP_QSTR_Act),                     MP_ROM_PTR(&k210_kpu_act_type) },
    { MP_ROM_QSTR(MP_QSTR_Face),                    MP_ROM_PTR(&k210_kpu_face_type) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_dict, k210_kpu_locals_dict_table);

STATIC const mp_obj_type_t k210_kpu_type = {
    {&mp_type_type},
    .name = MP_QSTR_KPU,
    .print = k210_kpu_print,
    .locals_dict = (mp_obj_dict_t *)&k210_kpu_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Activation Function /////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t k210_kpu_act_sigmoid(mp_obj_t arg_in)
{
    PY_ASSERT_TYPE(arg_in, &mp_type_float);

    mp_float_t x = mp_obj_get_float(arg_in);

    return mp_obj_new_float(sigmoid(x));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_act_sigmoid_obj, k210_kpu_act_sigmoid);

STATIC mp_obj_t k210_kpu_act_softmax(mp_obj_t arg_in)
{
    size_t nitems = 0;
    mp_obj_t lo, *items = MP_OBJ_NULL;
    float *dati = NULL, *dato = NULL;

    mp_obj_get_array(arg_in, &nitems, (mp_obj_t **)&items);

    if (1 <= nitems)
    {
        mp_raise_ValueError("list minimum size is 2");
    }

    dati = (float *)malloc(sizeof(float) * nitems * 2);
    dato = dati + nitems;

    for (int i = 0; i < nitems; i++)
    {
        dati[i] = mp_obj_get_float(*items++);
    }

    maix_kpu_alg_softmax(dati, dato, nitems);

    lo = mp_obj_new_list(nitems, NULL);
    for (int i = 0; i < nitems; i++)
    {
        mp_obj_list_store(lo, mp_obj_new_int(i), mp_obj_new_float(dato[i]));
    }

    free(dati);

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_act_softmax_obj, k210_kpu_act_softmax);

STATIC const mp_rom_map_elem_t k210_kpu_act_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_sigmoid), MP_ROM_PTR(&k210_kpu_act_sigmoid_obj) },
    { MP_ROM_QSTR(MP_QSTR_softmax), MP_ROM_PTR(&k210_kpu_act_softmax_obj) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_act_locals_dict, k210_kpu_act_locals_dict_table);

STATIC const mp_obj_type_t k210_kpu_act_type = {
    { &mp_type_type },
    .name = MP_QSTR_nn_activation,
    .locals_dict = (void*)&k210_kpu_act_locals_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Face Post-Processing Function ///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t k210_kpu_face_calc_feature(mp_obj_t arg_in)
{
    size_t nitems = 0;
    mp_obj_t lo, *items = MP_OBJ_NULL;
    float *feat_in = NULL, *feat_out = NULL;

    mp_obj_get_array(arg_in, &nitems, (mp_obj_t **)&items);

    if (MAX_FEATURE_LEN < nitems)
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

    lo = mp_obj_new_list(nitems, NULL);
    for (int i = 0; i < nitems; i++)
    {
        mp_obj_list_store(lo, mp_obj_new_int(i), mp_obj_new_float(feat_out[i]));
    }

    m_del(float, feat_in, nitems);
    m_del(float, feat_out, nitems);

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_face_calc_feature_obj, k210_kpu_face_calc_feature);

STATIC mp_obj_t k210_kpu_face_feature_compare(mp_obj_t feature0_obj, mp_obj_t feature1_obj)
{
    float *feature0 = NULL, *feature1 = NULL;
    size_t feature0_len = 0, feature1_len = 0;
    mp_obj_t *f0_items = MP_OBJ_NULL, *f1_items = MP_OBJ_NULL;

    mp_obj_get_array(feature0_obj, &feature0_len, (mp_obj_t **)&f0_items);
    mp_obj_get_array(feature1_obj, &feature1_len, (mp_obj_t **)&f1_items);

    if ((feature0_len != feature1_len) && (MAX_FEATURE_LEN < feature0_len))
    {
        mp_raise_ValueError("feature len not equal");
    }

    feature0 = (float *)malloc(sizeof(float) * feature0_len * 2);
    for (int i = 0; i < feature0_len; i++)
    {
        feature0[i] = mp_obj_get_float(*f0_items++);
    }

    feature1 = feature0 + feature0_len;
    for (int i = 0; i < feature1_len; i++)
    {
        feature1[i] = mp_obj_get_float(*f1_items++);
    }

    float score = calCosinDistance(feature0, feature1, feature0_len);

    free(feature0);

    return mp_obj_new_float(score);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(k210_kpu_face_feature_compare_obj, k210_kpu_face_feature_compare);

STATIC const mp_rom_map_elem_t k210_kpu_face_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_calculate),   MP_ROM_PTR(&k210_kpu_face_calc_feature_obj) },
    { MP_ROM_QSTR(MP_QSTR_compare),     MP_ROM_PTR(&k210_kpu_face_feature_compare_obj) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_face_locals_dict, k210_kpu_face_locals_dict_table);

STATIC const mp_obj_type_t k210_kpu_face_type = {
    { &mp_type_type },
    .name = MP_QSTR_face,
    .locals_dict = (void*)&k210_kpu_face_locals_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//LPR Post-Processing Function ////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC void k210_kpu_lpr_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_lpr_type);
    mp_obj_k210_kpu_lpr_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "LPR\n");
    mp_printf(print, "self             : 0x%lx\n", self);
    mp_printf(print, "parent           : 0x%lx\n", self->parent);
    mp_printf(print, "weight_name      : %s\n", self->weight.path);
    mp_printf(print, "weight_size      : %d\n", self->weight.size);
    mp_printf(print, "weight_buffer    : 0x%lx\n", self->weight.buffer);
    mp_printf(print, "weight_sha256    : ");
    for(int i = 0; i < 32; i++)
    {
        mp_printf(print, "%02X", self->weight.sha256[i]);
    }
    mp_printf(print, "\n");
}

// lpr.init("/sd/lpr_wight.bin")
// lpr.init(0x300000)
STATIC mp_obj_t k210_kpu_lpr_load(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    PY_ASSERT_TYPE(pos_args[0], &k210_kpu_lpr_type);
    mp_obj_k210_kpu_lpr_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    int load_from = 0; // 0: filesystem; 1: rawflash
    mp_int_t offset = 0, size = 0;
    const char *path = NULL;

    if (mp_obj_get_type(pos_args[1]) == &mp_type_str)
    {
        load_from = 0;

        path = mp_obj_str_get_str(pos_args[1]);
        size = maix_kpu_helper_get_file_size_from_filesystem(path);
        strncpy(self->weight.path, path, sizeof(self->weight.path));
    }
    else if (mp_obj_get_type(pos_args[1]) == &mp_type_int)
    {
        PY_ASSERT_TYPE(pos_args[2], &mp_type_int); // assert size type.

        load_from = 1;
        size = mp_obj_get_int(pos_args[2]);

        if ((offset = mp_obj_get_int(pos_args[1])) <= 0) // offset
        {
            mp_raise_ValueError("offset error!");
        }
        snprintf(self->weight.path, sizeof(self->weight.path), "flash:0x%08lX", offset);
    }
    else
    {
        mp_raise_ValueError("load(\"path\" / offset, size)");
    }

    if (size <= 0)
    {
        mp_raise_ValueError("weight file size error");
    }

    if (self->weight.buffer)
    {
        maix_kpu_heler_del_mem_from_list(self->weight.buffer);
        self->weight.buffer = NULL;
    }

    self->weight.size = size;
    self->weight.buffer = (uint8_t *)malloc(self->weight.size);
    if (NULL == self->weight.buffer)
    {
        self->weight.size = 0;
        mp_raise_msg(&mp_type_MemoryError, "LPR weight buffer memory allocation failed");
    }

    if (0x00 == load_from)
    {
        if (0x00 != maix_kpu_helper_load_file_from_filesystem(path, self->weight.buffer, self->weight.size))
        {
            self->weight.size = 0;
            free(self->weight.buffer);
            self->weight.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file from filesystem");
        }
    }
    else if (0x01 == load_from)
    {
        if (0x00 != maix_kpu_helper_load_file_from_rawflash(offset, self->weight.buffer, self->weight.size))
        {
            self->weight.size = 0;
            free(self->weight.buffer);
            self->weight.buffer = NULL;
            mp_raise_msg(&mp_type_OSError, "Failed to read file from rawflash");
        }
    }
    else
    {
        mp_raise_OSError(-MP_EINVAL);
    }

    kpu_used_mem_info_t mem = {self->weight.buffer, MEM_TYPE_PTR};
    if (0 > maix_kpu_helper_add_mem_to_list(&mem))
    {
        self->weight.size = 0;
        free(self->weight.buffer);
        self->weight.buffer = NULL;
        mp_raise_msg(&mp_type_MemoryError, "Too many mem to list");
    }

    sha256_hard_calculate(self->weight.buffer, self->weight.size, self->weight.sha256);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(k210_kpu_lpr_load_obj, 2, k210_kpu_lpr_load);

STATIC mp_obj_t kpu_post_process_lpr_run(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_lpr_type);
    mp_obj_k210_kpu_lpr_t *self = MP_OBJ_TO_PTR(self_in);

    if (NULL == self->weight.buffer)
    {
        mp_raise_msg(&mp_type_OSError, "Please call init before run");
    }

    if (NULL == self->parent)
    {
        mp_raise_msg(&mp_type_OSError, "Wrong uasge, please read the docs");
    }

    if ((0x00 == self->parent->state.load_kmodel) || (0x00 == self->parent->state.run_kmodel))
    {
        mp_raise_msg(&mp_type_OSError, "Please load/run kmodel before");
    }

    float *output = NULL;
    size_t output_size = 0;
    if (0x00 != kpu_get_output(&self->parent->model.ctx, 0, (uint8_t **)&output, &output_size))
    {
        mp_raise_msg(&mp_type_OSError, "Failed to get kpu outputs");
    }

    mp_obj_list_t *ret_list = m_new(mp_obj_list_t, sizeof(mp_obj_list_t));
    mp_obj_list_init(ret_list, 0);

    lp_recog_process(output, output_size / sizeof(float), (float *)self->weight.buffer, ret_list);

    return MP_OBJ_FROM_PTR(ret_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(kpu_post_process_lpr_run_obj, kpu_post_process_lpr_run);

STATIC mp_obj_t k210_kpu_lpr_deinit(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_lpr_type);
    mp_obj_k210_kpu_lpr_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->weight.buffer)
    {
        maix_kpu_heler_del_mem_from_list(self->weight.buffer);
    }

    memset(&self->weight, 0, sizeof(self->weight));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_lpr_deinit_obj, k210_kpu_lpr_deinit);

STATIC mp_obj_t k210_kpu_lpr_attr(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_lpr_type);
    mp_obj_k210_kpu_lpr_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t weigth = mp_obj_new_dict(0);
    mp_obj_dict_store(weigth, MP_OBJ_NEW_QSTR(MP_QSTR_size), mp_obj_new_int(self->weight.size));
    mp_obj_dict_store(weigth, MP_OBJ_NEW_QSTR(MP_QSTR_name), mp_obj_new_str((const char *)self->weight.path, strlen(self->weight.path)));
    mp_obj_dict_store(weigth, MP_OBJ_NEW_QSTR(MP_QSTR_sha256), mp_obj_new_bytearray(32, self->weight.sha256));

    mp_obj_t o = mp_obj_new_dict(0);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_weight), weigth);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_self), mp_obj_new_int((mp_uint_t)self));
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_parent), mp_obj_new_int((mp_uint_t)self->parent));

    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_lpr_attr_obj, k210_kpu_lpr_attr);

STATIC const mp_rom_map_elem_t k210_kpu_lpr_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_load),     MP_ROM_PTR(&k210_kpu_lpr_load_obj) },
    {MP_ROM_QSTR(MP_QSTR_run),      MP_ROM_PTR(&kpu_post_process_lpr_run_obj) },
    {MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&k210_kpu_lpr_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_attr),    MP_ROM_PTR(&k210_kpu_lpr_attr_obj) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_lpr_locals_dict, k210_kpu_lpr_locals_dict_table);

STATIC const mp_obj_type_t k210_kpu_lpr_type = {
    { &mp_type_type },
    .name = MP_QSTR_license_plate_recognition,
    .print = k210_kpu_lpr_print,
    .locals_dict = (void*)&k210_kpu_lpr_locals_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//YoloV2 Function /////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC void k210_kpu_yolo2_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_yolo2_type);
    mp_obj_k210_kpu_yolo2_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "Yolo2\n");
    mp_printf(print, "self             : 0x%lx\n", self);
    mp_printf(print, "parent           : 0x%lx\n", self->parent);

    if (0x00 == self->args.vaild)
    {
        return;
    }

    mp_printf(print, "nms              : %f\n", self->args.nms_value);
    mp_printf(print, "threshold        : %f\n", self->args.threshold);
    mp_printf(print, "anchor_number    : %d\n", self->args.anchor_number);
    mp_printf(print, "anchors          : ");

    if (self->args.anchor_number)
    {
        mp_obj_t anchors = mp_obj_new_list(self->args.anchor_number * 2, NULL);
        for(uint32_t i = 0; i < (self->args.anchor_number * 2); i++)
        {
            mp_obj_list_store(anchors, mp_obj_new_int(i), mp_obj_new_float(self->args.anchor[i]));
        }
        mp_obj_print(anchors, PRINT_REPR);

        // mp_obj_list_t *l = MP_OBJ_TO_PTR(anchors);
        // m_del(mp_obj_float_t, l->items, (self->args.anchor_number * 2));
        m_del_obj(mp_obj_list_t, anchors);
    }

    mp_printf(print, "\n");
}

STATIC mp_obj_t k210_kpu_yolo2_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
#define DEFAULT_THRESHOLD   (0.5)
#define DEFAULT_NMS_VALUE   (0.3)

    PY_ASSERT_TYPE(pos_args[0], &k210_kpu_yolo2_type);
    mp_obj_k210_kpu_yolo2_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    enum { ARG_anchor, ARG_threshold, ARG_nms_value, };

    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_anchor,    MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = mp_const_none}},
        {MP_QSTR_threshold, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        {MP_QSTR_nms_value, MP_ARG_OBJ, {.u_obj = mp_const_none}},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->args.anchor = NULL;
    self->args.anchor_number = 0;
    self->args.threshold = DEFAULT_THRESHOLD;
    self->args.nms_value = DEFAULT_NMS_VALUE;

    if (args[ARG_threshold].u_obj != mp_const_none)
    {
        self->args.threshold = mp_obj_get_float(args[ARG_threshold].u_obj);
    }

    if (args[ARG_nms_value].u_obj != mp_const_none)
    {
        self->args.nms_value = mp_obj_get_float(args[ARG_nms_value].u_obj);
    }

    if (args[ARG_anchor].u_obj != mp_const_none)
    {
        size_t nitems = 0;
        const mp_obj_t *items = 0;
        mp_obj_get_array(args[ARG_anchor].u_obj, &nitems, (mp_obj_t **)&items);

        if (0x00 != (nitems % 2))
        {
            mp_raise_ValueError("The number of anchors should be a multiple of 2");
        }

        self->args.anchor_number = nitems / 2;
        self->args.anchor = (float *)malloc(sizeof(float) * nitems);
        for (int i = 0; i < nitems; i++)
        {
            self->args.anchor[i] = mp_obj_get_float(*items++);
        }

        kpu_used_mem_info_t mem = {(void *)self->args.anchor, MEM_TYPE_PTR};
        if (0 > maix_kpu_helper_add_mem_to_list(&mem))
        {
            free(self->args.anchor);
            mp_raise_msg(&mp_type_MemoryError, "too many mem to list");
        }
    }
    else
    {
        mp_raise_ValueError("need input anchor");
    }

    self->args.vaild = 1;

    return mp_const_none;

#undef DEFAULT_THRESHOLD
#undef DEFAULT_NMS_VALUE
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(k210_kpu_yolo2_init_obj, 2, k210_kpu_yolo2_init);

STATIC mp_obj_t k210_kpu_yolo2_run(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_yolo2_type);
    mp_obj_k210_kpu_yolo2_t *self = MP_OBJ_TO_PTR(self_in);

    if ((NULL == self->args.anchor) || (0x00 == self->args.vaild))
    {
        mp_raise_msg(&mp_type_OSError, "Please call init before run");
    }

    if (NULL == self->parent)
    {
        mp_raise_msg(&mp_type_OSError, "Wrong uasge, please read the docs");
    }

    if ((0x00 == self->parent->shape.input.vaild) || (0x00 == self->parent->shape.output.vaild))
    {
        mp_raise_msg(&mp_type_OSError, "Parse model for yolo run failed");
    }

    if ((0x00 == self->parent->state.load_kmodel) || (0x00 == self->parent->state.run_kmodel))
    {
        mp_raise_msg(&mp_type_OSError, "Please load/run kmodel before");
    }

    float *output = NULL;
    size_t output_size = 0;
    if (0x00 != kpu_get_output(&self->parent->model.ctx, 0, (uint8_t **)&output, &output_size))
    {
        mp_raise_msg(&mp_type_OSError, "Failed to get kpu outputs");
    }

    yolo2_region_layer_t r;

    r.threshold = self->args.threshold;
    r.nms_value = self->args.nms_value;
    r.anchor_number = self->args.anchor_number;
    r.anchor = self->args.anchor;

    if (0x00 != yolo_region_layer_init(&r, &self->parent->shape.input, &self->parent->shape.output))
    {
        mp_raise_msg(&mp_type_OSError, "Yolo2 region layer init failed");
    }

    mp_obj_list_t *rect = m_new(mp_obj_list_t, 1);
    mp_obj_list_init(rect, 0);
 
    r.input = output;
    yolo_region_layer_run(&r);
    yolo_region_layer_get_rect(&r, rect);
    yolo_region_layer_deinit(&r);

    return MP_OBJ_FROM_PTR(rect);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_yolo2_run_obj, k210_kpu_yolo2_run);

STATIC mp_obj_t k210_kpu_yolo2_deinit(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_yolo2_type);
    mp_obj_k210_kpu_yolo2_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->args.anchor)
    {
        maix_kpu_heler_del_mem_from_list(self->args.anchor);
    }

    memset(&self->args, 0, sizeof(self->args));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_yolo2_deinit_obj, k210_kpu_yolo2_deinit);

STATIC mp_obj_t k210_kpu_yolo2_attr(mp_obj_t self_in)
{
    PY_ASSERT_TYPE(self_in, &k210_kpu_yolo2_type);
    mp_obj_k210_kpu_yolo2_t *self = MP_OBJ_TO_PTR(self_in);

    mp_obj_t args = mp_obj_new_dict(0);
    mp_obj_dict_store(args, MP_OBJ_NEW_QSTR(MP_QSTR_threshold), mp_obj_new_float(self->args.threshold));
    mp_obj_dict_store(args, MP_OBJ_NEW_QSTR(MP_QSTR_nms_value), mp_obj_new_float(self->args.nms_value));

    mp_obj_t anchors = mp_obj_new_list(self->args.anchor_number * 2, NULL);
    for(uint32_t i = 0; i < (self->args.anchor_number * 2); i++)
    {
        mp_obj_list_store(anchors, mp_obj_new_int(i), mp_obj_new_float(self->args.anchor[i]));
    }
    mp_obj_dict_store(args, MP_OBJ_NEW_QSTR(MP_QSTR_anchor_num), mp_obj_new_int(self->args.anchor_number));
    mp_obj_dict_store(args, MP_OBJ_NEW_QSTR(MP_QSTR_anchor), anchors);

    mp_obj_t o = mp_obj_new_dict(0);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_args), args);
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_self), mp_obj_new_int((mp_uint_t)self));
    mp_obj_dict_store(o, MP_OBJ_NEW_QSTR(MP_QSTR_parent), mp_obj_new_int((mp_uint_t)self->parent));

    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(k210_kpu_yolo2_attr_obj, k210_kpu_yolo2_attr);

STATIC const mp_rom_map_elem_t k210_kpu_yolo2_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init),     MP_ROM_PTR(&k210_kpu_yolo2_init_obj) },
    {MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&k210_kpu_yolo2_deinit_obj) },
    {MP_ROM_QSTR(MP_QSTR_run),      MP_ROM_PTR(&k210_kpu_yolo2_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_attr),    MP_ROM_PTR(&k210_kpu_yolo2_attr_obj) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_yolo2_locals_dict, k210_kpu_yolo2_locals_dict_table);

STATIC const mp_obj_type_t k210_kpu_yolo2_type = {
    { &mp_type_type },
    .name = MP_QSTR_yolo2,
    .print = k210_kpu_yolo2_print,
    .locals_dict = (void*)&k210_kpu_yolo2_locals_dict,
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
