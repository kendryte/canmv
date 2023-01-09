#ifndef ESP32_IF_h
#define ESP32_IF_h

#include <string.h>

#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "ctrl_api.h"
#include "lib/netutils/dhcpserver.h"

// Return value of esp32c3_wifi_link_status
#define ESP32_LINK_DOWN         (0)
#define ESP32_LINK_JOIN         (1)
#define ESP32_LINK_NOIP         (2)
#define ESP32_LINK_UP           (3)
#define ESP32_LINK_FAIL         (-1)
// #define ESP32_LINK_NONET        (-2)
// #define ESP32_LINK_BADAUTH      (-3)

#define WIFI_JOIN_STATE_NONE        (0)
#define WIFI_JOIN_STATE_JOINING     (1)
#define WIFI_JOIN_STATE_JOINED      (2)


#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

typedef struct esp32c3_t { //_esp32c3_t {
    //esp32c3_ll_t esp32c3_ll;

    uint8_t itf_state;
    uint32_t trace_flags;

    // State for async events
    volatile uint32_t wifi_scan_state;
    uint32_t wifi_join_state;
    void *wifi_scan_env;
    int (*wifi_scan_cb)(const wifi_scanlist_t*);

    // Pending things to do
    bool pend_disassoc;
    bool pend_rejoin;
    bool pend_rejoin_wpa;

    // AP settings
    uint8_t ap_channel;
    uint8_t ap_auth;
    uint8_t ap_ssid_len;
    uint8_t ap_key_len;
    uint8_t ap_ssid[32];
    uint8_t ap_key[64];
    uint8_t ap_max_conn;
    uint8_t ap_ssid_hidden;
    uint8_t ap_bandwidth;

    // lwIP data
    struct netif netif[2];
    struct dhcp dhcp_client;
    dhcp_server_t dhcp_server;
} esp32c3_t;


enum {
    ESP32_ITF_STA,
    ESP32_ITF_AP,
};

extern esp32c3_t esp32c3_state;

void esp32c3_init(esp32c3_t *self);
void esp32c3_deinit(void);

int esp32c3_set_wifi_mode(int mode);
void esp32c3_wifi_set_up(esp32c3_t *self, int itf, bool up);
int esp32c3_wifi_get_mac(esp32c3_t *self, int itf, uint8_t mac[6]);
int esp32c3_wifi_get_curr_tx_power(void);
int esp32c3_scan_wifi_list(esp32c3_t *self, int (*result_cb)(const wifi_scanlist_t*));

static inline void esp32c3_wifi_ap_get_ssid(esp32c3_t *self, size_t *len, const uint8_t **buf) {
    *len = self->ap_ssid_len;
    *buf = self->ap_ssid;
}

static inline void esp32c3_wifi_ap_set_channel(esp32c3_t *self, uint32_t channel) {
    self->ap_channel = channel;
}

static inline void esp32c3_wifi_ap_set_ssid(esp32c3_t *self, size_t len, const uint8_t *buf) {
    self->ap_ssid_len = MIN(len, sizeof(self->ap_ssid));
    memcpy(self->ap_ssid, buf, self->ap_ssid_len);
}

static inline void esp32c3_wifi_ap_set_password(esp32c3_t *self, size_t len, const uint8_t *buf) {
    self->ap_key_len = MIN(len, sizeof(self->ap_key));
    memcpy(self->ap_key, buf, self->ap_key_len);
}

static inline void esp32c3_wifi_ap_set_auth(esp32c3_t *self, uint32_t auth) {
    self->ap_auth = auth;
}

void esp32c3_tcpip_init(esp32c3_t *self, int itf);
void esp32c3_tcpip_deinit(esp32c3_t *self, int itf);
void esp32c3_tcpip_set_link_up(esp32c3_t *self, int itf);
void esp32c3_tcpip_set_link_down(esp32c3_t *self, int itf);
int esp32c3_tcpip_link_status(esp32c3_t *self, int itf);
int esp32c3_send_ethernet(esp32c3_t *self, int itf, size_t len, const void *buf);

void lwip_task(void const *arg);
void LwIP_DHCP_task(void * pvParameters);

#endif // ESP32_IF_h
