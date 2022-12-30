// #include <bsp.h>
// #include <sysctl.h>
#include "FreeRTOS.h"
// #include "task.h"

#include "spi_drv.h"

#include "trace.h"
#include "esp_common/common.h"

#include "netdev_api.h"

#include "ctrl_api.h"
#include "util.h"
#include "esp32_control.h"

#include "esp32_if.h"
//#include "lib/netutils/dhcpserver.h"

/** Constants/Macros **/
//#define LWIP_TASK_STACK_SIZE     4096

// #define TASK_STACK_SIZE (32 * 1024)
// #define TASK_STACK_LEN (TASK_STACK_SIZE / sizeof(StackType_t))


/** Function declaration **/
// static void init_sta(void);
// static void init_ap(void);

static void sta_rx_callback(struct network_handle *net_handle);
static void ap_rx_callback(struct network_handle *net_handle);
//static void lwip_task(void const *arg);
//static void esp_network_lwip_init(void);
struct network_handle *esp_net_handle[2];
//struct network_handle *sta_handle, *ap_handle;
//static thread_handle_t lwip_task_id = 0;

//------------------------network----------------------------

/**
  * @brief start station mode network path
  * @param None
  * @retval None
  */
static void init_sta(void)
{
	esp_net_handle[ESP32_ITF_STA] = network_open(STA_INTERFACE, sta_rx_callback);
	assert(esp_net_handle[ESP32_ITF_STA]);
}

/**
  * @brief start softap mode network path
  * @param None
  * @retval None
  */
static void init_ap(void)
{
	esp_net_handle[ESP32_ITF_AP] = network_open(SOFTAP_INTERFACE, ap_rx_callback);
	assert(esp_net_handle[ESP32_ITF_AP]);
}

//-------------------lwip port ----------------------------


#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"

#include "lwip/err.h"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/dns.h"
//#include "lwip/tcpip.h"
#include "lwip/priv/tcpip_priv.h"
#include "netif/ethernet.h"


// #include "lwip/apps/netbiosns.h"
// #include "lwip/apps/httpd.h" // http服务器


//struct netif netif_esp32c3;
struct netif *g_netif_esp32c3[2]={NULL};


/**
  * @brief Station mode rx callback
  * @param  net_handle - station network handle
  * @retval None
  */
static void sta_rx_callback(struct network_handle *net_handle)
{
	struct esp_pbuf *rx_buffer = NULL;
	struct netif * netif = &esp32c3_state.netif[ESP32_ITF_STA];

	rx_buffer = network_read(net_handle, 0);
	if(rx_buffer->len){
		//print_hex_dump(rx_buffer->payload, rx_buffer->len, "sta_rx_callback recv data");	
		/* Allocate pbuf from pool (avoid using heap in interrupts) */
		struct pbuf* p = pbuf_alloc(PBUF_RAW, rx_buffer->len, PBUF_POOL);

		if (p != NULL) {
            pbuf_take(p, rx_buffer->payload, rx_buffer->len);
            //if (netif_esp32c3.input(p, &netif_esp32c3) != ERR_OK) {
            //if (g_netif_esp32c3[0]->input(p, g_netif_esp32c3[0]) != ERR_OK) {
			if (netif->input(p, netif) != ERR_OK) {
                pbuf_free(p);
            }
        }
	}

	if (rx_buffer) {
		free(rx_buffer->payload);
		rx_buffer->payload = NULL;
		free(rx_buffer);
		rx_buffer = NULL;
	}
}

/**
  * @brief Ap mode rx callback
  * @param  net_handle - ap network handle
  * @retval None
  */
