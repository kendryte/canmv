/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _KPU_H
#define _KPU_H

#include <stdint.h>
#include <plic.h>
#include "dmac.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kpu_matmul_begin kpu_conv2d_output

typedef int (*plic_irq_callback_t)(void *ctx);

typedef struct
{
    union
    {
        uint64_t reg;
        struct
        {
            uint64_t int_en:1;
            uint64_t ram_flag:1;
            uint64_t full_add:1;
            uint64_t depth_wise_layer:1;
            uint64_t reserved:60;
        } data;
    } interrupt_enabe;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t image_src_addr:15;
            uint64_t reserved0:17;
            uint64_t image_dst_addr:15;
            uint64_t reserved1:17;
        } data;
    } image_addr;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t i_ch_num:10;
            uint64_t reserved0:22;
            uint64_t o_ch_num:10;
            uint64_t reserved1:6;
            uint64_t o_ch_num_coef:10;
            uint64_t reserved2:6;
        } data;
    } image_channel_num;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t i_row_wid:10;
            uint64_t i_col_high:9;
            uint64_t reserved0:13;
            uint64_t o_row_wid:10;
            uint64_t o_col_high:9;
            uint64_t reserved1:13;
        } data;
    } image_size;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t kernel_type:3;
            uint64_t pad_type:1;
            uint64_t pool_type:4;
            uint64_t first_stride:1;
            uint64_t bypass_conv:1;
            uint64_t load_para:1;
            uint64_t reserved0:5;
            uint64_t dma_burst_size:8;
            uint64_t pad_value:8;
            uint64_t bwsx_base_addr:32;
        } data;
    } kernel_pool_type_cfg;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t load_coor:1;
            uint64_t load_time:6;
            uint64_t reserved0:8;
            uint64_t para_size:17;
            uint64_t para_start_addr:32;
        } data;
    } kernel_load_cfg;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t coef_column_offset:4;
            uint64_t coef_row_offset:12;
            uint64_t reserved0:48;
        } data;
    } kernel_offset;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t channel_switch_addr:15;
            uint64_t reserved:1;
            uint64_t row_switch_addr:4;
            uint64_t coef_size:8;
            uint64_t coef_group:3;
            uint64_t load_act:1;
            uint64_t active_addr:32;
        } data;
    } kernel_calc_type_cfg;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t wb_channel_switch_addr:15;
            uint64_t reserved0:1;
            uint64_t wb_row_switch_addr:4;
            uint64_t wb_group:3;
            uint64_t reserved1:41;
        } data;
    } write_back_cfg;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t shr_w:4;
            uint64_t shr_x:4;
            uint64_t arg_w:24;
            uint64_t arg_x:24;
            uint64_t reserved0:8;
        } data;
    } conv_value;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t arg_add:40;
            uint64_t reserved:24;
        } data;
    } conv_value2;

    union
    {
        uint64_t reg;
        struct
        {
            uint64_t send_data_out:1;
            uint64_t reserved:15;
            uint64_t channel_byte_num:16;
            uint64_t dma_total_byte:32;
        } data;
    } dma_parameter;
} kpu_layer_argument_t;

typedef struct
{
    union
    {
        uint64_t reg;
        struct
        {
            uint64_t shift_number:8;
            uint64_t y_mul:16;
            uint64_t x_start:36;
        } data;
    } activate_para[16];

    union
    {
        uint64_t reg;
        struct
        {
            uint8_t result_bias[8];
        } data;
    } activate_para_bias0;

    union
    {
        uint64_t reg;
        struct
        {
            uint8_t result_bias[8];
        } data;
    } activate_para_bias1;
} kpu_activate_table_t;

typedef struct
{
    union
    {
        uint64_t reg;
        struct
        {
            uint64_t norm_mul:24;
            uint64_t norm_add:32;
            uint64_t norm_shift:4;
        } data;
    } batchnorm;
} kpu_batchnorm_argument_t;


typedef struct
{
    union
    {
        uint64_t reg;
        struct
        {
            uint16_t weight[9];
        } data;
    } weights;
} kpu_weights_kernel_16_3x3_t;

