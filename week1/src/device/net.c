// week1/src/device/net.c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "uxn.h"

#define NETWORK_DEV_ADDR 0xD0   
#define PORT_COUNT       15    

enum {
  P_SOCKET_H = 2,  
  P_SOCKET_L,      
  P_ADDR_H,        
  P_ADDR_L,        
  P_OPT_H,        
  P_OPT_L,         
  P_BUF_H,         
  P_BUF_L,         
  P_LEN_H,         
  P_LEN_L,         
  P_FLAGS,         
  P_STATUS,        
  P_CALL           
};

static Device network_dev;

static void
network_init(Device *dev) {
  dev->port_count = PORT_COUNT;
}

static void
network_deo(Uint8 addr, Uint8 value) {
  Uint8 port = addr & 0x0F;
  network_dev.data[port] = value;
  if (port == P_CALL) {
    
    uint16_t sockfd = (network_dev.data[P_SOCKET_H] << 8) |
                       network_dev.data[P_SOCKET_L];
    uint16_t addr_ptr = (network_dev.data[P_ADDR_H] << 8) |
                         network_dev.data[P_ADDR_L];
    uint16_t buf_ptr  = (network_dev.data[P_BUF_H] << 8) |
                         network_dev.data[P_BUF_L];
    uint16_t len      = (network_dev.data[P_LEN_H] << 8) |
                         network_dev.data[P_LEN_L];
    int status = -1;
    switch (value) {
      case 0: /* socket */
        status = socket(AF_INET, SOCK_STREAM, 0);
        break;
      case 1: /* bind */
        status = bind(sockfd,
                      (struct sockaddr *)&network_dev.data[addr_ptr],
                      sizeof(struct sockaddr_in));
        break;
      /* TODO: 2=listen,3=accept,4=connect,5=send,6=recv,7=close */
    }
    network_dev.data[P_STATUS] = (Uint8)status;
  }
}

static Uint8
network_dei(Uint8 addr) {
  return network_dev.data[addr & 0x0F];
}

Device device_net = {
  .name  = "NET",
  .init  = network_init,
  .write = network_deo,
  .read  = network_dei
};

