/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018-2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/mphal.h"


#if MICROPY_PY_NETWORK_ESP32C3

#include "lwip/netif.h"
#include "lwip/dhcp.h"

#include "network_esp32c3.h"
#include "modnetwork.h"
#include "esp32_if.h"
#include "esp32_control.h"
#include "ctrl_api.h"

typedef struct _network_esp32c3_obj_t {
    mp_obj_base_t base;
    esp32c3_t *esp;
    int itf;
} network_esp32c3_obj_t;

STATIC const network_esp32c3_obj_t network_esp32c3_wl0 = { { &mp_network_esp32c3_type }, &esp32c3_state, 0 };
STATIC const network_esp32c3_obj_t network_esp32c3_wl1 = { { &mp_network_esp32c3_type }, &esp32c3_state, 1 };

STATIC void network_esp32c3_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(self_in);
    struct netif *netif = &self->esp->netif[self->itf];
    int status = esp32c3_tcpip_link_status(self->esp, self->itf);
    const char *status_str;
    if (status == ESP32_LINK_DOWN) {
        status_str = "down";
    } else if (status == ESP32_LINK_JOIN || status == ESP32_LINK_NOIP) {
        status_str = "join";
    } else if (status == ESP32_LINK_UP) {
        status_str = "up";
    } else {
        status_str = "fail";
    }
    mp_printf(print, "<ESP32C3 %s %s %u.%u.%u.%u>",
        self->itf == 0 ? "STA" : "AP",
        status_str,
        netif->ip_addr.addr & 0xff,
        netif->ip_addr.addr >> 8 & 0xff,
        netif->ip_addr.addr >> 16 & 0xff,
        netif->ip_addr.addr >> 24
    );
}

STATIC void esp32_make_new_helper(void)
{
    //esp32c3_init();
    //fix softap and station can not connect bug： set max txpower < 56
	test_wifi_set_max_tx_power(52);
}

STATIC mp_obj_t network_esp32c3_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    esp32_make_new_helper(); //init esp32c3 driver

    if (n_args == 0 || mp_obj_get_int(args[0]) == 0) {
        esp32c3_set_wifi_mode(MODE_STATION);
        return MP_OBJ_FROM_PTR(&network_esp32c3_wl0);
    } else {
        esp32c3_set_wifi_mode(MODE_SOFTAP);
        return MP_OBJ_FROM_PTR(&network_esp32c3_wl1);
    }
}

// 发送原始网络数据
STATIC mp_obj_t network_esp32c3_send_ethernet(mp_obj_t self_in, mp_obj_t buf_in) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t buf;
    mp_get_buffer_raise(buf_in, &buf, MP_BUFFER_READ);
    int ret = esp32c3_send_ethernet(self->esp, self->itf, buf.len, buf.buf);
    if (ret) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(network_esp32c3_send_ethernet_obj, network_esp32c3_send_ethernet);

/*******************************************************************************/
// network API

STATIC mp_obj_t network_esp32c3_deinit(mp_obj_t self_in) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //esp32c3_deinit(self->esp);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_esp32c3_deinit_obj, network_esp32c3_deinit);

STATIC mp_obj_t network_esp32c3_active(size_t n_args, const mp_obj_t *args) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        return mp_obj_new_bool(esp32c3_tcpip_link_status(self->esp, self->itf));
        //return mp_const_none; 
    } else {
        esp32c3_wifi_set_up(self->esp, self->itf, mp_obj_is_true(args[1]));
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_esp32c3_active_obj, 1, 2, network_esp32c3_active);

