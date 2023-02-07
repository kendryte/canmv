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

#include <mp.h>
#include "py_assert.h"
#include "py/qstr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "kpu.h"
#include "sleep.h"

#include "yolo2_region_layer.h"
#include "kpu_algorithm.h"

#include "vfs_wrapper.h"
#include "py_image.h"

///////////////////////////////////////////////////////////////////////////////
//#define DISABLE_CAHACE_ADDRESS
#ifdef DISABLE_CAHACE_ADDRESS
#define NO_CAHACE_ADDRESS(a) ((a)-0x40000000)
#else
#define NO_CAHACE_ADDRESS(a) (a)
#endif

#define MAX_MODEL_NUM 8

extern int load_file_from_flash(uint32_t addr, uint8_t *data_buf, uint32_t length);
extern int load_file_from_ff(const char *path, void* buffer, size_t model_size);
extern mp_uint_t get_file_size(const char *path);

typedef struct _kpu_km_ptr_t
{
    void *km_ptr[MAX_MODEL_NUM];
    size_t index;
} __attribute__((aligned(8))) kpu_km_ptr_t;

static kpu_km_ptr_t g_km_ptr_t = {
    .km_ptr = {NULL},
    .index = 0,
};

typedef struct _k210_kpu_obj_t
{
    mp_obj_base_t base; 
    kpu_model_context_t*  kmodel_ctx;
    size_t     model_size;
    mp_obj_t   model_buffer;
    mp_obj_t   model_path;
    uint32_t   inputs;
    mp_obj_t   inputs_addr;
    uint32_t   outputs;
    mp_obj_t   *output;   
    size_t     *output_size;
    mp_obj_t   user_buffer;
    yolo2_region_layer_t *yolo2_rl;

} __attribute__((aligned(8))) k210_kpu_obj_t;

const mp_obj_type_t k210_kpu_type;
extern const mp_obj_type_t py_image_type;

STATIC mp_obj_t py_kpu_deinit(mp_obj_t self_in);


struct model_header
{
    uint32_t identifier;
    uint32_t version;
    uint32_t flags;
    uint32_t target;    //enum model_target : uint32_t
    uint32_t constants;
    uint32_t main_mem;
    uint32_t nodes;
    uint32_t inputs;
    uint32_t outputs;
    uint32_t reserved0;
} __attribute__((aligned(8))) ;

volatile uint32_t wait_kpu_done = 0;
volatile uint32_t g_ai_done_flag = 0;
static void ai_done(void *ctx)
{
    // wait_kpu_done = 0;
    g_ai_done_flag = 1;
}

//manage kpu model buffer ptr, situation as a whole
static int kpu_model_buffer_add_ptr(void *ptr)
{
    if(g_km_ptr_t.index >= MAX_MODEL_NUM){
        g_km_ptr_t.index = 0;
    }
    if(g_km_ptr_t.km_ptr[g_km_ptr_t.index] == NULL){
        g_km_ptr_t.km_ptr[g_km_ptr_t.index] = ptr;
        //mp_printf(&mp_plat_print, "km ptr %x\r\n", g_km_ptr_t.km_ptr[g_km_ptr_t.index]);
        g_km_ptr_t.index += 1;
        return 0;
    }
    return -1;
}
static int kpu_model_buffer_del_ptr(void *ptr)
{
    for(uint32_t i= 0; i < MAX_MODEL_NUM; i++){
        if(g_km_ptr_t.km_ptr[i] == ptr){
            g_km_ptr_t.km_ptr[i] = NULL;
            return 0;
        }
    }
    return -1;
}
int kpu_model_buffer_free_all_ptr(void)
{
    int num = 0;
    k210_kpu_obj_t *km;

    for(uint32_t i= 0; i < MAX_MODEL_NUM; i++){
        if(g_km_ptr_t.km_ptr[i] != NULL){
            km = (k210_kpu_obj_t *)g_km_ptr_t.km_ptr[i];
            g_km_ptr_t.km_ptr[i] = NULL;
            if(py_kpu_deinit(km) == mp_const_false)
                continue;
            num ++;
        }
    }
    g_km_ptr_t.index = 0;

    return num;
}