static void ap_rx_callback(struct network_handle *net_handle)
{
	struct esp_pbuf *rx_buffer = NULL;
	struct netif * netif = &esp32c3_state.netif[ESP32_ITF_AP];

	rx_buffer = network_read(net_handle, 0);
	if(rx_buffer->len){
		/* Allocate pbuf from pool (avoid using heap in interrupts) */
		struct pbuf* p = pbuf_alloc(PBUF_RAW, rx_buffer->len, PBUF_POOL);

		if (p != NULL) {
            pbuf_take(p, rx_buffer->payload, rx_buffer->len);
            //if (netif_esp32c3.input(p, &netif_esp32c3) != ERR_OK) {
            //if (g_netif_esp32c3[1]->input(p, g_netif_esp32c3[1]) != ERR_OK) {
			if (netif->input(p, netif) != ERR_OK) {
                pbuf_free(p);
            }
        }
	}

	if (rx_buffer) {
		free(rx_buffer->payload);
		rx_buffer->payload = NULL;
		free(rx_buffer);
		rx_buffer = NULL;
	}
}
#define netifGUARD_BLOCK_TIME			( 250 )
static err_t esp32_netif_output(struct netif *netif, struct pbuf *p)
{
	static xSemaphoreHandle xTxSemaphore = NULL;
	struct esp_pbuf *snd_buffer = NULL;
	int ret;
	int itf = netif->name[1] - '0';
	if (xTxSemaphore == NULL)
  	{
    	vSemaphoreCreateBinary (xTxSemaphore);
 	}

	if (xSemaphoreTake(xTxSemaphore, netifGUARD_BLOCK_TIME))
	{
		// 申请发送缓冲
		snd_buffer = (struct esp_pbuf *)malloc(sizeof(struct esp_pbuf));
		assert(snd_buffer);
		//snd_buffer->payload = p->payload;

		snd_buffer->payload = (uint8_t *)malloc(p->tot_len);
		assert(snd_buffer->payload);

		snd_buffer->len = p->tot_len;

		//lock_interrupts();
		pbuf_copy_partial(p, snd_buffer->payload, p->tot_len, 0);
		/* Start MAC transmit here */
		//unlock_interrupts();
	//print_hex_dump(snd_buffer->payload, snd_buffer->len, "esp_net_handle send data");	
		ret = network_write(esp_net_handle[itf], snd_buffer);
		if (ret)
			printf("%s: Failed to send data\n\r", __func__);

		xSemaphoreGive(xTxSemaphore);
  	}

  	return ERR_OK;
}

static void netif_status_callback(struct netif *netif)
{
  	printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

static err_t esp32c3_netif_init(struct netif *netif)
{
	int itf = netif->name[1] - '0';
	netif->linkoutput = esp32_netif_output;
	netif->output     = etharp_output;
	//netif->output_ip6 = ethip6_output;
	netif->mtu        = 1500;
	netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;
	//MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);
	//34:b4:72:41:aa:78
	//SMEMCPY(netif->hwaddr, your_mac_address_goes_here, sizeof(netif->hwaddr));
	char mac[WIFI_MAX_STR_LEN]={0};
	if(itf == ESP32_ITF_STA){
		if (test_station_mode_get_mac_addr(mac)) {
			printf("Failed to get wifi mac\n\r");
			return FAILURE;
		}
	}
	else{
		if (test_softap_mode_get_mac_addr(mac)) {
			printf("Failed to get softap mac\n\r");
			return FAILURE;
		}
		printf("get softap mac finish\n\r");
	}
	convert_mac_to_bytes(&netif->hwaddr[0], mac);
	netif->hwaddr_len = sizeof(netif->hwaddr);

	return ERR_OK;
}

/* Exported types ------------------------------------------------------------*/
#define DHCP_START                 1
#define DHCP_WAIT_ADDRESS          2
#define DHCP_ADDRESS_ASSIGNED      3
#define DHCP_TIMEOUT               4
#define DHCP_LINK_DOWN             5

#define MAX_DHCP_TRIES        4
/*Static IP ADDRESS*/
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   10
/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   1  

uint8_t DHCP_state;
uint32_t IPaddress = 0;

void esp32c3_tcpip_init(esp32c3_t *self, int itf) {
    if(itf == ESP32_ITF_STA){
		init_sta(); // open net handle
    }
	else{
		init_ap();
    }

    ip_addr_t ipconfig[4];
    #if LWIP_IPV6
    #define IP(x) ((x).u_addr.ip4)
    #else
    #define IP(x) (x)
    #endif
    if (itf == 0) {
        // need to zero out to get isconnected() working
        IP4_ADDR(&IP(ipconfig[0]), 0, 0, 0, 0);
        IP4_ADDR(&IP(ipconfig[2]), 192, 168, 0, 1);
    } else {
        IP4_ADDR(&IP(ipconfig[0]), 192, 168, 4, 1);
        IP4_ADDR(&IP(ipconfig[2]), 192, 168, 4, 1);
    }
    IP4_ADDR(&IP(ipconfig[1]), 255, 255, 255, 0);
    IP4_ADDR(&IP(ipconfig[3]), 8, 8, 8, 8);
    #undef IP

    struct netif *n = &self->netif[itf];
    //g_netif_esp32c3[itf] = &self->netif[itf];//netif 句柄传递给全局变量，以便rx cb使用
    n->name[0] = 'w';
    n->name[1] = '0' + itf;
    #if LWIP_IPV6
    netif_add(n, &ipconfig[0].u_addr.ip4, &ipconfig[1].u_addr.ip4, &ipconfig[2].u_addr.ip4, self, esp32c3_netif_init, ethernet_input);
    #else
    netif_add(n, &ipconfig[0], &ipconfig[1], &ipconfig[2], self, esp32c3_netif_init, netif_input); //netif_input);
    #endif
    netif_set_hostname(n, "CanMV");
    netif_set_default(n);
    netif_set_up(n);
	//DHCP_state = DHCP_START;

    if (itf == ESP32_ITF_STA) {
        dns_setserver(0, &ipconfig[3]);
        dhcp_set_struct(n, &self->dhcp_client);
        dhcp_start(n);
    } else {
        dhcp_server_init(&self->dhcp_server, &ipconfig[0], &ipconfig[1]);
    }
    netif_set_link_up(n);
    //esp32c3_tcpip_set_link_up(self, itf);
    printf("lwip init finish\r\n");

    #if LWIP_MDNS_RESPONDER
    // TODO better to call after IP address is set
    char mdns_hostname[9];
    memcpy(&mdns_hostname[0], "CanMV", 4);
    mp_hal_get_mac_ascii(MP_HAL_MAC_WLAN0, 8, 4, &mdns_hostname[4]);
    mdns_hostname[8] = '\0';
    mdns_resp_add_netif(n, mdns_hostname, 60);
    #endif
}

void esp32c3_tcpip_deinit(esp32c3_t *self, int itf) {
    struct netif *n = &self->netif[itf];
    if (itf == ESP32_ITF_STA) {
        dhcp_stop(n);
    } else {
        dhcp_server_deinit(&self->dhcp_server);
    }
    #if LWIP_MDNS_RESPONDER
    mdns_resp_remove_netif(n);
    #endif
    for (struct netif *netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif == n) {
            netif_remove(netif);
            netif->ip_addr.addr = 0;
            netif->flags = 0;
        }
    }
}

void esp32c3_tcpip_set_link_up(esp32c3_t *self, int itf) {
    netif_set_link_up(&self->netif[itf]);
}

void esp32c3_tcpip_set_link_down(esp32c3_t *self, int itf) {
    netif_set_link_down(&self->netif[itf]);
}

int esp32c3_tcpip_link_status(esp32c3_t *self, int itf) {
    struct netif *netif = &self->netif[itf];
    if ((netif->flags & (NETIF_FLAG_UP | NETIF_FLAG_LINK_UP))
        == (NETIF_FLAG_UP | NETIF_FLAG_LINK_UP)) {
        if (netif->ip_addr.addr != 0) {
            return ESP32_LINK_UP;
        } else {
            return ESP32_LINK_NOIP;
        }
    } else {
        return ESP32_LINK_DOWN;
        //return cyw43_wifi_link_status(self, itf);
    }
}

int esp32c3_send_ethernet(esp32c3_t *self, int itf, size_t len, const void *buf) {
	struct esp_pbuf *snd_buffer = NULL;
	int ret;

	// 申请发送缓冲
	snd_buffer = (struct esp_pbuf *)malloc(sizeof(struct esp_pbuf));
	assert(snd_buffer);
	snd_buffer->payload = (uint8_t *)malloc(len);
	assert(snd_buffer->payload);

	snd_buffer->len = len;
    memcpy(snd_buffer->payload, buf, len);

	ret = network_write(esp_net_handle[itf], snd_buffer);
	if (ret)
		printf("%s: Failed to send data\n\r", __func__);

  	return ret;

}

//----------------------------old fun
#if 1
/* 显示DHCP分配的IP地址 */
#if LWIP_DHCP
//extern void dns_test(void);
static void display_ip(void)
{
	static uint8_t ip_displayed = 0;
	struct dhcp *dhcp;
  	struct netif * netif = &esp32c3_state.netif[ESP32_ITF_STA];

	if (dhcp_supplied_address(netif))
	{
		if (ip_displayed == 0)
		{
			ip_displayed = 1;
			dhcp = netif_dhcp_data(netif);
			printf("DHCP supplied address!\n");
			printf("IP address: %s\n", ip4addr_ntoa(&dhcp->offered_ip_addr));
			printf("Subnet mask: %s\n", ip4addr_ntoa(&dhcp->offered_sn_mask));
			printf("Default gateway: %s\n", ip4addr_ntoa(&dhcp->offered_gw_addr));
	#if LWIP_DNS
			printf("DNS Server: %s\n", ip4addr_ntoa(dns_getserver(0)));
			//dns_test();
	#endif
		}
	}
	else
		ip_displayed = 0;
}
#endif

// static void esp_network_lwip_init(void)
// {
// 	struct ip4_addr ipaddr, netmask, gw;
// 	lwip_init();

// 	netif_add(&netif_esp32c3, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY, NULL, esp32c3_netif_init, netif_input);
// 	netif_esp32c3.name[0] = 'e';
// 	netif_esp32c3.name[1] = '0';
// 	//netif_create_ip6_linklocal_address(&netif, 1);
// 	//netif.ip6_autoconfig_enabled = 1;
// 	netif_esp32c3.hostname = "lyr_lwip"; // ★路由器中显示的名称
// 	//netif_set_status_callback(&netif_esp32c3, netif_status_callback);
// 	#if LWIP_DNS
// 		IP4_ADDR(&ipaddr, 8, 8, 8, 8); // 首选DNS服务器
// 		dns_setserver(0, &ipaddr);
// 		IP4_ADDR(&ipaddr, 8, 8, 4, 4); // 备用DNS服务器
// 		dns_setserver(1, &ipaddr);
// 	#endif
// 	netif_set_default(&netif_esp32c3);
// 	printf("lwip init finish -------------\r\n");
// 	netif_set_up(&netif_esp32c3);
// 	netif_set_link_up(&netif_esp32c3);
// 	/* Start DHCP and HTTPD */
// 	dhcp_start(&netif_esp32c3 );

// 	// netbiosns_init();
// 	// netbiosns_set_name("k210test"); // 计算机名 (测试方法: (1)ping 计算机名 (2)用浏览器访问http://计算机名/) // 注意: 路由器中显示的名称可以和计算机名不同
// 	// httpd_init(); // 初始化lwip自带的http服务器 (lwip/apps/httpd/*)
// }

void lwip_task(void const *arg)
{
	
	while (1) {

        //printf("sys_check_timeouts  ...\r\n");
		sys_check_timeouts();
		// 显示DHCP获取到的IP地址
		#if LWIP_DHCP
		display_ip();
		#endif
		esp_msleep(50);
	}
}


#endif

u32_t
sys_now(void)
{
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}


#if 0 //SYS_LIGHTWEIGHT_PROT

// sys_prot_t
// sys_arch_protect(void)
// {
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
//   BaseType_t ret;
//   LWIP_ASSERT("sys_arch_protect_mutex != NULL", sys_arch_protect_mutex != NULL);

//   ret = xSemaphoreTakeRecursive(sys_arch_protect_mutex, portMAX_DELAY);
//   LWIP_ASSERT("sys_arch_protect failed to take the mutex", ret == pdTRUE);
// #else /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
//   taskENTER_CRITICAL();
// #endif /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
//   {
//     /* every nested call to sys_arch_protect() returns an increased number */
//     sys_prot_t ret = sys_arch_protect_nesting;
//     sys_arch_protect_nesting++;
//     LWIP_ASSERT("sys_arch_protect overflow", sys_arch_protect_nesting > ret);
//     return ret;
//   }
// #else
//   return 1;
// #endif
// }

// void
// sys_arch_unprotect(sys_prot_t pval)
// {
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
//   BaseType_t ret;
// #endif
// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_SANITY_CHECK
//   LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting > 0);
//   sys_arch_protect_nesting--;
//   LWIP_ASSERT("unexpected sys_arch_protect_nesting", sys_arch_protect_nesting == pval);
// #endif

// #if LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX
//   LWIP_ASSERT("sys_arch_protect_mutex != NULL", sys_arch_protect_mutex != NULL);

//   ret = xSemaphoreGiveRecursive(sys_arch_protect_mutex);
//   LWIP_ASSERT("sys_arch_unprotect failed to give the mutex", ret == pdTRUE);
// #else /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
//   taskEXIT_CRITICAL();
// #endif /* LWIP_FREERTOS_SYS_ARCH_PROTECT_USES_MUTEX */
//   LWIP_UNUSED_ARG(pval);
// }