STATIC int network_esp32c3_scan_cb(const wifi_scanlist_t *res) {
    mp_obj_t list = MP_OBJ_FROM_PTR(esp32c3_state.wifi_scan_env);

    // Search for existing BSSID to remove duplicates
    bool found = false;
    // size_t len;
    // mp_obj_t *items;
    // mp_obj_get_array(list, &len, &items);
    // for (size_t i = 0; i < len; ++i) {
    //     mp_obj_tuple_t *t = MP_OBJ_TO_PTR(items[i]);
    //     if (memcmp(res->bssid, ((mp_obj_str_t*)MP_OBJ_TO_PTR(t->items[1]))->data, sizeof(res->bssid)) == 0) {
    //         if (res->rssi > MP_OBJ_SMALL_INT_VALUE(t->items[3])) {
    //             t->items[3] = MP_OBJ_NEW_SMALL_INT(res->rssi);
    //         }
    //         t->items[5] = MP_OBJ_NEW_SMALL_INT(MP_OBJ_SMALL_INT_VALUE(t->items[5]) + 1);
    //         found = true;
    //         break;
    //     }
    // }

    // Add to list of results if wanted
    if (!found) {
        mp_obj_t tuple[6] = {
            mp_obj_new_bytes(res->ssid, strlen(res->ssid)),
            mp_obj_new_bytes(res->bssid, strlen(res->bssid)),
            MP_OBJ_NEW_SMALL_INT(res->channel),
            MP_OBJ_NEW_SMALL_INT(res->rssi),
            MP_OBJ_NEW_SMALL_INT(res->encryption_mode),
            //mp_const_false, // hidden
            MP_OBJ_NEW_SMALL_INT(1), // N
        };
        mp_obj_list_append(list, mp_obj_new_tuple(6, tuple));
    }

    return 0; // continue scan
}

STATIC mp_obj_t network_esp32c3_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_obj_t res = mp_obj_new_list(0, NULL);
    esp32c3_state.wifi_scan_env = res;
    //test_get_available_wifi();

    if(esp32c3_scan_wifi_list(self->esp, network_esp32c3_scan_cb) != 0){
        mp_raise_msg(&mp_type_OSError, "STA must be active");
    }

    // Wait for scan to finish, with a 10s timeout
    // uint32_t start = mp_hal_ticks_ms();
    // while (esp32c3_wifi_scan_active(self->esp) && mp_hal_ticks_ms() - start < 10000) {
    //     MICROPY_EVENT_POLL_HOOK
    // }
    esp32c3_state.wifi_scan_env = NULL;
    return res;
    //return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(network_esp32c3_scan_obj, 1, network_esp32c3_scan);

STATIC mp_obj_t network_esp32c3_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_ssid, ARG_key, ARG_auth, ARG_bssid, ARG_channel };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_ssid, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_key, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_auth, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bssid, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_channel, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    };

    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t ssid;
    mp_get_buffer_raise(args[ARG_ssid].u_obj, &ssid, MP_BUFFER_READ);
    mp_buffer_info_t key;
    key.buf = NULL;
    if (args[ARG_key].u_obj != mp_const_none) {
        mp_get_buffer_raise(args[ARG_key].u_obj, &key, MP_BUFFER_READ);
    }
    mp_buffer_info_t bssid;
    bssid.buf = NULL;
    if (args[ARG_bssid].u_obj != mp_const_none) {
        mp_get_buffer_raise(args[ARG_bssid].u_obj, &bssid, MP_BUFFER_READ);
        if (bssid.len != 12) {
            mp_raise_ValueError(NULL);
        }
    }
    int is_wpa3_supported = 0;
    int listen_interval = 3; //DEFAULT_LISTEN_INTERVAL
    is_wpa3_supported = args[ARG_auth].u_int;
    int ret = test_station_mode_connect(ssid.buf, key.buf, bssid.buf, is_wpa3_supported, listen_interval);
    if (ret != 0) {
        mp_raise_OSError(ret);
    }
    else{
        //if(self->esp->wifi_join_state == WIFI_JOIN_STATE_JOINED)
        {
            //lwip dhcp init
            dhcp_stop(&self->esp->netif[self->itf]);
            dhcp_start(&self->esp->netif[self->itf]);
            esp32c3_tcpip_set_link_up(self->esp, self->itf);
        }
        self->esp->wifi_join_state = WIFI_JOIN_STATE_JOINED;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(network_esp32c3_connect_obj, 1, network_esp32c3_connect);

STATIC mp_obj_t network_esp32c3_disconnect(mp_obj_t self_in) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int ret = test_station_mode_disconnect();
    if (ret != 0) {
        mp_raise_OSError(ret);
    }
    else{
        self->esp->wifi_join_state = WIFI_JOIN_STATE_NONE;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_esp32c3_disconnect_obj, network_esp32c3_disconnect);

STATIC mp_obj_t network_esp32c3_isconnected(mp_obj_t self_in) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(esp32c3_tcpip_link_status(self->esp, self->itf) == 3);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(network_esp32c3_isconnected_obj, network_esp32c3_isconnected);

STATIC mp_obj_t network_esp32c3_ifconfig(size_t n_args, const mp_obj_t *args) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    return mod_network_nic_ifconfig(&self->esp->netif[self->itf], n_args - 1, args + 1);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_esp32c3_ifconfig_obj, 1, 2, network_esp32c3_ifconfig);