typedef struct
{
    uint64_t calc_done_int:1;
    uint64_t layer_cfg_almost_empty_int:1;
    uint64_t layer_cfg_almost_full_int:1;
    uint64_t reserved:61;
} kpu_config_interrupt_t;

typedef struct
{
    uint64_t fifo_full_threshold:4;
    uint64_t fifo_empty_threshold:4;
    uint64_t reserved:56;
} kpu_config_fifo_threshold_t;

typedef struct
{
    uint64_t dma_fifo_flush_n:1;
    uint64_t gs_fifo_flush_n:1;
    uint64_t cfg_fifo_flush_n:1;
    uint64_t cmd_fifo_flush_n:1;
    uint64_t resp_fifo_flush_n:1;
    uint64_t reserved:59;
} kpu_config_fifo_ctrl_t;

typedef struct
{
    uint64_t eight_bit_mode:1;
    uint64_t reserved:63;
} kpu_config_eight_bit_mode_t;


typedef struct
{
    volatile uint64_t layer_argument_fifo;

    volatile union
    {
        uint64_t reg;
        kpu_config_interrupt_t data;
    } interrupt_status;

    volatile  union
    {
        uint64_t reg;
        kpu_config_interrupt_t  data;
    } interrupt_raw;

    volatile  union {
        uint64_t reg;
        kpu_config_interrupt_t  data;
    } interrupt_mask;

    volatile  union
    {
        uint64_t reg;
        kpu_config_interrupt_t data;
    } interrupt_clear;

    volatile  union
    {
        uint64_t reg;
        kpu_config_fifo_threshold_t  data;
    } fifo_threshold;

    volatile uint64_t fifo_data_out;

    volatile  union
    {
        uint64_t reg;
        kpu_config_fifo_ctrl_t  data;
    } fifo_ctrl;

    volatile  union
    {
        uint64_t reg;
        kpu_config_eight_bit_mode_t  data;
    } eight_bit_mode;
} kpu_config_t;

typedef struct
{
    kpu_layer_argument_t *layers;
    kpu_layer_argument_t *remain_layers;
    plic_irq_callback_t callback;
    void *ctx;
    uint64_t *src;
    uint64_t *dst;
    uint32_t src_length;
    uint32_t dst_length;
    uint32_t layers_length;
    uint32_t remain_layers_length;
    dmac_channel_number_t dma_ch;
    uint32_t eight_bit_mode;
    float output_scale;
    float output_bias;
    float input_scale;
    float input_bias;
} kpu_task_t;

typedef struct
{
    uint32_t version;
    uint32_t flags;
    uint32_t arch;
    uint32_t layers_length;
    uint32_t max_start_address;
    uint32_t main_mem_usage;
    uint32_t output_count;
} kpu_kmodel_header_t;

typedef struct
{
    uint32_t version;
    uint32_t flags;
    uint32_t layers_length;
    uint32_t max_start_address;
    uint32_t layers_argument_start;
} kpu_model_header_t;

typedef struct
{
    uint32_t address;
    uint32_t size;
} kpu_model_output_t;

typedef enum
{
    KL_INVALID = 0,
    KL_ADD,
    KL_QUANTIZED_ADD,
    KL_GLOBAL_MAX_POOL2D,
    KL_QUANTIZED_GLOBAL_MAX_POOL2D,
    KL_GLOBAL_AVERAGE_POOL2D,
    KL_QUANTIZED_GLOBAL_AVERAGE_POOL2D,
    KL_MAX_POOL2D,
    KL_QUANTIZED_MAX_POOL2D,
    KL_AVERAGE_POOL2D,
    KL_QUANTIZED_AVERAGE_POOL2D,
    KL_QUANTIZE,
    KL_DEQUANTIZE,
    KL_REQUANTIZE,
    KL_L2_NORMALIZATION,
    KL_SOFTMAX,
    KL_CONCAT,
    KL_QUANTIZED_CONCAT,
    KL_FULLY_CONNECTED,
    KL_QUANTIZED_FULLY_CONNECTED,
    KL_TENSORFLOW_FLATTEN,
    KL_QUANTIZED_TENSORFLOW_FLATTEN,
    KL_RESIZE_NEAREST_NEIGHBOR,
    KL_QUANTIZED_RESIZE_NEAREST_NEIGHBOR,
    KL_CHANNELWISE_DEQUANTIZE,
    KL_LOGISTIC,
    KL_K210_CONV = 10240,
    KL_K210_ADD_PADDING,
    KL_K210_REMOVE_PADDING,
    KL_K210_UPLOAD
} kpu_model_layer_type_t;

typedef struct
{
    uint32_t type;
    uint32_t body_size;
} kpu_model_layer_header_t;

typedef enum
{
    KLF_NONE = 0,
    KLF_MAIN_MEM_OUT = 1
} kpu_model_layer_flags_t;

typedef enum
{
    KLP_SAME = 0,
    KLP_VALID = 1
} kpu_model_padding_t;

typedef enum
{
    KLA_LINEAR = 0,
    KLA_RELU = 1,
    KLA_RELU6 = 2
} kpu_model_activation_t;

typedef struct
{
    float scale;
	float bias;
} kpu_model_quant_param_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;
} kpu_model_shape_t;

typedef struct
{
    uint32_t start;
    uint32_t size;
} kpu_model_memory_range_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_out_address;
    uint32_t layer_offset;
    uint32_t weights_offset;
    uint32_t bn_offset;
    uint32_t act_offset;
} kpu_model_conv_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_a_address;
    uint32_t main_mem_in_b_address;
    uint32_t main_mem_out_address;
    uint32_t count;
} kpu_model_add_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_a_address;
    uint32_t main_mem_in_b_address;
    uint32_t main_mem_out_address;
    uint32_t count;
    int32_t in_a_offset;
    int32_t in_a_mul;
    int32_t in_a_shift;
    int32_t in_b_offset;
    int32_t in_b_mul;
    int32_t in_b_shift;
    int32_t out_offset;
    int32_t out_mul;
    int32_t out_shift;
} kpu_model_quant_add_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t kernel_size;
    uint32_t channels;
} kpu_model_gap2d_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    kpu_model_shape_t in_shape;
    kpu_model_shape_t out_shape;
    uint32_t kernel_width;
    uint32_t kernel_height;
    uint32_t stride_width;
    uint32_t stride_height;
    uint32_t padding_width;
    uint32_t padding_height;
} kpu_model_quant_max_pool2d_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    kpu_model_shape_t in_shape;
    kpu_model_shape_t out_shape;
    uint32_t kernel_width;
    uint32_t kernel_height;
    uint32_t stride_width;
    uint32_t stride_height;
    uint32_t padding_width;
    uint32_t padding_height;
    kpu_model_activation_t act;
} kpu_model_ave_pool2d_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t mem_out_address;
    uint32_t count;
    kpu_model_quant_param_t quant_param;
} kpu_model_quantize_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t count;
    kpu_model_quant_param_t quant_param;
} kpu_model_dequantize_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t count;
    uint8_t table[256];
} kpu_model_requantize_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t kpu_mem_out_address;
    uint32_t channels;
} kpu_model_add_padding_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t channels;
} kpu_model_remove_padding_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t kpu_mem_out_address;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
} kpu_model_upload_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t channels;
} kpu_model_l2_norm_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t channels;
} kpu_model_softmax_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_out_address;
    uint32_t input_count;
    kpu_model_memory_range_t inputs_mem[0];
} kpu_model_concat_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t in_channels;
    uint32_t out_channels;
    kpu_model_activation_t act;
    float weights[0];
} kpu_model_fully_connected_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    kpu_model_shape_t shape;
} kpu_model_tf_flatten_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    kpu_model_shape_t in_shape;
    uint32_t out_width;
    uint32_t out_height;
    uint32_t align_corners;
} kpu_model_resize_nearest_neighbor_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    kpu_model_shape_t in_shape;
    uint32_t out_width;
    uint32_t out_height;
    uint32_t align_corners;
} kpu_model_quant_resize_nearest_neighbor_layer_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t channels;
    uint32_t channel_size;
    kpu_model_quant_param_t quant_params[0];
} kpu_model_channelwise_dequant_argument_t;

typedef struct
{
    uint32_t flags;
    uint32_t main_mem_in_address;
    uint32_t main_mem_out_address;
    uint32_t channels;
} kpu_model_logistic_layer_argument_t;

