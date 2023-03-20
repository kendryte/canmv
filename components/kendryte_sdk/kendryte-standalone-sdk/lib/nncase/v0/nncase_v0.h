/* Copyright 2019 Canaan Inc.
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
#ifndef _NNCASE_V0_H
#define _NNCASE_V0_H

#include "kpu.h"

#ifdef __cplusplus
extern "C" {
#endif

int nncase_v0_load_kmodel(kpu_model_context_t *ctx, const uint8_t *buffer);
int nncase_v0_get_output(kpu_model_context_t *ctx, uint32_t index, uint8_t **data, size_t *size);
void nncase_v0_model_free(kpu_model_context_t *ctx);
int nncase_v0_run_kmodel(kpu_model_context_t *ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata);

int32_t nncase_v0_probe_model_buffer_size(const uint8_t *buffer, uint32_t buffer_size);
int nncase_v0_get_output_count(kpu_model_context_t *ctx);

int nncase_v0_get_input_shape(kpu_model_context_t *ctx, int index, int *chn, int *h, int *w);
int nncase_v0_get_output_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w);

#ifdef __cplusplus
}
#endif

#endif