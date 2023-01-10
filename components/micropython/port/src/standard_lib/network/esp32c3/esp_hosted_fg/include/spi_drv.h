// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/** prevent recursive inclusion **/
#ifndef __SPI_DRV_H
#define __SPI_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

/** Includes **/
#include "esp_common/common.h"

/** constants/macros **/
#define MAX_NETWORK_INTERFACES  2
#define STA_INTERFACE           "ESP_STATION"
#define SOFTAP_INTERFACE        "ESP_SOFTAP"

/* NSS or CS0 configuration (Pin 11) */
/* In case of different board than STM32F469I,
 * User need to update SPI NSS pin as per hardware*/
// #ifndef USR_SPI_CS_GPIO_Port
// #define USR_SPI_CS_GPIO_Port    GPIOA
// #endif
// #ifndef USR_SPI_CS_Pin
// #define USR_SPI_CS_Pin          GPIO_PIN_15
// #endif
//#define GPIO_DATA_READY_Pin 0 //GPIOHS index not pin number
//#define GPIO_HANDSHAKE_Pin 1

#define HANDSHAKE_PIN    				14
#define DATA_READY_PIN    				13
#define SLAVE_RESET_PIN    			    17
#define ESP_SPI_CS_PIN					10

#define DATA_READY_GPIO 	 	28 
#define HANDSHAKE_GPIO		 	27
#define SLAVE_RESET_GPIO     	26
//#define ESP_SPI_CS_GPIO			25

typedef enum spi_drv_events_s {
	SPI_DRIVER_ACTIVE
} spi_drv_events_e;

/** Exported Structures **/

/** Exported variables **/

/** Inline functions **/

/** Exported Functions **/
void reset_slave(void);
void stm_spi_init(void);
stm_ret_t send_to_slave(uint8_t iface_type, uint8_t iface_num,
		uint8_t * wbuffer, uint16_t wlen);

struct esp_private {
	uint8_t     if_type;
	uint8_t     if_num;
	void        *netdev;
};

#ifdef __cplusplus
}
#endif

#endif