// static inline uint32_t nw_get_le32(const uint8_t *buf) {
//     return buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
// }

// static inline void nw_put_le32(uint8_t *buf, uint32_t x) {
//     buf[0] = x;
//     buf[1] = x >> 8;
//     buf[2] = x >> 16;
//     buf[3] = x >> 24;
// }

STATIC mp_obj_t network_esp32c3_config(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (kwargs->used == 0) {
        // Get config value
        if (n_args != 2) {
            mp_raise_TypeError("must query one param");
        }

        switch (mp_obj_str_get_qstr(args[1])) {
            // case MP_QSTR_antenna: {
            //     uint8_t buf[4];
            //     esp32c3_ioctl(self->esp, ESP32C3_IOCTL_GET_ANTDIV, 4, buf, self->itf);
            //     return MP_OBJ_NEW_SMALL_INT(nw_get_le32(buf));
            // }
            // case MP_QSTR_channel: {
            //     uint8_t buf[4];
            //     esp32c3_ioctl(self->esp, ESP32C3_IOCTL_GET_CHANNEL, 4, buf, self->itf);
            //     return MP_OBJ_NEW_SMALL_INT(nw_get_le32(buf));
            // }
            case MP_QSTR_ssid: {
                if (self->itf == ESP32_ITF_STA) {
                    // uint8_t buf[36];
                    // esp32c3_ioctl(self->esp, ESP32C3_IOCTL_GET_SSID, 36, buf, self->itf);
                    // return mp_obj_new_str((const char*)buf + 4, nw_get_le32(buf));
                } else {
                    size_t len;
                    const uint8_t *buf;
                    esp32c3_wifi_ap_get_ssid(self->esp, &len, &buf);
                    return mp_obj_new_str((const char*)buf, len);
                }
            }
            case MP_QSTR_mac: {
                uint8_t buf[6];
                esp32c3_wifi_get_mac(self->esp, self->itf, buf);
                return mp_obj_new_bytes(buf, 6);
            }
            case MP_QSTR_txpower: {
                return MP_OBJ_NEW_SMALL_INT(esp32c3_wifi_get_curr_tx_power());
            }
            default:
                mp_raise_ValueError("unknown config param");
        }
    } else {
        // Set config value(s)
        if (n_args != 1) {
            mp_raise_TypeError("can't specify pos and kw args");
        }

        for (size_t i = 0; i < kwargs->alloc; ++i) {
            if (MP_MAP_SLOT_IS_FILLED(kwargs, i)) {
                mp_map_elem_t *e = &kwargs->table[i];
                switch (mp_obj_str_get_qstr(e->key)) {
                    // case MP_QSTR_antenna: {
                    //     uint8_t buf[4];
                    //     nw_put_le32(buf, mp_obj_get_int(e->value));
                    //     esp32c3_ioctl(self->esp, ESP32C3_IOCTL_SET_ANTDIV, 4, buf, self->itf);
                    //     break;
                    // }
                    case MP_QSTR_channel: {
                        esp32c3_wifi_ap_set_channel(self->esp, mp_obj_get_int(e->value));
                        break;
                    }
                    case MP_QSTR_ssid: {
                        size_t len;
                        const char *str = mp_obj_str_get_data(e->value, &len);
                        esp32c3_wifi_ap_set_ssid(self->esp, len, (const uint8_t*)str);
                        break;
                    }
                    // case MP_QSTR_monitor: { //debug
                    //     mp_int_t value = mp_obj_get_int(e->value);
                    //     uint8_t buf[9 + 4];
                    //     memcpy(buf, "allmulti\x00", 9);
                    //     nw_put_le32(buf + 9, value);
                    //     esp32c3_ioctl(self->esp, ESP32C3_IOCTL_SET_VAR, 9 + 4, buf, self->itf);
                    //     nw_put_le32(buf, value);
                    //     esp32c3_ioctl(self->esp, ESP32C3_IOCTL_SET_MONITOR, 4, buf, self->itf);
                    //     if (value) {
                    //         self->esp->trace_flags |= ESP32C3_TRACE_MAC;
                    //     } else {
                    //         self->esp->trace_flags &= ~ESP32C3_TRACE_MAC;
                    //     }
                    //     break;
                    // }
                    case MP_QSTR_password: {
                        size_t len;
                        const char *str = mp_obj_str_get_data(e->value, &len);
                        esp32c3_wifi_ap_set_password(self->esp, len, (const uint8_t*)str);
                        break;
                    }
                    // case MP_QSTR_pm: {
                    //     esp32c3_wifi_pm(self->esp, mp_obj_get_int(e->value));
                    //     break;
                    // }
                    // case MP_QSTR_trace: {
                    //     self->esp->trace_flags = mp_obj_get_int(e->value);
                    //     break;
                    // }
                    case MP_QSTR_txpower: {
                        mp_int_t dbm = mp_obj_get_int(e->value);
                        if( dbm < 8 && dbm > 72)
                            mp_raise_ValueError("txpower must between [8,72]");
                        test_wifi_set_max_tx_power(dbm);
                        break;
                    }
                    default:
                        mp_raise_ValueError("unknown config param");
                }
            }
        }

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(network_esp32c3_config_obj, 1, network_esp32c3_config);

#if 0
STATIC mp_obj_t network_esp32c3_status(size_t n_args, const mp_obj_t *args) {
    network_esp32c3_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    (void)self;

    if (n_args == 1) {
        // no arguments: return link status
        return MP_OBJ_NEW_SMALL_INT(esp32c3_tcpip_link_status(self->esp, self->itf));
    }

    // one argument: return status based on query parameter
    switch (mp_obj_str_get_qstr(args[1])) {
        case MP_QSTR_stations: {
            // return list of connected stations
            if (self->itf != ESP32_ITF_AP) {
                mp_raise_ValueError("AP required");
            }
            int num_stas;
            uint8_t macs[32 * 6];
            esp32c3_wifi_ap_get_stas(self->esp, &num_stas, macs);
            mp_obj_t list = mp_obj_new_list(num_stas, NULL);
            for (int i = 0; i < num_stas; ++i) {
                mp_obj_t tuple[1] = {
                    mp_obj_new_bytes(&macs[i * 6], 6),
                };
                ((mp_obj_list_t*)MP_OBJ_TO_PTR(list))->items[i] = mp_obj_new_tuple(1, tuple);
            }
            return list;
        }
    }

    mp_raise_ValueError("unknown status param");
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(network_esp32c3_status_obj, 1, 2, network_esp32c3_status);

#endif

/*******************************************************************************/
// class bindings

STATIC const mp_rom_map_elem_t network_esp32c3_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_send_ethernet), MP_ROM_PTR(&network_esp32c3_send_ethernet_obj) },

    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&network_esp32c3_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_active), MP_ROM_PTR(&network_esp32c3_active_obj) }, // active lwip link
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&network_esp32c3_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&network_esp32c3_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&network_esp32c3_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected), MP_ROM_PTR(&network_esp32c3_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig), MP_ROM_PTR(&network_esp32c3_ifconfig_obj) },
    // { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&network_esp32c3_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_config), MP_ROM_PTR(&network_esp32c3_config_obj) },
};
STATIC MP_DEFINE_CONST_DICT(network_esp32c3_locals_dict, network_esp32c3_locals_dict_table);

const mp_obj_type_t mp_network_esp32c3_type = {
    { &mp_type_type },
    .name = MP_QSTR_ESP32C3,
    .print = network_esp32c3_print,
    .make_new = network_esp32c3_make_new,
    .locals_dict = (mp_obj_dict_t*)&network_esp32c3_locals_dict,
};

#endif // MICROPY_PY_NETWORK_ESP32C3
