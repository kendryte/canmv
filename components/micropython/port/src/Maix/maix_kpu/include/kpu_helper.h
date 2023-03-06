#ifndef __KPU_HELPER_H
#define __KPU_HELPER_H

#include <stdint.h>

#include <mp.h>

#include "maix_kpu.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _kpu_used_mem_info {
    void *ptr;
    size_t size;
} kpu_used_mem_info_t;

int maix_kpu_helper_probe_model_size(uint8_t *model_buffer, uint32_t buffer_size);
int maix_kpu_helper_get_input_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w);
int maix_kpu_helper_get_output_count(kpu_model_context_t *ctx);
int maix_kpu_helper_get_output_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w);

int maix_kpu_helper_load_file_from_rawflash(uint32_t addr, uint8_t *data_buf, uint32_t length);
int32_t maix_kpu_helper_get_mode_size_from_rawflash(uint32_t offset);

int maix_kpu_helper_load_file_from_filesystem(const char *path, void *buffer, size_t model_size);
int32_t maix_kpu_helper_get_file_size_from_filesystem(const char *path);

int maix_kpu_helper_add_mem_to_list(kpu_used_mem_info_t *mem);
int maix_kpu_heler_del_mem_from_list(void *ptr);
int maix_kpu_helper_free_mem_list(size_t *size);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __KPU_HELPER_H
