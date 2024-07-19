#include "ft63xx.h"
#include <string.h>
#include <stdlib.h>
#include <tsfilter.h>
#include <stdio.h>
#include "fpioa.h"
#include "sleep.h"
#include <math.h>
#include "machine_i2c.h"
#include "py/obj.h"


static i2c_device_number_t m_i2c_num;

static inline int hi2c_mem_recv(uint8_t addr,uint8_t mem_addr,uint8_t *read_buf,uint16_t read_size)
{
    return maix_i2c_recv_data(m_i2c_num, addr, &mem_addr, 1, read_buf, read_size, 100);
}

int ts_ft63xx_probe(ts_ft63xx_t *ts,touchscreen_config_t *config,ts_lcd_cfg cfg)
{
    ts->i2c_num = config->i2c->i2c;
    ts->last_type = TOUCHSCREEN_STATUS_IDLE;
    memcpy(&ts->cfg,&cfg,sizeof(ts_lcd_cfg));
    ts->last_x = ts->last_y = 0;
    
    uint8_t reg_val;

    if (hi2c_mem_recv(FT63XX_SLAVE_ADDR,FT_ID_G_FOCALTECH_ID,&reg_val,1) != 0)
        return EIO;

    if (reg_val != 0x11)
        return EXDEV;
    
    return 0;
}


void ts_ft63xx_poll(ts_ft63xx_t *ts,int *x,int *y,touchscreen_type_t *type)
{
    union 
    {
        struct 
        {
            uint8_t touch_num;
            uint8_t p1_xh;
            uint8_t p1_xl;
            uint8_t p1_yh;
            uint8_t p1_yl;
        };
        uint8_t dat[5];
    }reg_val;
    
    
    if (hi2c_mem_recv(FT63XX_SLAVE_ADDR,FT_REG_NUM_FINGER,reg_val.dat,5) != 0)
    {
        *type = TOUCHSCREEN_STATUS_IDLE;
        return;
    }

    if (reg_val.touch_num&7)
    {
        *type = TOUCHSCREEN_STATUS_PRESS;
        int reg_x = ((reg_val.p1_xh&0x0f)<<8) + reg_val.p1_xl;
        int reg_y = ((reg_val.p1_yh&0x0f)<<8) + reg_val.p1_yl;

        switch (ts->cfg.dir)
        {
        case 0:
            *x = reg_x;
            *y = reg_y;
            break;
        case 1:
            *x = reg_y;
            *y = ts->cfg.height - reg_x;
            break;
        case 2:
            *x = ts->cfg.width - reg_x;
            *y = ts->cfg.height - reg_y;
            break;
        case 3:
            *x = ts->cfg.width - reg_y;
            *y = reg_x;
            break;
        default:
            break;
        }
        
    }    
    else
    {
        *type = TOUCHSCREEN_STATUS_RELEASE;
        *x = ts->last_x;
        *y = ts->last_y;
    }
        
    if (ts->last_type == TOUCHSCREEN_STATUS_PRESS && (*x != ts->last_x || *y != ts->last_y))
    {
        *type = TOUCHSCREEN_STATUS_MOVE;
    }
    

    ts->last_type = *type;
    ts->last_x = *x;
    ts->last_y = *y;
}