//#define MICROPY_BEGIN_ATOMIC_SECTION	portDISABLE_INTERRUPTS

sys_prot_t sys_arch_protect() {
    //return (sys_prot_t)MICROPY_BEGIN_ATOMIC_SECTION();
	//portDISABLE_INTERRUPTS();
	sysctl_disable_irq();

	return 1;
}

void sys_arch_unprotect(sys_prot_t state) {
    //MICROPY_END_ATOMIC_SECTION((mp_uint_t)state);
	//portENABLE_INTERRUPTS();
    sysctl_enable_irq();
}

#endif /* SYS_LIGHTWEIGHT_PROT */

#include "lwip/ip4_addr.h"

void LwIP_DHCP_task(void * pvParameters)
{
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  uint8_t iptab[4] = {0};
  uint8_t iptxt[20];
  uint8_t mactxt[40];
	struct netif * netif = &esp32c3_state.netif[ESP32_ITF_STA];
  for (;;)
  {
	switch (DHCP_state)
	{
	case DHCP_START:
	{
		DHCP_state = DHCP_WAIT_ADDRESS;
		dhcp_start(netif);
		/* IP address should be set to 0 
			every time we want to assign a new DHCP address */
		IPaddress = 0;

		printf("     Looking for    \r\n");
		printf("     DHCP server    \r\n");
		printf("     please wait... \r\n");	  
	}
	break;
	
	case DHCP_WAIT_ADDRESS:
	{
		/* Read the new IP address */
		IPaddress = netif->ip_addr.addr;//netif->ip_addr.addr;

		if (IPaddress!=0) 
		{
		DHCP_state = DHCP_ADDRESS_ASSIGNED;	

		/* Stop DHCP */
		//dhcp_stop(&gnetif);
		
		iptab[0] = (uint8_t)(IPaddress >> 24);
		iptab[1] = (uint8_t)(IPaddress >> 16);
		iptab[2] = (uint8_t)(IPaddress >> 8);
		iptab[3] = (uint8_t)(IPaddress);

		sprintf((char*)iptxt, " %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

		/* Display the IP address */
		printf("IP address assigned by a DHCP server:\r\n");
		sprintf((char*)iptxt, " %d.%d.%d.%d", (uint8_t)(netif->ip_addr.addr), (uint8_t)(netif->ip_addr.addr>>8), (uint8_t)(netif->ip_addr.addr>>16), (uint8_t)(netif->ip_addr.addr>>24));
		printf("IP addr: %s\r\n", iptxt);
		
		sprintf((char*)iptxt, " %d.%d.%d.%d", (uint8_t)(netif->netmask.addr), (uint8_t)(netif->netmask.addr>>8), (uint8_t)(netif->netmask.addr>>16), (uint8_t)(netif->netmask.addr>>24));
		printf("Netmask addr: %s\r\n", iptxt);
		
		sprintf((char*)iptxt, " %d.%d.%d.%d", (uint8_t)(netif->gw.addr), (uint8_t)(netif->gw.addr>>8), (uint8_t)(netif->gw.addr>>16), (uint8_t)(netif->gw.addr>>24));
		printf("GW addr: %s\r\n", iptxt);
		
		sprintf((char*)mactxt, " %02x-%02x-%02x-%02x-%02x-%02x", netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);
		printf("MAC addr: %s\r\n", mactxt);

		//LED1(ON);
		}
		else
		{
		struct dhcp *dhcp = netif_dhcp_data(netif);
		/* DHCP timeout */
		if (dhcp->tries > MAX_DHCP_TRIES)
		{
			DHCP_state = DHCP_TIMEOUT;

			/* Stop DHCP */
			dhcp_stop(netif);

			/* Static address used */
			IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
			IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
			IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
			netif_set_addr(netif, &ipaddr , &netmask, &gw);
	
			printf("    DHCP timeout    \r\n");

			iptab[0] = IP_ADDR3;
			iptab[1] = IP_ADDR2;
			iptab[2] = IP_ADDR1;
			iptab[3] = IP_ADDR0;

			sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

			printf("  Static IP address   \r\n");
			printf("%s\r\n", iptxt);

			//LED1(ON);
		}
		}
	}
	break;
	
	default: break;
	}

	/* wait 250 ms */
	vTaskDelay(25);
   }
}
