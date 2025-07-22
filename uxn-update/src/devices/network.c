/*
 * Fixed Network Device for Uxn
 * src/devices/network.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "../uxn.h"

/* Network device state */
static struct {
    int sockfd;
    int client_sockfd;
    int state;
    int type;
    char host[256];
    int port;
    struct sockaddr_in addr;
    int vector_pending;  /* 是否需要触发向量 */
} net = {0};

/* Device states */
#define NET_CLOSED     0x00
#define NET_CONNECTING 0x01  
#define NET_CONNECTED  0x02
#define NET_LISTENING  0x03
#define NET_ERROR      0x04

/* Device types */
#define NET_TCP_CLIENT 0x00
#define NET_TCP_SERVER 0x01

/* Commands */
#define NET_CMD_CONNECT 0x01
#define NET_CMD_LISTEN  0x02
#define NET_CMD_CLOSE   0x03
#define NET_CMD_ACCEPT  0x04

/* External variables from uxn.c */
extern Uxn uxn;

/* Helper function to trigger network vector */
static void trigger_network_vector(void) {
    Uint16 vector = (uxn.dev[0xa0] << 8) | uxn.dev[0xa1];
    if (vector != 0x0000) {
        printf("[DEBUG] Triggering network vector at 0x%04x, state=%d\n", vector, net.state);
        uxn_eval(vector);
    }
}

/* Helper functions */
static void net_close(void) {
    printf("[DEBUG] Closing network connection\n");
    if (net.sockfd > 0) {
        close(net.sockfd);
        net.sockfd = 0;
    }
    if (net.client_sockfd > 0) {
        close(net.client_sockfd);
        net.client_sockfd = 0;
    }
    net.state = NET_CLOSED;
    trigger_network_vector();
}

static int net_create_socket(void) {
    printf("[DEBUG] Creating socket, type=%d\n", net.type);
    net.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (net.sockfd < 0) {
        printf("[DEBUG] Socket creation failed: %s\n", strerror(errno));
        net.state = NET_ERROR;
        return -1;
    }
    
    /* Set SO_REUSEADDR for server */
    if (net.type == NET_TCP_SERVER) {
        int yes = 1;
        setsockopt(net.sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    }
    
    printf("[DEBUG] Socket created successfully, fd=%d\n", net.sockfd);
    return 0;
}

static void net_connect(void) {
    printf("[DEBUG] Connecting to %s:%d\n", net.host, net.port);
    if (net_create_socket() < 0) return;
    
    memset(&net.addr, 0, sizeof(net.addr));
    net.addr.sin_family = AF_INET;
    net.addr.sin_port = htons(net.port);
    inet_aton(net.host, &net.addr.sin_addr);
    
    int result = connect(net.sockfd, (struct sockaddr*)&net.addr, sizeof(net.addr));
    if (result == 0) {
        printf("[DEBUG] Connected immediately\n");
        net.state = NET_CONNECTED;
        trigger_network_vector();
    } else if (errno == EINPROGRESS) {
        printf("[DEBUG] Connection in progress\n");
        net.state = NET_CONNECTING;
        trigger_network_vector();
    } else {
        printf("[DEBUG] Connection failed: %s\n", strerror(errno));
        net.state = NET_ERROR;
        trigger_network_vector();
    }
}

static void net_listen(void) {
    printf("[DEBUG] Starting to listen on port %d\n", net.port);
    if (net_create_socket() < 0) return;
    
    memset(&net.addr, 0, sizeof(net.addr));
    net.addr.sin_family = AF_INET;
    net.addr.sin_port = htons(net.port);
    net.addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(net.sockfd, (struct sockaddr*)&net.addr, sizeof(net.addr)) < 0) {
        printf("[DEBUG] Bind failed: %s\n", strerror(errno));
        net.state = NET_ERROR;
        trigger_network_vector();
        return;
    }
    
    if (listen(net.sockfd, 5) < 0) {
        printf("[DEBUG] Listen failed: %s\n", strerror(errno));
        net.state = NET_ERROR;
        trigger_network_vector();
        return;
    }
    
    printf("[DEBUG] Successfully listening on port %d\n", net.port);
    net.state = NET_LISTENING;
    trigger_network_vector();
}

static void net_accept(void) {
    printf("[DEBUG] Accepting connection\n");
    if (net.state != NET_LISTENING) {
        printf("[DEBUG] Not in listening state, current state=%d\n", net.state);
        return;
    }
    
    socklen_t addr_len = sizeof(net.addr);
    net.client_sockfd = accept(net.sockfd, (struct sockaddr*)&net.addr, &addr_len);
    
    if (net.client_sockfd > 0) {
        printf("[DEBUG] Client accepted, client_fd=%d\n", net.client_sockfd);
        net.state = NET_CONNECTED;
        trigger_network_vector();
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("[DEBUG] Accept failed: %s\n", strerror(errno));
        }
    }
}

static void net_check_connection(void) {
    if (net.state != NET_CONNECTING) return;
    
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(net.sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
        if (error == 0) {
            printf("[DEBUG] Connection established\n");
            net.state = NET_CONNECTED;
            trigger_network_vector();
        } else {
            printf("[DEBUG] Connection failed with error: %s\n", strerror(error));
            net.state = NET_ERROR;
            trigger_network_vector();
        }
    }
}

static int net_get_socket(void) {
    return (net.type == NET_TCP_SERVER && net.client_sockfd > 0) ? 
           net.client_sockfd : net.sockfd;
}

