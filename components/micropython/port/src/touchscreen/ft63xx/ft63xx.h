#ifndef _FT63XX_H_
#define _FT63XX_H_

#include <stdint.h>
#include "touchscreen.h"

#define FT63XX_SLAVE_ADDR        56
#define FT_ID_G_FOCALTECH_ID     0xA8
#define FT_REG_NUM_FINGER        0x02         //触摸状态寄存器
#define FT_TP1_REG               0X03         //第一个触摸点数据地址

typedef struct 
{
    uint8_t dir;
    uint32_t width;
    uint32_t height;
}ts_lcd_cfg;


typedef struct 
{
    i2c_device_number_t i2c_num;
    touchscreen_type_t last_type;
    int last_x;
    int last_y;
    ts_lcd_cfg cfg;
}ts_ft63xx_t;


int ts_ft63xx_probe(ts_ft63xx_t *ts,touchscreen_config_t *config,ts_lcd_cfg cfg);
void ts_ft63xx_poll(ts_ft63xx_t *ts,int *x,int *y,touchscreen_type_t *type);


#endif // 