STATIC mp_obj_t k210_kpu_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    
    k210_kpu_obj_t *self = m_new_obj(k210_kpu_obj_t);
    self->base.type=&k210_kpu_type;
    self->kmodel_ctx = m_new_obj(kpu_model_context_t);
    self->model_size = 0;
    self->model_buffer = NULL;
    self->model_path = NULL;
    self->output = NULL;
    self->output_size = NULL;
    self->inputs = 0;
    self->outputs = 0;
    self->inputs_addr = NULL;
    self->user_buffer = NULL; //自定义使用buf
    self->yolo2_rl = NULL;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t py_kpu_load_kmodel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)pos_args[0];
      enum
    {
        ARG_path,
        ARG_size,
    };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_path, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        {MP_QSTR_size, MP_ARG_INT, {.u_int = 0}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    if(args[ARG_path].u_obj == mp_const_none){
        mp_raise_ValueError("invalid path input");
    }

    if(mp_obj_get_type(args[ARG_path].u_obj) == &mp_type_str)
    {
        const char *path = mp_obj_str_get_str(args[ARG_path].u_obj);
        km->model_path = mp_obj_new_str(path,strlen(path));     
        km->model_size = get_file_size(path);

        mp_printf(&mp_plat_print, "model size %d \r\n", km->model_size);
        km->model_buffer = malloc(km->model_size);
        if (!km->model_buffer) {
            mp_raise_msg(&mp_type_MemoryError, "model buffer memory allocation failed");
            return mp_const_none;
        }

        if(load_file_from_ff(path, km->model_buffer, km->model_size) != 0)
        {
            free(km->model_buffer);
            km->model_buffer = NULL;
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to read file"));
        }
    }
    else if(mp_obj_get_type(args[ARG_path].u_obj) == &mp_type_int){
        if(mp_obj_get_int(args[ARG_path].u_obj)<=0){
            mp_raise_ValueError("path error!");
        }
        if(args[ARG_size].u_int <= 0){
            mp_raise_ValueError("size error!");
        }
        uint32_t path_addr;
        path_addr = mp_obj_get_int(args[ARG_path].u_obj);
        km->model_path = (mp_obj_t)path_addr;
        km->model_size = args[ARG_size].u_int;
        km->model_buffer = malloc(km->model_size);
        if (!km->model_buffer) {
            mp_raise_msg(&mp_type_MemoryError, "model buffer memory allocation failed");
            return mp_const_none;
        }
        if(load_file_from_flash( path_addr, km->model_buffer, km->model_size) != 0)
        {
            free(km->model_buffer);
            km->model_buffer = NULL;
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to read file"));
        }
    }
    else
    {
        mp_raise_ValueError("path error!");
    }

    if (kpu_load_kmodel(km->kmodel_ctx, km->model_buffer) != 0){
        free(km->model_buffer);
        km->model_buffer = NULL;
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to init model"));
    }

    if(kpu_model_buffer_add_ptr(km) != 0){
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to add kpu_model_buffer"));
    }
    mp_printf(&mp_plat_print, "model load succeed\r\n");  //debug
    if(km->kmodel_ctx->is_nncase){
        km->inputs = ((struct model_header*)km->model_buffer)->inputs;
        km->outputs = ((struct model_header*)km->model_buffer)->outputs;
    }
    else{
        km->inputs = 0;
        km->outputs = ((kpu_kmodel_header_t*)km->model_buffer)->output_count;
    }
    km->output_size = m_new(size_t, km->outputs);
    km->output = m_new(mp_obj_t,km->outputs);  

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_load_kmodel_obj,2, py_kpu_load_kmodel);

STATIC mp_obj_t py_kpu_run_with_output(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)pos_args[0];

     enum
    {
        ARG_input,
        ARG_getlist,
        ARG_get_feature,
    };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_input, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        {MP_QSTR_getlist, MP_ARG_BOOL, {.u_bool = 0}},
        {MP_QSTR_get_feature, MP_ARG_BOOL, {.u_bool = 0}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

////////// kpu run
    if(args[ARG_input].u_obj == mp_const_none){
        mp_raise_ValueError("invalid input");
    }
    if(mp_obj_get_type(args[ARG_input].u_obj) == &mp_type_str){
        const char *path = mp_obj_str_get_str(args[ARG_input].u_obj);
        mp_uint_t f_size = get_file_size(path);
        km->inputs_addr = m_malloc(f_size);
        if(load_file_from_ff(path, km->inputs_addr, f_size) != 0)
        {
            m_free(km->inputs_addr, f_size);
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to read file"));
        }
    }
    else if(mp_obj_get_type(args[ARG_input].u_obj) == &py_image_type){
        image_t* kimage = py_image_cobj(args[ARG_input].u_obj);
        if(kimage->pix_ai == NULL)
            mp_raise_msg(&mp_type_OSError, "Image formart error, use pix_to_ai() method to convert for kpu");
        km->inputs_addr = kimage->pix_ai;
    }
    else{
        mp_raise_ValueError("invalid input");
    }

    dmac_channel_number_t dma_ch = DMAC_CHANNEL5;//DMAC_CHANNEL_MAX;
    wait_kpu_done = 1;
    g_ai_done_flag = 0;
    if(0x00 != kpu_run_kmodel(km->kmodel_ctx, (uint8_t *)NO_CAHACE_ADDRESS(km->inputs_addr), dma_ch, ai_done, NULL)) {
        wait_kpu_done = 0;
        mp_raise_msg(&mp_type_OSError, "Model Buffer maybe dirty!");
    }

    while (!g_ai_done_flag);
    g_ai_done_flag = 0;

    dmac_free_irq(dma_ch);

    wait_kpu_done = 0;

/////////// kpu get output
    mp_obj_list_t *ret_list = NULL;
    kpu_get_output(km->kmodel_ctx, 0, (uint8_t **)&(km->output[0]), &(km->output_size[0]));
    int output_count = (km->output_size[0])/sizeof(float);
    //mp_printf(&mp_plat_print, "output_size:%d\r\n", (km->output_size[0])/sizeof(float));
    if(args[ARG_getlist].u_bool){
        ret_list = m_new(mp_obj_list_t, 1);
        mp_obj_list_init(ret_list, 0);

        for(int j = 0; j < (km->output_size[0])/sizeof(float); j++){
            mp_obj_list_append(ret_list, mp_obj_new_float( ((float*)km->output[0])[j] ));
        }
    }
    else if(args[ARG_get_feature].u_bool){
        float feature_tmp[MAX_FEATURE_LEN];
        ret_list = m_new(mp_obj_list_t, 1);
        mp_obj_list_init(ret_list, 0);
        //mp_printf(&mp_plat_print, "feature len %d \r\n", output_count);
        if(output_count > MAX_FEATURE_LEN){
		    mp_raise_ValueError("feature len out of 256\r\n");
	    }
        l2normalize((float*)km->output[0], feature_tmp, output_count);

        for(int j = 0; j < output_count; j++){
            mp_obj_list_append(ret_list, mp_obj_new_float(feature_tmp[j]) );
        }
    }

    if(args[ARG_getlist].u_bool || args[ARG_get_feature].u_bool){
        wait_kpu_done = 0;
        return MP_OBJ_FROM_PTR(ret_list);
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_kpu_run_with_output_obj, 1, py_kpu_run_with_output);

STATIC mp_obj_t py_init_yolo2(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)pos_args[0];
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
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    float threshold = 0.7;
    float nms_value = 0.4;
    float *anchor;
    if(args[ARG_threshold].u_obj != mp_const_none){
        threshold = mp_obj_get_float(args[ARG_threshold].u_obj);
    }
    if(args[ARG_nms_value].u_obj != mp_const_none){
        nms_value = mp_obj_get_float(args[ARG_nms_value].u_obj);
    }
    if(args[ARG_anchor].u_obj != mp_const_none){
        size_t nitems = 0;
        const mp_obj_t *items = 0;
        mp_obj_get_array(args[ARG_anchor].u_obj, &nitems, (mp_obj_t **)&items);
        anchor = m_new(float,nitems);
        for (int i = 0; i < nitems; i++){
            anchor[i] = mp_obj_get_float(*items++);
        }
    }
    else{
        //anchor = default_anchor;
        mp_raise_ValueError("need input anchor");
    }
    km->yolo2_rl = m_new(yolo2_region_layer_t,1);
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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_init_yolo2_obj, 2, py_init_yolo2);

