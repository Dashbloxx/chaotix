#pragma once

#define ETHERNET_MTU 1500

struct ethernet_header {
    unsigned char dest_mac[6];
    unsigned char src_mac[6];
    unsigned short ethertype;
};

void eth_driver_init();
void eth_send_packet(unsigned char *packet, int len);
int eth_recv_packet(unsigned char *packet, int maxlen);
void eth_driver_loop();