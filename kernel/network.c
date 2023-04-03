#include "network.h"

/* Initialize the network stack... */
void net_init() {
  /* Initialize drivers here! */
}

/* Send a packet to an IP address... */
void net_send_packet(unsigned int dest_ip, unsigned char protocol, unsigned char *data, int len) {
    /* Ask the driver to send a packet... */
}

/* Recieve a packet... */
void net_recv_packet(unsigned char *packet, int len) {
    /* Handle the recieved packet... */
}