STATIC mp_obj_t py_feature_compare(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_obj_t feature0_obj, feature1_obj;
    if(3 == n_args && mp_obj_get_type(pos_args[0]) == &k210_kpu_type){
        feature0_obj = pos_args[1];
        feature1_obj = pos_args[2];
    }
    else if(2 == n_args && mp_obj_get_type(pos_args[0]) != &k210_kpu_type){
        feature0_obj = pos_args[0];
        feature1_obj = pos_args[1];
    }
    else{
        mp_raise_ValueError("need input 2 feature");
    }
    float *feature0;
    float *feature1;
    size_t feature0_len = 0;
    size_t feature1_len = 0;
    const mp_obj_t *f0_items = 0;
    const mp_obj_t *f1_items = 0;
    mp_obj_get_array(feature0_obj, &feature0_len, (mp_obj_t **)&f0_items);
    mp_obj_get_array(feature1_obj, &feature1_len, (mp_obj_t **)&f1_items);
    if(feature0_len != feature1_len){
        mp_raise_ValueError("feature len error");
    }

    feature0 = m_new(float, feature0_len);
    for (int i = 0; i < feature0_len; i++){
        feature0[i] = mp_obj_get_float(*f0_items++);
    }
    feature1 = m_new(float, feature1_len);
    for (int i = 0; i < feature1_len; i++){
        feature1[i] = mp_obj_get_float(*f1_items++);
    }
    
    float score = calCosinDistance(feature0, feature1, feature0_len);
    return mp_obj_new_float(score);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_feature_compare_obj, 2, py_feature_compare);

STATIC mp_obj_t py_regionlayer_yolo2(mp_obj_t self_in)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)self_in;

    mp_obj_list_t *rect = m_new(mp_obj_list_t, 1);
    mp_obj_list_init(rect, 0);
 
    km->yolo2_rl->input = (float*)(km->output[0]);
    yolo_region_layer_run(km->yolo2_rl);
    yolo_region_layer_get_rect(km->yolo2_rl, rect);

    return MP_OBJ_FROM_PTR(rect);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_regionlayer_yolo2_obj, py_regionlayer_yolo2);

