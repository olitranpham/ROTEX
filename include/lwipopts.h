#pragma once

// Required for lwIP Sequential API (netconn/sockets)
#define NO_SYS                          0

// Enable netconn + sockets (Sequential API)
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1
#define LWIP_COMPAT_SOCKETS             0

// Pico SDK / cyw43 integration expects these commonly:
#define LWIP_TCPIP_CORE_LOCKING         0
#define SYS_LIGHTWEIGHT_PROT            1

// Memory options (reasonable defaults)
#define MEM_LIBC_MALLOC                 0
#define MEMP_MEM_MALLOC                 0
#define MEM_ALIGNMENT                   4

// Protocols
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_ICMP                       1
#define LWIP_DNS                        1
#define LWIP_DHCP                       1
#define LWIP_ARP                        1

// Buffers (safe starting values)
#define TCP_MSS                         1460
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_WND                         (4 * TCP_MSS)
#define PBUF_POOL_BUFSIZE               1600

// Debug off
#define LWIP_DEBUG                      0