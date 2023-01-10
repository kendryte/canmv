#include <bsp.h>
#include <sysctl.h>
#include "FreeRTOS.h"
#include "task.h"

#include "spi_drv.h"

#include "trace.h"
//#include "app_main.h"
#include "esp_common/common.h"

#include "netdev_api.h"
//#include "arp_server_stub.h"
#include "ctrl_api.h"
#include "util.h"
#include "esp32_control.h"

#include "esp32_if.h"
//#include "lwip/init.h"
#include "lwip/timeouts.h"

/** Constants/Macros **/
#define CTRL_CMD_DEFAULT_REQ() {                          \
  .msg_type = CTRL_REQ,                                   \
  .ctrl_resp_cb = NULL,                                   \
  .cmd_timeout_sec = 3 /*DEFAULT_CTRL_RESP_TIMEOUT*/ /*30 sec*/ \
}


#define DEFAULT_SOFTAP_MAX_CONNECTIONS      10
#define DEFAULT_SOFTAP_CONNECTIONS 			4

#define LWIP_TASK_STACK_SIZE     2048
//#define GET_TASK_STACK_LEN(size) ((size) / sizeof(StackType_t))


/** Function declaration **/
static void init_sta(void);
static void init_ap(void);
static void esp32c3_wifi_softap_set_up(esp32c3_t *self, bool up);

/** Exported variables **/

//static int esp32_driver_init_flag = 0;
static TimerHandle_t timer_hander_lwip;
static StaticTimer_t xTimerBuffer;
static thread_handle_t lwip_task_id = 0;

esp32c3_t esp32c3_state;


void esp32c3_init(esp32c3_t *self)
{
	//if(!esp32_driver_init_flag)
	{
		//esp32_driver_init_flag = 1;
		reset_slave();
			/* Init network interface */
		network_init();

		/* init spi driver */
		stm_spi_init();
	
		self->itf_state = 0;
		self->wifi_scan_state = 0;
		self->wifi_join_state = 0;
		self->pend_disassoc = false;
		self->pend_rejoin= false;
		self->pend_rejoin_wpa = false;
		self->ap_channel = 1;
		self->ap_ssid_len = 0;
		self->ap_auth = WIFI_AUTH_WPA2_PSK;
		self->ap_max_conn = DEFAULT_SOFTAP_CONNECTIONS;
		self->ap_ssid_hidden = 0; // 1 hidden
		self->ap_bandwidth = WIFI_BW_HT40;

		if (init_hosted_control_lib()) {
			printf("init hosted control lib failed\n\r");
			return;
		}

		register_event_callbacks();

			/* Start DHCP Client */
		//xTaskCreate(LwIP_DHCP_task, "DHCP", GET_TASK_STACK_LEN(2048), NULL, 4, NULL);

		BaseType_t xReturned;
		xReturned = xTaskCreate((TaskFunction_t)lwip_task, 
								"lwip_Thread", 
								GET_TASK_STACK_LEN(LWIP_TASK_STACK_SIZE),
								NULL, 
								osPriorityAboveNormal, 
								&lwip_task_id);
		if (xReturned != pdPASS){
			printf("Failed to create task, at: %s:%u\n", __FILE__, __LINE__);
		}
		assert(lwip_task_id);

	}

}

void esp32c3_deinit(void)
{
	//esp32_driver_init_flag = 0;
	//TODO: delete init-threads
}

static int g_wifi_mode = WIFI_MODE_NONE;
int esp32c3_set_wifi_mode(int mode)
{
	if ((mode & g_wifi_mode) != mode) {
		g_wifi_mode |= mode;
	}

	if (test_set_wifi_mode(g_wifi_mode)) {
		printf("Failed to set wifi mode to %u\n\r", mode);
		return ESP_FAIL;
	}
	return ESP_OK; //0;
}

int esp32c3_get_available_wifi(esp32c3_t *self)
{
	/* implemented synchronous */
	ctrl_cmd_t req = CTRL_CMD_DEFAULT_REQ();
	req.cmd_timeout_sec = 30;

	ctrl_cmd_t *resp = NULL;

	resp = wifi_ap_scan_list(req);
	esp32_set_user_resp_cb(resp, self->wifi_scan_cb, NULL);

	return ctrl_app_resp_callback(resp);
}

int esp32c3_scan_wifi_list(esp32c3_t *self, int (*result_cb)(const wifi_scanlist_t*))
{
    if (self->itf_state == 0) {
		//printf("STA must be active\n\r");
        return -1;
    }
	// Set state and callback data
	self->wifi_scan_state = 1;
    //self->wifi_scan_env = env;
    self->wifi_scan_cb = result_cb;

	esp32c3_get_available_wifi(self);

	return 0;
}


int esp32c3_wifi_get_mac(esp32c3_t *self, int itf, uint8_t mac[6]) {
    //mp_hal_get_mac(MP_HAL_MAC_WLAN0, &mac[0]);
	memcpy(mac, self->netif[itf].hwaddr, 6);
    return 0;
}

void esp32c3_wifi_set_up(esp32c3_t *self, int itf, bool up) {
    if (up) {
        if (self->itf_state == 0) {
			// TODO: set wifi tx power
        }
        if (itf == ESP32_ITF_AP) {
            esp32c3_wifi_softap_set_up(self, true);
        }
        if ((self->itf_state & (1 << itf)) == 0) {
            // CYW_ENTER
            esp32c3_tcpip_deinit(self, itf);
            esp32c3_tcpip_init(self, itf);
            self->itf_state |= 1 << itf;
            // CYW_EXIT
        }
    } else {
        if (itf == ESP32_ITF_AP) {
        	esp32c3_wifi_softap_set_up(self, false);
        }
    }
}

int esp32c3_wifi_get_curr_tx_power(void)
{
	/* implemented synchronous */
	ctrl_cmd_t req = CTRL_CMD_DEFAULT_REQ();
	ctrl_cmd_t *resp = NULL;

	resp = wifi_get_curr_tx_power(req);
	if(resp->msg_id == CTRL_RESP_GET_WIFI_CURR_TX_POWER){
		return resp->u.wifi_tx_power.power;
	}
	return 0;

}

/*******************************************************************************/
// WiFi AP

static void esp32c3_wifi_softap_set_up(esp32c3_t *self, bool up) {
	int ret = 0;
	if (up) {
		ret = test_softap_mode_start(self->ap_ssid, self->ap_key, self->ap_channel, self->ap_auth, 
									self->ap_max_conn, self->ap_ssid_hidden, self->ap_bandwidth);
		if (ret) {
			printf("Failed to start softAP \n\r");
		} else {
			printf("started %s softAP\n\r", self->ap_ssid);
			//control_path_call_event(SOFTAP_STARTED);
		}
	}
	else{
		test_softap_mode_stop();
	}
}
