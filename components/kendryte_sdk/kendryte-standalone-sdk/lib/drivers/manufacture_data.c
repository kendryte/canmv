#include <assert.h>
#include <stdlib.h>
#include	"otp.h"


#define		OTP_MANUFACTURE_DATA_ADDR		0x3D90
#define		ASSERT(ex)				while(!(ex));

/* Manufacture data (Start address 0x3D90) */
typedef struct {

	uint8_t 	cp_kgd;
	uint8_t 	ft_kgd;
	uint8_t		work_order_id[10];
    union {
        uint32_t time;
        struct
        {
            uint32_t second: 6;
            uint32_t minute: 6;
            uint32_t hour: 	 5;
			uint32_t day:    5;
            uint32_t month:  4;
            uint32_t year:   6;
        }data;
    }id1 ;
    union {
        uint16_t no;
        struct
        {
            uint16_t site_no: 6;
			uint16_t ate_no:  10;
        } data;
    }id2;	
    union {
        uint8_t sit;
        struct
        {
            uint8_t package_site: 5;
			uint8_t	fab_site:	  3;
        } data;
    } site;
    union {
        uint32_t version;
        struct
        {
            uint32_t chip_rev: 	4;
			uint32_t chip_code: 10;
			uint32_t chip_no: 	10;
			uint32_t chip_name:	8;	
        } data;
    } version;
	uint8_t diey;
	uint8_t diex;
	uint8_t wafer_id;
	uint8_t lot_id[6];
}__attribute__((packed, aligned(4))) manufacture_data_t;

uint64_t	get_chip_number(void)
{
	uint64_t devid = 0;
	manufacture_data_t *manufacture_data = malloc(sizeof(manufacture_data_t));
	ASSERT(manufacture_data); //assert(manufacture_data);
	
	if(0 == otp_read_data(OTP_MANUFACTURE_DATA_ADDR, (uint8_t *)manufacture_data, sizeof(manufacture_data_t)) ) {
		
		devid = manufacture_data->id1.time;
		devid <<= 16;
		devid |= manufacture_data->id2.no;
		//printk("devid= %lx\n", devid);

	}
	else {
		//printk("get device number error !\n");
	}	
	
	free(manufacture_data);
	
	return devid;
}



