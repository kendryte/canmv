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
#ifndef __CONTROL_H
#define __CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/** Includes **/
//#include "cmsis_os.h"
#include "esp_common/common.h"
#include "ctrl_api.h"

/** constants/macros **/
typedef enum {
	MODE_NULL    = 0x0,
	MODE_STATION = 0x1,
	MODE_SOFTAP  = 0x2,
	MODE_SOFTAP_STATION = (MODE_STATION|MODE_SOFTAP),
	MODE_MAX
} operating_mode;

typedef enum control_path_events_s {
	STATION_CONNECTED,
	STATION_DISCONNECTED,
	SOFTAP_STARTED,
	SOFTAP_STOPPED
} control_path_events_e;


#define WIFI_MAX_STR_LEN                  19

/** Exported Structures **/

/** Exported variables **/


/** Inline functions **/

/** Exported Functions **/
void control_path_init(void(*control_path_evt_handler)(uint8_t));
stm_ret_t get_self_ip_station(uint32_t *self_ip);
stm_ret_t get_self_ip_softap(uint32_t *self_ip);
uint8_t *get_self_mac_station();
uint8_t *get_self_mac_softap();
stm_ret_t get_arp_dst_ip_station(uint32_t *sta_ip);
stm_ret_t get_arp_dst_ip_softap(uint32_t *soft_ip);

int test_set_wifi_mode(int mode);
int test_get_available_wifi(void);
int test_station_mode_get_mac_addr(char *mac);
int test_station_mode_connect(char *ssid, char *pwd, char *bssid,
	int is_wpa3_supported, int listen_interval);
int test_station_mode_disconnect(void);
int test_station_mode_get_info(void);
int test_softap_mode_get_mac_addr(char *mac);
int test_softap_mode_start(char *ssid, char *pwd, int channel,
	int encryption_mode, int max_conn, int ssid_hidden, int bw);
int test_softap_mode_stop(void);
int unregister_event_callbacks(void);
int register_event_callbacks(void);
int test_wifi_set_max_tx_power(int in_power);

int ctrl_app_resp_callback(ctrl_cmd_t * app_resp);
void esp32_set_user_resp_cb(ctrl_cmd_t *app_resp, int (*user_resp_cb)(void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif
