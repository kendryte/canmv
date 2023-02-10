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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "kpu_algorithm.h"

// L2 归一化
uint32_t l2normalize(float *x, float *dx, int len)
{
    int f;
    float sum = 0;
    for(f = 0; f < len; ++f)
    {
        sum += x[f] * x[f];
    }
    sum = sqrtf(sum);
    for(f = 0; f < len; ++f)
    {
        dx[f] = x[f] / sum;
    }
    return 0;
}

void maix_kpu_helper_softmax(float *x, float *dx, uint32_t len)
{
    float max_value = x[0];
    for(uint32_t i = 0; i < len; i++)
    {
        if(max_value < x[i])
        {
            max_value = x[i];
        }
    }
    for(uint32_t i = 0; i < len; i++)
    {
        x[i] -= max_value;
        x[i] = expf(x[i]);
    }
    float sum_value = 0.0f;
    for(uint32_t i = 0; i < len; i++)
    {
        sum_value += x[i];
    }
    for(uint32_t i = 0; i < len; i++)
    {
        dx[i] = x[i] / sum_value;
    }
}

float calCosinDistance(float *faceFeature0P, float *faceFeature1P, int featureLen)
{
    float coorFeature = 0;
    // calculate the sum square
    for(int fIdx = 0; fIdx < featureLen; fIdx++)
    {
        float featureVal0 = *(faceFeature0P + fIdx);
        float featureVal1 = *(faceFeature1P + fIdx);
        coorFeature += featureVal0 * featureVal1;
        //coorFeature += powf(featureVal0 - featureVal1, 2);
    }
    // cosin distance
    //return sqrtf(coorFeature);
    return (0.5 + 0.5 * coorFeature) * 100;
}

/*
#define FEATURE_LEN 256
uint32_t calulate_score(float *feature, float *saved_feature, uint32_t saved_len, float *score)
{
    int i;
    int v_id = -1;
    float v_score;
    float v_score_max = 0.0;
    for(i = 0; i < saved_len; i++)
    {
        v_score = calCosinDistance(feature, &saved_feature[i * FEATURE_LEN], FEATURE_LEN);
        if(v_score > v_score_max)
        {
            v_score_max = v_score;
            v_id = i;
        }
    }
    *score = v_score_max;
    return v_id;
}
*/

#ifdef LP_RECOG_EN
// licenseplate_recognization
static void run_fully(const float *features, const uint32_t len, const float *weight, const float *bias, const uint32_t n_logit, float *result)
{
    memset(result, 0, sizeof(float)*n_logit);
    for(uint32_t i=0; i<n_logit; ++i){
        for(uint32_t j=0; j<len; ++j)
            result[i] += features[j] * weight[i*len + j];
        result[i] += bias[i];
    }
}

#define D1_LEN 31
#define D2_LEN 24
#define D3_7_LEN 34

#define WEIGHT1_LEN 51584
#define WEIGHT2_LEN 39936
#define WEIGHT3_LEN 56576
#define WEIGHT4_LEN 56576
#define WEIGHT5_LEN 56576
#define WEIGHT6_LEN 56576
#define WEIGHT7_LEN 56576
#define BIAS1_LEN 31
#define BIAS2_LEN 24
#define BIAS3_LEN 34
#define BIAS4_LEN 34
#define BIAS5_LEN 34
#define BIAS6_LEN 34
#define BIAS7_LEN 34
void lp_recog_process(const float *features, uint32_t size, const float *weight_data, mp_obj_list_t* result_list)
{
    static float fully_result[34]={0};
    uint32_t chunk = size / 7;
    float *weight1 = weight_data;
    float *bias1 = weight1 + WEIGHT1_LEN;
    float *weight2 = bias1 + BIAS1_LEN;
    float *bias2 = weight2 + WEIGHT2_LEN;
    float *weight3 = bias2+ BIAS2_LEN;
    float *bias3 = weight3 + WEIGHT3_LEN;
    float *weight4 = bias3+ BIAS3_LEN;
    float *bias4 = weight4 + WEIGHT4_LEN;
    float *weight5 = bias4+ BIAS4_LEN;
    float *bias5 = weight5 + WEIGHT5_LEN;
    float *weight6 = bias5+ BIAS5_LEN;
    float *bias6 = weight6 + WEIGHT6_LEN;
    float *weight7 = bias6+ BIAS6_LEN;
    float *bias7 = weight7 + WEIGHT7_LEN;

    run_fully(features, chunk, weight1, bias1, 31, fully_result);
    //mp_obj_list_t *ret_list = m_new(mp_obj_list_t, 1);
    //mp_obj_list_init(ret_list, 0);
    mp_obj_t tuple_1[D1_LEN];
    for(int i = 0; i < D1_LEN; i++){
        tuple_1[i] = mp_obj_new_float(fully_result[i]);
        //mp_obj_list_append(ret_list, mp_obj_new_float(fully_result[i]));
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_1), tuple_1));
    //mp_obj_list_append(result_list, ret_list);

    features += chunk;
    run_fully(features, chunk, weight2, bias2, 24, fully_result);
    mp_obj_t tuple_2[D2_LEN];
    for(int i = 0; i < D2_LEN; i++){
        tuple_2[i] = mp_obj_new_float(fully_result[i]);
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_2), tuple_2));

    features += chunk;
    run_fully(features, chunk, weight3, bias3, 34, fully_result);
    mp_obj_t tuple_3[D3_7_LEN];
    for(int i = 0; i < D3_7_LEN; i++){
        tuple_3[i] = mp_obj_new_float(fully_result[i]);
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_3), tuple_3));

    features += chunk;
    run_fully(features, chunk, weight4, bias4, 34, fully_result);
    mp_obj_t tuple_4[D3_7_LEN];
    for(int i = 0; i < D3_7_LEN; i++){
        tuple_4[i] = mp_obj_new_float(fully_result[i]);
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_4), tuple_4));

    features += chunk;
    run_fully(features, chunk, weight5, bias5, 34, fully_result);
    mp_obj_t tuple_5[D3_7_LEN];
    for(int i = 0; i < D3_7_LEN; i++){
        tuple_5[i] = mp_obj_new_float(fully_result[i]);
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_5), tuple_5));

    features += chunk;
    run_fully(features, chunk, weight6, bias6, 34, fully_result);
    mp_obj_t tuple_6[D3_7_LEN];
    for(int i = 0; i < D3_7_LEN; i++){
        tuple_6[i] = mp_obj_new_float(fully_result[i]);
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_6), tuple_6));

    features += chunk;
    run_fully(features, chunk, weight7, bias7, 34, fully_result);
    mp_obj_t tuple_7[D3_7_LEN];
    for(int i = 0; i < D3_7_LEN; i++){
        tuple_7[i] = mp_obj_new_float(fully_result[i]);
    }
    mp_obj_list_append(result_list, mp_obj_new_tuple(MP_ARRAY_SIZE(tuple_7), tuple_7));
	
	//sprintf(result, "%s%c%c%c%c%c%c", d1, d2, d3, d4, d5, d6, d7);
}
#endif

