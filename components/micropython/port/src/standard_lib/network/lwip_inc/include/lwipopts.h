#ifndef K210_LWIP_LWIPOPTS_H
#define K210_LWIP_LWIPOPTS_H

// This protection is not needed, instead we execute all lwIP code at PendSV priority
// #define SYS_ARCH_DECL_PROTECT(lev) do { } while (0)
// #define SYS_ARCH_PROTECT(lev) do { } while (0)
// #define SYS_ARCH_UNPROTECT(lev) do { } while (0)


#define NO_SYS                          1 // 无操作系统

#define MEM_ALIGNMENT                   8 // k210是64位的单片机, 因此是8字节对齐的

#define LWIP_CHKSUM_ALGORITHM           2 //3
//#define LWIP_CHECKSUM_CTRL_PER_NETIF    1
 
#define SYS_LIGHTWEIGHT_PROT            1 // 0不进行临界区保护
#include <stdint.h>
typedef uint32_t sys_prot_t;

#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0
#define LWIP_STATS                      0
#define LWIP_NETIF_HOSTNAME             1

// 配置DHCP
#define LWIP_IPV6                       0
#define LWIP_DHCP                       1
#define LWIP_DHCP_CHECK_LINK_UP         1
#define DHCP_DOES_ARP_CHECK             0 // to speed DHCP up


// 配置DNS
#define LWIP_DNS                        1

extern uint32_t rng_get(void);
#define LWIP_RAND() rng_get()

#include <py/mpconfig.h>
#include <py/misc.h>
#include <py/mphal.h>

#ifdef MICROPY_PY_LWIP_SLIP
#define LWIP_HAVE_SLIPIF 1
#endif

// For now, we can simply define this as a macro for the timer code. But this function isn't
// universal and other ports will need to do something else. It may be necessary to move
// things like this into a port-provided header file.
//#define sys_now mp_hal_ticks_ms

#define LWIP_IGMP                       1

// default
// lwip takes 15800 bytes; TCP d/l: 380k/s local, 7.2k/s remote
// TCP u/l is very slow

#if 0
// lwip takes 19159 bytes; TCP d/l and u/l are around 320k/s on local network
#define MEM_SIZE (5000)
#define TCP_WND (4 * TCP_MSS)
#define TCP_SND_BUF (4 * TCP_MSS)
#endif

#if 0
// lwip takes 26700 bytes; TCP dl/ul are around 750/600 k/s on local network
#define MEM_SIZE (8000)
#define TCP_MSS (800)
#define TCP_WND (8 * TCP_MSS)
#define TCP_SND_BUF (8 * TCP_MSS)
#define MEMP_NUM_TCP_SEG (32)
#endif

#if 0
// lwip takes 45600 bytes; TCP dl/ul are around 1200/1000 k/s on local network
#define MEM_SIZE (16000)
#define TCP_MSS (1460)
#define TCP_WND (8 * TCP_MSS)
#define TCP_SND_BUF (8 * TCP_MSS)
#define MEMP_NUM_TCP_SEG (32)
#endif

#if 0
#define MEM_SIZE (25*1024)
#define TCP_MSS (1460)
#define TCP_WND (11 * TCP_MSS)
#define TCP_SND_BUF (11 * TCP_MSS)
#define MEMP_NUM_TCP_SEG (150)
#endif

#if 1
// lwip TCP rx/tx are around 1600/1200 k/s on local network
//内存堆 heap 大小
#define MEM_SIZE (25*1024)

/* memp 结构的 pbuf 数量,如果应用从 ROM 或者静态存储区发送大量数据时这个值应该设置大一点 */
#define MEMP_NUM_PBUF 25

/* 最多同时在 TCP 缓冲队列中的报文段数量 */
#define MEMP_NUM_TCP_SEG 150

/* 内存池大小 */
#define PBUF_POOL_SIZE 65

/* 每个 pbuf 内存池大小 */
#define PBUF_POOL_BUFSIZE \
LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)

/* 最大 TCP 报文段， TCP_MSS = (MTU - IP 报头大小 - TCP 报头大小 */
#define TCP_MSS (1500 - 40)

/* TCP 发送缓冲区大小（字节） */
#define TCP_SND_BUF (11*TCP_MSS)

/* TCP 发送缓冲区队列的最大长度 */
#define TCP_SND_QUEUELEN (8* TCP_SND_BUF/TCP_MSS)

/* TCP 接收窗口大小 */
#define TCP_WND (11*TCP_MSS)
#endif

#if 0
// lwip TCP rx/tx are around 1600/1200 k/s on local network
//内存堆 heap 大小
#define MEM_SIZE (25*1024)

/* memp 结构的 pbuf 数量,如果应用从 ROM 或者静态存储区发送大量数据时这个值应该设置大一点 */
#define MEMP_NUM_PBUF 64 //25

/* 最多同时在 TCP 缓冲队列中的报文段数量 */
#define MEMP_NUM_TCP_SEG 256  //150

/* 内存池大小 */
#define PBUF_POOL_SIZE 128 //65

/* 每个 pbuf 内存池大小 */
#define PBUF_POOL_BUFSIZE \
LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)

/* 最大 TCP 报文段， TCP_MSS = (MTU - IP 报头大小 - TCP 报头大小 */
#define TCP_MSS (1500 - 40)

/* TCP 发送缓冲区大小（字节） */
#define TCP_SND_BUF (11*TCP_MSS)  //16

/* TCP 发送缓冲区队列的最大长度 */
#define TCP_SND_QUEUELEN (8* TCP_SND_BUF/TCP_MSS) //8

/* TCP 接收窗口大小 */
#define TCP_WND (18*TCP_MSS) //18
#endif


#endif // K210_LWIP_LWIPOPTS_H
