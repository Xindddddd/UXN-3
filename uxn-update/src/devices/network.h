/*
 * Network Device Header for Uxn
 * src/devices/network.h
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

/* Forward declarations to avoid including uxn.h */
typedef uint8_t Uint8;

/* Device interface functions */
Uint8 network_dei(Uint8 addr);
void network_deo(Uint8 addr);
void network_cleanup(void);
void network_tick(void);

#endif