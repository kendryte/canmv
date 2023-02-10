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

#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __MAIX_KPU_H