typedef void(*kpu_done_callback_t)(void* userdata);

typedef struct
{
    int is_nncase;

    union
    {
        struct
        {
            const uint8_t *model_buffer;
            uint8_t *main_buffer;
            uint32_t output_count;
            const kpu_model_output_t *outputs;
            const kpu_model_layer_header_t *layer_headers;
            const uint8_t *body_start;
            uint32_t layers_length;
            volatile uint32_t current_layer;
            const uint8_t *volatile current_body;
            dmac_channel_number_t dma_ch;
            kpu_done_callback_t done_callback;
            void *userdata;
        };

        struct
        {
            void* nncase_ctx;
        };
    };
} kpu_model_context_t;

typedef struct
{
    uint32_t weigths_offset;
    uint32_t bn_offset;
    uint32_t act_offset;
    float input_scale;
    float input_bias;
    float output_scale;
    float output_bias;
} kpu_model_layer_metadata_t;

typedef struct _quantize_param
{
    float scale;
    float bias;
} quantize_param_t;

extern volatile kpu_config_t *const kpu;

/**
 * @brief       Modle complier init kpu handler
 *
 * @param[in]   task            Kpu handler
 *
 * @return      Kpu handler
 */
extern kpu_task_t *kpu_task_init(kpu_task_t* task);

/**
 * @brief       Kpu run for AI
 *
 * @param[in]   task                Kpu handler
 * @param[in]   dma_ch              DMA for kpu
 * @param[in]   src                 The picture data
 * @param[in]   dest                The result of kpu
 * @param[in]   callback            The callback of kpu
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.Kpu is busy.
 */
int kpu_run(kpu_task_t* task, dmac_channel_number_t dma_ch, const void *src, void* dest, plic_irq_callback_t callback);

/**
 * @brief       Get kpu result buf
 *
 * @param[in]   task                Kpu handler
 *
 * @return      Kpu result buf
 */
uint8_t *kpu_get_output_buf(kpu_task_t* task);

/**
 * @brief       Release kpu output buf
 *
 * @param[in]   output_buf                Kpu output buf
 *
 */
void kpu_release_output_buf(uint8_t *output_buf);

/**
 * @brief       Kpu run for AI
 *
 * @param[in]   task                Kpu handler
*
* @return      result
*     - 0      Success
*     - Other  Fail.Kpu is busy.
*/
int kpu_start(kpu_task_t *task);

/**
 * @brief      Initialize kpu handler
 *
 * @param[in]   task            Kpu handler
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.
 */
int kpu_single_task_init(kpu_task_t *task);

/**
 * @brief      Uninitialize kpu handler
 *
 * @param[in]   task            Kpu handler
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.
 */
int kpu_single_task_deinit(kpu_task_t *task);

/**
 * @brief      Load kmodel and init kpu task
 *
 * @param[in]   task            Kpu handler
 * @param[in]   buffer          Kmodel
 * @param[in]   meta            Test data
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.
 */
int kpu_model_load_from_buffer(kpu_task_t *task, uint8_t *buffer, kpu_model_layer_metadata_t **meta);

/**
 * @brief       Kpu initialize
 *
 * @param[in]   eight_bit_mode            0:16bit mode  1:8bit mode
 * @param[in]   callback                  Callback of kpu
 * @param[in]   userdata                  Data of callback
 *
 */
void kpu_init(int eight_bit_mode, plic_irq_callback_t callback, void *userdata);

/**
 * @brief       Kpu input data by dma
 *
 * @param[in]   layer                   Kpu task layer
 * @param[in]   src                     Image data
 * @param[in]   dma_ch                  Dmac channel
 * @param[in]   callback                Dmac complete callback
 * @param[in]   userdata                Data of callback
 *
 */
void kpu_input_dma(const kpu_layer_argument_t *layer, const uint8_t *src, dmac_channel_number_t dma_ch, plic_irq_callback_t callback, void *userdata);

/**
 * @brief       Kpu input data by cpu
 *
 * @param[in]   layer                   Kpu task layer
 * @param[in]   src                     Image data
 * @param[in]   width                   Image width
 * @param[in]   height                  Image heigth
 * @param[in]   channels                Color channel, RGB is 3
 *
 */
void kpu_input_with_padding(kpu_layer_argument_t *layer, const uint8_t *src, int width, int height, int channels);

/**
 * @brief       Kpu run only one layer
 *
 * @param[in]   layer                   Kpu task layer
 *
 */
void kpu_conv2d(kpu_layer_argument_t *layer);

/**
 * @brief       Kpu run only one layer then get the result by dma
 *
 * @param[in]   layer                   Kpu task layer
 * @param[in]   dma_ch                  Dmac channel
 * @param[in]   dest                    Result
 * @param[in]   callback                Dmac complete callback
 * @param[in]   userdata                Data of callback
 *
 */
void kpu_conv2d_output(kpu_layer_argument_t *layer, dmac_channel_number_t dma_ch, uint8_t *dest, plic_irq_callback_t callback, void *userdata);

/**
 * @brief       Kpu pooling
 *
 * @param[in]   src                        Source
 * @param[in]   src_param                  Source param
 * @param[in]   kernel_size                Kernel size, 7*7 is 49
 * @param[in]   channels                   Channels
 * @param[in]   dest                       Dest
 * @param[in]   dest_param                 Dest param
 *
 */
void kpu_global_average_pool(const uint8_t *src, const quantize_param_t *src_param, int kernel_size, int channels, uint8_t *dest, const quantize_param_t *dest_param);

/**
 * @brief       Kpu pooling
 *
 * @param[in]   src                        Source
 * @param[in]   src_param                  Source param
 * @param[in]   kernel_size                Kernel size, 7*7 is 49
 * @param[in]   channels                   Channels
 * @param[in]   dest                       Dest
 *
 */
void kpu_global_average_pool_float(const uint8_t *src, const quantize_param_t *src_param, int kernel_size, int channels, float *dest);

/**
 * @brief       Kpu fullly connected by cpu
 *
 * @param[in]   src                                 Source
 * @param[in]   weights                             Weight
 * @param[in]   biases                              Biases
 * @param[in]   dest                                Dest
 * @param[in]   input_channels                      Input channels
 * @param[in]   output_channels                     Output channels
 *
 */
void kpu_fully_connected(const float *src, const float *weights, const float *biases, float *dest, int input_channels, int output_channels);

/**
 * @brief       Kpu matrix multiplication
 *
 * @param[in]   src                                 Source
 * @param[in]   channels                            Channels
 * @param[in]   dest                                Dest
 * @param[in]   dest_param                          Dest param
 *
 */
void kpu_matmul_end(const uint8_t *src, int channels, float *dest, const quantize_param_t *dest_param);

/**
 * @brief       Kpu dequantize
 *
 * @param[in]   src                                 Source
 * @param[in]   src_param                           Source param
 * @param[in]   count                               Dequantize count
 * @param[in]   dest                                Dest
 *
 */
void kpu_dequantize(const uint8_t *src, const quantize_param_t *src_param, size_t count, float *dest);

/**
 * @brief       Kpu load kmodel
 *
 * @param[in]   ctx                                 Kmodel object
 * @param[in]   buffer                              Kmodel buffer
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.
 */
int kpu_load_kmodel(kpu_model_context_t *ctx, const uint8_t *buffer);

/**
 * @brief       Kpu free kmodel buffer
 *
 * @param[in]   ctx                                 kmodel object
 *
 */
void kpu_model_free(kpu_model_context_t *ctx);

/**
 * @brief       Kpu load kmodel
 *
 * @param[in]   ctx                                 Kmodel object
 * @param[in]   index                               Output index
 * @param[in]   data                                Output data
 * @param[in]   size                                Output data size
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.
 */
int kpu_get_output(kpu_model_context_t *ctx, uint32_t index, uint8_t **data, size_t *size);

/**
 * @brief       Kpu run kmodel
 *
 * @param[in]   ctx                                 Kmodel object
 * @param[in]   src                                 Source data
 * @param[in]   dma_ch                              Dma channel
 * @param[in]   done_callback                       Kpu complete callback
 * @param[in]   userdata                            Data of callback
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail.
 */
int kpu_run_kmodel(kpu_model_context_t *ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif
