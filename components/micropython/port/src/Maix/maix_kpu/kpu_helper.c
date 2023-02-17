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

#include "kpu_helper.h"

#include "nncase.h"

#include "atomic.h"

#include "vfs_wrapper.h"
#include "vfs_spiffs.h"

int maix_kpu_helper_probe_model_size(uint8_t *model_buffer, uint32_t buffer_size)
{
    uint32_t body_size = 0;

    const kpu_kmodel_header_t *header = (const kpu_kmodel_header_t *)model_buffer;

    if (header->version == 3 && header->arch == 0)
    {
        kpu_model_context_t ctx;

        ctx.output_count = header->output_count;
        ctx.outputs = (const kpu_model_output_t *)(model_buffer + sizeof(kpu_kmodel_header_t));
        ctx.layer_headers = (const kpu_model_layer_header_t *)((uintptr_t)ctx.outputs + sizeof(kpu_model_output_t) * ctx.output_count);
        ctx.layers_length = header->layers_length;
        ctx.body_start = (const uint8_t *)((uintptr_t)ctx.layer_headers + sizeof(kpu_model_layer_header_t) * header->layers_length);

        body_size = (uint32_t)(ctx.body_start - (const uint8_t *)header);

        if(body_size > buffer_size)
        {
            return -1;
        }

        for(int i=0; i< ctx.layers_length; i++)
        {
            const kpu_model_layer_header_t *cnt_layer_header = ctx.layer_headers + i;
            body_size += cnt_layer_header->body_size;
        }

        return body_size;
    }
    else if(header->version == 'KMDL')
    {
        body_size = nncase_probe_model_buffer_size(model_buffer, buffer_size);

        return body_size;
    }

    return -1;
}

int maix_kpu_helper_get_input_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w)
{
    if(chn) *chn = 0;
    if(h) *h = 0;
    if(w) *w = 0;

    if(ctx->is_nncase)
    {
        return nncase_get_input_shape(ctx, 0, chn, h, w);
    }
    else
    {
        const kpu_model_layer_header_t *first_layer_header = ctx->layer_headers;

        if(KL_K210_CONV == first_layer_header->type)
        {
            const kpu_model_conv_layer_argument_t *first_layer = (const kpu_model_conv_layer_argument_t *)ctx->body_start;
            kpu_layer_argument_t layer_arg = *(volatile kpu_layer_argument_t *)(ctx->model_buffer + first_layer->layer_offset);

            if(chn) *chn = 1 + layer_arg.image_channel_num.data.i_ch_num;
            if(h) *h = 1 + layer_arg.image_size.data.i_col_high;
            if(w) *w = 1 + layer_arg.image_size.data.i_row_wid;

            return 0;
        }
        else if(KL_FULLY_CONNECTED == first_layer_header->type)
        {
            // TODO: get input shape.
            return -1;
        }
    }

    return -1;
}

int maix_kpu_helper_get_output_count(kpu_model_context_t *ctx)
{
    if(ctx->is_nncase)
    {
        return nncase_get_output_count(ctx);
    }

    return ctx->output_count;
}

int maix_kpu_helper_get_output_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w)
{
    if(chn) *chn = 0;
    if(h) *h = 0;
    if(w) *w = 0;

    if(ctx->is_nncase)
    {
        return nncase_get_output_shape(ctx, chn, h, w);
    }
    else
    {
        const kpu_model_layer_header_t *_layer = NULL;
        const kpu_model_layer_header_t *output_layer = ctx->layer_headers + ctx->layers_length - 1;
        const kpu_model_layer_header_t *conv2_layer = ctx->layer_headers + ctx->layers_length - 2;

        if((KL_DEQUANTIZE != output_layer->type) || (KL_K210_CONV != conv2_layer->type))
        {
            return -1;
        }

        const uint8_t *body = ctx->body_start;
        for(int i = 0; i < (ctx->layers_length - 2); i++)
        {
            _layer = ctx->layer_headers + i;
            body += _layer->body_size;
        }

        if(KL_K210_CONV == _layer->type)
        {
            const kpu_model_conv_layer_argument_t *conv_layer = (const kpu_model_conv_layer_argument_t *)body;
            kpu_layer_argument_t layer_arg = *(volatile kpu_layer_argument_t *)(ctx->model_buffer + conv_layer->layer_offset);

            if(chn) *chn = 1 + layer_arg.image_channel_num.data.o_ch_num;
            if(h) *h = 1 + layer_arg.image_size.data.o_col_high;
            if(w) *w = 1 + layer_arg.image_size.data.o_row_wid;

            return 0;
        }
    }

    return -1;
}