#ifdef LP_RECOG_EN 
STATIC mp_obj_t py_lp_recog_load_weight_data(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)pos_args[0];
    if(mp_obj_get_type(pos_args[1]) == &mp_type_str)
    {
        mp_obj_t path_obj = pos_args[1];
        const char *path = mp_obj_str_get_str(path_obj);  
        size_t weight_data_size = get_file_size(path);

        mp_printf(&mp_plat_print, "weight_data_size: %d\r\n", weight_data_size);
        km->user_buffer = malloc(weight_data_size);
        if (!km->user_buffer) {
            mp_raise_msg(&mp_type_MemoryError, "weight data buffer memory allocation failed");
            return mp_const_none;
        }

        if(load_file_from_ff(path, km->user_buffer, weight_data_size) != 0)
        {
            free(km->user_buffer);
            km->user_buffer = NULL;
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to read file"));
        }
    }
    else if(mp_obj_get_type(pos_args[1]) == &mp_type_int){
        mp_obj_t path_obj = pos_args[1];
        mp_obj_t size_obj = pos_args[2];

        if(mp_obj_get_int(path_obj)<=0){
            mp_raise_ValueError("path error!");
        }
        if(mp_obj_get_int(size_obj) <= 0){
            mp_raise_ValueError("size error!");
        }
        uint32_t path_addr;
        path_addr = mp_obj_get_int(path_obj);
        size_t weight_data_size = mp_obj_get_int(size_obj);
        km->user_buffer = malloc(weight_data_size);
        mp_printf(&mp_plat_print, "weight_data_size: %d\r\n", weight_data_size);
        if (!km->user_buffer) {
            mp_raise_msg(&mp_type_MemoryError, "weight data buffer memory allocation failed");
            return mp_const_none;
        }
        if(load_file_from_flash(path_addr, km->user_buffer, weight_data_size) != 0)
        {
            free(km->user_buffer);
            km->user_buffer = NULL;
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Failed to read file"));
        }
    }
    else
    {
        mp_raise_ValueError("path error!");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lp_recog_load_weight_data_obj, 2, py_lp_recog_load_weight_data);

STATIC mp_obj_t py_lp_recog(mp_obj_t self_in)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)self_in;

    mp_obj_list_t *ret_list = m_new(mp_obj_list_t, sizeof(mp_obj_list_t));
    mp_obj_list_init(ret_list, 0);
    if(km->user_buffer)
        lp_recog_process((float*)km->output[0], km->output_size[0]/sizeof(float), (float*)km->user_buffer, ret_list);
    else
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "not find lp weight data, please load"));

    return MP_OBJ_FROM_PTR(ret_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_lp_recog_obj, py_lp_recog);
#endif

// 将x归一化(0-1)
STATIC mp_obj_t py_sigmoid(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_obj_t x_in;
    if(2 == n_args && mp_obj_get_type(pos_args[0]) == &k210_kpu_type){
        x_in = pos_args[1];
    }
    else if(1 == n_args && mp_obj_get_type(pos_args[0]) != &k210_kpu_type){
        x_in = pos_args[0];
    }
    else{
        mp_raise_ValueError(NULL);
    }

    mp_float_t  x = mp_obj_get_float(x_in);
    return mp_obj_new_float(sigmoid(x));

}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_sigmoid_obj, 1, py_sigmoid);