/* Check for pending network events */
void network_tick(void) {
    if (net.state == NET_LISTENING && net.sockfd > 0) {
        /* Check for pending connections */
        fd_set readfds;
        struct timeval timeout = {0, 0};
        FD_ZERO(&readfds);
        FD_SET(net.sockfd, &readfds);
        
        if (select(net.sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(net.sockfd, &readfds)) {
                printf("[DEBUG] New connection pending\n");
                net.state = NET_LISTENING;  /* Signal that accept is ready */
                trigger_network_vector();
            }
        }
    }
    
    if (net.state == NET_CONNECTED) {
        int sockfd = net_get_socket();
        if (sockfd > 0) {
            /* Check for incoming data */
            fd_set readfds;
            struct timeval timeout = {0, 0};
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            
            if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
                if (FD_ISSET(sockfd, &readfds)) {
                    printf("[DEBUG] Data available for reading\n");
                    trigger_network_vector();
                }
            }
        }
    }
}

/* Public interface */
Uint8 network_dei(Uint8 addr) {
    switch (addr & 0x0f) {
        case 0x02: /* state */
            net_check_connection();
            printf("[DEBUG] State requested: %d\n", net.state);
            return net.state;
        case 0x08: /* length - available data (simplified) */
            return 0x00;
        case 0x09:
            return 0x00;
        default:
            return uxn.dev[addr];
    }
}

void network_deo(Uint8 addr) {
    Uint8 value = uxn.dev[addr];
    
    switch (addr & 0x0f) {
        case 0x02: /* state/command */
            printf("[DEBUG] Network command: %d\n", value);
            switch (value) {
                case NET_CMD_CONNECT:
                    if (net.type == NET_TCP_CLIENT) net_connect();
                    break;
                case NET_CMD_LISTEN:
                    if (net.type == NET_TCP_SERVER) net_listen();
                    break;
                case NET_CMD_CLOSE:
                    net_close();
                    break;
                case NET_CMD_ACCEPT:
                    if (net.type == NET_TCP_SERVER) net_accept();
                    break;
            }
            break;
            
        case 0x03: /* type */
            printf("[DEBUG] Setting network type: %d\n", value);
            net.type = value;
            break;
            
        case 0x05: /* host address (low byte) */
            {
                Uint16 host_addr = (uxn.dev[addr - 1] << 8) | value;
                int i;
                for (i = 0; i < 255 && uxn.ram[host_addr + i] != 0; i++) {
                    net.host[i] = uxn.ram[host_addr + i];
                }
                net.host[i] = '\0';
                printf("[DEBUG] Host set to: %s\n", net.host);
            }
            break;
            
        case 0x07: /* port (low byte) */
            net.port = (uxn.dev[addr - 1] << 8) | value;
            printf("[DEBUG] Port set to: %d\n", net.port);
            break;
            
        case 0x0b: /* read address (low byte) - trigger read */
            {
                Uint16 read_addr = (uxn.dev[addr - 1] << 8) | value;
                Uint16 max_len = (uxn.dev[0xa8] << 8) | uxn.dev[0xa9];
                int sockfd = net_get_socket();
                
                printf("[DEBUG] Read request: addr=0x%04x, max_len=%d, sockfd=%d, state=%d\n", 
                       read_addr, max_len, sockfd, net.state);
                
                if (sockfd > 0 && net.state == NET_CONNECTED && max_len > 0) {
                    int bytes_read = recv(sockfd, &uxn.ram[read_addr], max_len, 0);
                    if (bytes_read < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            printf("[DEBUG] Read error: %s\n", strerror(errno));
                            net.state = NET_ERROR;
                        }
                        bytes_read = 0;
                    } else if (bytes_read == 0) {
                        printf("[DEBUG] Connection closed by peer\n");
                        net.state = NET_CLOSED;
                    } else {
                        printf("[DEBUG] Read %d bytes\n", bytes_read);
                    }
                    
                    /* Update length with actual bytes read */
                    uxn.dev[0xa8] = (bytes_read >> 8) & 0xff;
                    uxn.dev[0xa9] = bytes_read & 0xff;
                }
            }
            break;
            
        case 0x0d: /* write address (low byte) - trigger write */
            {
                Uint16 write_addr = (uxn.dev[addr - 1] << 8) | value;
                Uint16 len = (uxn.dev[0xa8] << 8) | uxn.dev[0xa9];
                int sockfd = net_get_socket();
                
                printf("[DEBUG] Write request: addr=0x%04x, len=%d, sockfd=%d, state=%d\n", 
                       write_addr, len, sockfd, net.state);
                
                if (sockfd > 0 && net.state == NET_CONNECTED && len > 0) {
                    int bytes_sent = send(sockfd, &uxn.ram[write_addr], len, 0);
                    if (bytes_sent < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            printf("[DEBUG] Write error: %s\n", strerror(errno));
                            net.state = NET_ERROR;
                        }
                        bytes_sent = 0;
                    } else {
                        printf("[DEBUG] Sent %d bytes\n", bytes_sent);
                    }
                    
                    /* Update length with actual bytes sent */
                    uxn.dev[0xa8] = (bytes_sent >> 8) & 0xff;
                    uxn.dev[0xa9] = bytes_sent & 0xff;
                }
            }
            break;
    }
}

void network_cleanup(void) {
    net_close();
}