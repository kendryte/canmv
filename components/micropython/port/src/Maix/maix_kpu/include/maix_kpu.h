#ifndef __MAIX_KPU_H
#define __MAIX_KPU_H

#include <stdint.h>

#include <mp.h>

#include "kpu.h"

#include "kpu_algorithm.h"
#include "kpu_helper.h"

#ifdef  __cplusplus
extern "C" {
#endif

// NCHW
typedef struct _k210_kpu_shape {
    int vaild;
    int num;
    int chn;
    int h;
    int w;
} k210_kpu_shape_t;

typedef struct _mp_obj_k210_kpu
{
    mp_obj_base_t base;

    struct {
        kpu_model_context_t ctx;
        char path[128];
        size_t size;
        uint8_t *buffer;
    } model;

    union
    {
        uint32_t _data;
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

    uint8_t sha256[32];

} __attribute__((aligned(8))) mp_obj_k210_kpu_t;

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __MAIX_KPU_H