int maix_kpu_helper_load_file_from_rawflash(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    sys_spiffs_read(addr, length, data_buf);

    return 0;
}

int32_t maix_kpu_helper_get_mode_size_from_rawflash(uint32_t addr)
{
#define PROBE_MODEL_TEMP_BUFF_SIZE      (64 * 1024)

    uint8_t *ptr = (uint8_t *)malloc(PROBE_MODEL_TEMP_BUFF_SIZE);
    if(!ptr)
    {
        return -1;
    }

    sys_spiffs_read(addr, PROBE_MODEL_TEMP_BUFF_SIZE, ptr);

    int model_size = maix_kpu_helper_probe_model_size(ptr, PROBE_MODEL_TEMP_BUFF_SIZE);

    free(ptr);

    return model_size;

#undef PROBE_MODEL_TEMP_BUFF_SIZE
}

int maix_kpu_helper_load_file_from_filesystem(const char *path, void *buffer, size_t model_size)
{
    int ret = 0;
    mp_obj_t fp;
    file_read_open_raise(&fp, path);
    if (read_data(fp, buffer, model_size))
    {
        ret = -2;
    }
    file_close(fp);

    return ret;
}

int32_t maix_kpu_helper_get_file_size_from_filesystem(const char *path)
{
    mp_obj_t fp;
    int32_t size = 0;

    int err = file_read_open(&fp, path);

    if (fp == MP_OBJ_NULL || err != 0)
    {
        mp_raise_ValueError("open file error, please check file path!");
    }
    size = file_size(fp);
    file_close(fp);

    return (int32_t)size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static spinlock_t _mem_lock = SPINLOCK_INIT;

static kpu_used_mem_info_t _mem_table[64] = {
    [0 ... 63] = {NULL, 0}
};

static const int _mem_table_sz = sizeof(_mem_table) / sizeof(_mem_table[0]);

static int maix_kpu_helper_find_mem_in_list(kpu_used_mem_info_t *mem)
{
    spinlock_lock(&_mem_lock);
    
    for(int i = 0; i < _mem_table_sz; i++) {
        if(_mem_table[i].ptr == mem->ptr) {
            spinlock_unlock(&_mem_lock);

            return i;
        }
    }

    spinlock_unlock(&_mem_lock);

    return -1;
}

int maix_kpu_helper_add_mem_to_list(kpu_used_mem_info_t *mem)
{
    int idx = maix_kpu_helper_find_mem_in_list(mem);

    if(idx >= 0) {
        return 0;
    }

    spinlock_lock(&_mem_lock);

    for(int i = 0; i < _mem_table_sz; i++) {
        if(NULL == _mem_table[i].ptr) {
            _mem_table[i].ptr = mem->ptr;
            _mem_table[i].type = mem->type;

            spinlock_unlock(&_mem_lock);

            return 0;
        }
    }

    spinlock_unlock(&_mem_lock);

    return -1;
}

// not use lock.
static inline void maix_kpu_helper_free_mem(kpu_used_mem_info_t *mem)
{
    if(MEM_TYPE_PTR == mem->type) {
        free(mem->ptr);
        mem->ptr = NULL;
    }
}

int maix_kpu_heler_del_mem_from_list(void *ptr)
{
    kpu_used_mem_info_t mem = {ptr, MEM_TYPE_PTR};

    int idx = maix_kpu_helper_find_mem_in_list(&mem);

    if(idx < 0) {
        return -1;
    }

    spinlock_lock(&_mem_lock);

    maix_kpu_helper_free_mem(&_mem_table[idx]);

    spinlock_unlock(&_mem_lock);

    return 0;
}

int maix_kpu_helper_free_mem_list(void)
{
    int cnt = 0;
    spinlock_lock(&_mem_lock);

    for(int i = 0; i < _mem_table_sz; i++) {
        if(_mem_table[i].ptr) {
            cnt++;
            maix_kpu_helper_free_mem(&_mem_table[i]);
        }
    }

    spinlock_unlock(&_mem_lock);

    return cnt;
}
