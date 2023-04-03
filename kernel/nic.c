#include "nic.h"

/*
 *  The files `nic.c` & `nic.h` represent the NIC (Network Interface Driver), while the files `network.c` & `network.h` represent the
 *  network protocol stack...
 */

/* Use this function to initiailize the corresponding ethernet driver... */
void eth_driver_init() { }

/* Send a packet through ethernet... */
void eth_send_packet(unsigned char *packet, int len) { }

/* Recieve a packet through ethernet... */
int eth_recv_packet(unsigned char *packet, int maxlen) { /* Return -1 if an error happens... */ }

/* A loop for the ethernet driver to use... */
void eth_driver_loop() { /* call eth_recv_packet() when a packet is received */ }