#ifndef _YOLO2_REGION_LAYER
#define _YOLO2_REGION_LAYER

#include <stdint.h>
#include "kpu.h"
#include "objlist.h"

typedef struct
{
    uint32_t obj_number;
    struct
    {
        uint32_t x1;
        uint32_t y1;
        uint32_t x2;
        uint32_t y2;
        uint32_t class_id;
        float prob;
    } obj[10];
} obj_info_t;

typedef struct
{
    float threshold;
    float nms_value;
    uint32_t coords;
    uint32_t anchor_number;
    float *anchor;
    uint32_t image_width;
    uint32_t image_height;
    uint32_t classes;
    uint32_t net_width;
    uint32_t net_height;
    uint32_t layer_width;
    uint32_t layer_height;
    uint32_t boxes_number;
    uint32_t output_number;
    void *boxes;
    float *input;
    float *output;
    float *probs_buf;
    float **probs;
} yolo2_region_layer_t;

//typedef void(*callback_draw_box)(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob);
static inline float sigmoid(float x)
{
    return 1.f / (1.f + expf(-x));
}
int yolo_region_layer_init(yolo2_region_layer_t *rl, int width, int height, int channels, int origin_width, int origin_height);
void yolo_region_layer_deinit(yolo2_region_layer_t *rl);
void yolo_region_layer_run(yolo2_region_layer_t *rl);
void yolo_region_layer_get_rect(yolo2_region_layer_t *rl, mp_obj_list_t* out_box);

#endif // _YOLO2_REGION_LAYER
