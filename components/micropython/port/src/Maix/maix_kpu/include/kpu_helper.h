#ifndef __KPU_HELPER_H
#define __KPU_HELPER_H

#include <stdint.h>

#include <mp.h>

#include "maix_kpu.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum kpu_used_mem_type {
    MEM_TYPE_PTR = 0,
    MEM_TYPE_MP_KPU_OBJ,

    MEM_TYPE_MAX = 255,
};

typedef struct _kpu_used_mem_info {
    void *ptr;
    enum kpu_used_mem_type type;
} kpu_used_mem_info_t;

int maix_kpu_helper_probe_model_size(uint8_t *model_buffer);
int maix_kpu_helper_get_input_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w);
int maix_kpu_helper_get_output_shape(kpu_model_context_t *ctx, int *chn, int *h, int *w);

int maix_kpu_helper_load_file_from_rawflash(uint32_t addr, uint8_t *data_buf, uint32_t length);
mp_uint_t maix_kpu_helper_get_mode_size_from_rawflash(uint32_t offset);

int maix_kpu_helper_load_file_from_filesystem(const char *path, void *buffer, size_t model_size);
mp_uint_t maix_kpu_helper_get_file_size_from_filesystem(const char *path);

int maix_kpu_helper_add_mem_to_list(kpu_used_mem_info_t *mem);
int maix_kpu_heler_del_mem_from_list(void *ptr);
int maix_kpu_helper_free_mem_list(void);

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __KPU_HELPER_H
