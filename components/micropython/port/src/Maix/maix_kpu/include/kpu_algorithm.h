#ifndef _PY_KPU_CLASSIFIER_H
#define _PY_KPU_CLASSIFIER_H

#include <stdint.h>
#include "objlist.h"
//#include "image_process.h"
//#include "kpu.h"

#define MAX_FEATURE_LEN 256
#define LP_RECOG_EN

uint32_t l2normalize(float *x, float *dx, int len);
void maix_kpu_helper_softmax(float *x, float *dx, uint32_t len);

float calCosinDistance(float *faceFeature0P, float *faceFeature1P, int featureLen);
void lp_recog_process(const float *features, uint32_t size, const float *weight_data, mp_obj_list_t* result_list);

#endif /* _PY_KPU_CLASSIFIER_H */