STATIC mp_obj_t py_softmax(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{    
    mp_obj_t li;

    if((2 == n_args) && (mp_obj_get_type(pos_args[0]) == &k210_kpu_type)) {
        li = pos_args[1];
    }
    else if((1 == n_args) && (mp_obj_get_type(pos_args[0]) != &k210_kpu_type)) {
        li = pos_args[0];
    }
    else {
        mp_raise_ValueError(NULL);
    }

    PY_ASSERT_TYPE(li, &mp_type_list);

    size_t nitems = 0;
    const mp_obj_t *items = 0;

    mp_obj_get_array(li, &nitems, (mp_obj_t **)&items);

    float *dati = m_new(float, nitems);
    float *dato = m_new(float, nitems);

    for (int i = 0; i < nitems; i++) {
        dati[i] = mp_obj_get_float(*items++);
    }

    maix_kpu_helper_softmax(dati, dato, nitems);

    mp_obj_list_t *lo = m_new(mp_obj_list_t, 1);
    mp_obj_list_init(lo, 0);

    for (int i = 0; i < nitems; i++) {
        mp_obj_list_append(lo, mp_obj_new_float(dato[i]));
    }

    m_del(float, dati, nitems);
    m_del(float, dato, nitems);

    return MP_OBJ_FROM_PTR(lo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_softmax_obj, 1, py_softmax);

#if 0
//need IR camera to test
#define FACE_GENUINE_THRESH 0.90f
#define THETA_THRESH 15
#define RATIO_THRESH 3.0f
#define RATIO_MIN_THRESH 1.0f / RATIO_THRESH
#define PI 3.1415926f
STATIC mp_obj_t py_alive_detect(mp_obj_t afs_bb_in, mp_obj_t afs_st_in, mp_obj_t keypoint_list)
{
    uint16_t face_alive = 1;
    float afs_prob_bb[2] = {0};
    float afs_prob_st[2] = {0};
    uint16_t key_point[5][2];

    if(afs_bb_in != mp_const_none && afs_st_in != mp_const_none && MP_OBJ_IS_TYPE(keypoint_list, &mp_type_list)){
        float *bb, *st;
        size_t nitems = 0;
        const mp_obj_t *items = 0;
        mp_obj_get_array(afs_bb_in, &nitems, (mp_obj_t **)&items);
        bb = m_new(float,nitems);
        for (int i = 0; i < nitems; i++){
            bb[i] = mp_obj_get_float(*items++);
        }
        mp_obj_get_array(afs_st_in, &nitems, (mp_obj_t **)&items);
        st = m_new(float,nitems);
        for (int i = 0; i < nitems; i++){
            st[i] = mp_obj_get_float(*items++);
        }

        mp_uint_t src_l_len;
		mp_obj_t *src_l;
		mp_obj_get_array(keypoint_list, &src_l_len, &src_l);
        //PY_ASSERT_TRUE_MSG(src_l_len<=5, "must<=5 points");
        for(int i=0; i<src_l_len; i++) {
			mp_obj_t *tuple;
            mp_obj_get_array_fixed_n(src_l[i], 2, &tuple);
            key_point[i][0] = mp_obj_get_int(tuple[0]);
            key_point[i][1] = mp_obj_get_int(tuple[1]);
			//printf("point %d: (%d,%d)->(%d,%d)\r\n", i, src_pos[i][0],src_pos[i][1],dst_pos[i][0],dst_pos[i][1]);
		}

        local_softmax(bb, afs_prob_bb, 2);
        local_softmax(st, afs_prob_st, 2);

        float lenx_dis = (float)key_point[2][0] - (float)key_point[0][0] + 1;
        float renx_dis = (float)key_point[1][0] - (float)key_point[2][0] + 1;
        float lmnx_dis = (float)key_point[2][0] - (float)key_point[3][0] + 1;
        float rmnx_dis = (float)key_point[4][0] - (float)key_point[2][0] + 1;
        float lrx_ratio = (lenx_dis * lmnx_dis) / (renx_dis * rmnx_dis);
        float ex_dis = fabsf((float)key_point[1][0] - (float)key_point[0][0] + 1);
        float ey_dis = fabsf((float)key_point[1][1] - (float)key_point[0][1] + 1);
        float theta1 = atan2f(ey_dis, ex_dis) * 180 / PI;
        float mx_dis = fabsf((float)key_point[4][0] - (float)key_point[3][0] + 1);
        float my_dis = fabsf((float)key_point[4][1] - (float)key_point[3][1] + 1);
        float theta2 = atan2f(my_dis, mx_dis) * 180 / PI;
        face_alive = (afs_prob_bb[0] >= FACE_GENUINE_THRESH) && (afs_prob_st[0] >= FACE_GENUINE_THRESH) && (lrx_ratio >= RATIO_MIN_THRESH) 
                     && (lrx_ratio <= RATIO_THRESH) && (theta1 >= 0) && (theta1 <= THETA_THRESH) && (theta2 >= 0) && (theta2 <= THETA_THRESH);
        return mp_obj_new_int(face_alive);
    }
    else{
        mp_raise_ValueError(NULL);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_alive_detect_obj, py_alive_detect);
#endif

STATIC mp_obj_t py_kpu_deinit(mp_obj_t self_in)
{
    if(mp_obj_get_type(self_in) == &k210_kpu_type)
    {
        k210_kpu_obj_t *km = (k210_kpu_obj_t *)self_in;
        if(km->user_buffer){
            free(km->user_buffer);
            km->user_buffer = NULL;
        }
        if(km->model_buffer){
            free(km->model_buffer);
            km->model_buffer = NULL;
            kpu_model_free(km->kmodel_ctx);
            kpu_model_buffer_del_ptr(km);
            mp_printf(&mp_plat_print, "free kpu model buf succeed\r\n");
        }
        else
            return mp_const_false;

        m_del_obj(kpu_model_context_t,km->kmodel_ctx);
        yolo_region_layer_deinit(km->yolo2_rl);
        m_del(float,km->yolo2_rl->anchor,km->yolo2_rl->anchor_number);
        // m_del(float,km->yolo2_rl->anchor,1);
        m_del(yolo2_region_layer_t,km->yolo2_rl,1);
        m_del(size_t,km->output_size,km->outputs);
        m_del(mp_obj_t,km->output,km->outputs);
        return mp_const_true;
    }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_kpu_deinit_obj, py_kpu_deinit);


///////////////////////////////////////////////////////////////////////////////

STATIC void k210_kpu_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    k210_kpu_obj_t *km = (k210_kpu_obj_t *)self_in;
    mp_printf(print, "type        : kmodel (KPU) \n");
    mp_printf(print, "model_size  : %d\n", km->model_size);
    mp_printf(print, "inputs      : %d\n", km->inputs);
    mp_printf(print, "outputs     : %d\n", km->outputs);

}

STATIC const mp_rom_map_elem_t k210_kpu_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_load_kmodel), MP_ROM_PTR(&py_kpu_load_kmodel_obj)},
    {MP_ROM_QSTR(MP_QSTR_run_with_output), MP_ROM_PTR(&py_kpu_run_with_output_obj)},
    {MP_ROM_QSTR(MP_QSTR_regionlayer_yolo2), MP_ROM_PTR(&py_regionlayer_yolo2_obj)},
    {MP_ROM_QSTR(MP_QSTR_init_yolo2), MP_ROM_PTR(&py_init_yolo2_obj)},
    {MP_ROM_QSTR(MP_QSTR_deinit),  MP_ROM_PTR(&py_kpu_deinit_obj) },
    {MP_ROM_QSTR(MP_QSTR_sigmoid),  MP_ROM_PTR(&py_sigmoid_obj) },
    {MP_ROM_QSTR(MP_QSTR_softmax),  MP_ROM_PTR(&py_softmax_obj) },
    {MP_ROM_QSTR(MP_QSTR_feature_compare),  MP_ROM_PTR(&py_feature_compare_obj) },
#ifdef LP_RECOG_EN 
    {MP_ROM_QSTR(MP_QSTR_lp_recog_load_weight_data),  MP_ROM_PTR(&py_lp_recog_load_weight_data_obj) },
    {MP_ROM_QSTR(MP_QSTR_lp_recog),  MP_ROM_PTR(&py_lp_recog_obj) },
#endif
    //{MP_ROM_QSTR(MP_QSTR_alive_detect),  MP_ROM_PTR(&py_alive_detect_obj) },
};
STATIC MP_DEFINE_CONST_DICT(k210_kpu_dict, k210_kpu_locals_dict_table);

const mp_obj_type_t k210_kpu_type = {
    {&mp_type_type},
    .name = MP_QSTR_KPU,
    .print = k210_kpu_print,
    .make_new = k210_kpu_make_new,
    .locals_dict = (mp_obj_dict_t *)&k210_kpu_dict,
};
