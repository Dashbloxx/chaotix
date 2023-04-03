/*
 *    __  __                      
 *   |  \/  |__ _ __ _ _ __  __ _ 
 *   | |\/| / _` / _` | '  \/ _` |
 *   |_|  |_\__,_\__, |_|_|_\__,_|
 *               |___/        
 * 
 *  Magma is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss, John Paul Wohlscheid, rilysh, Milton612, and FueledByCocaine
 * 
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#pragma once

/*
 *  This file defines some basic structures that are related to networking. When talking about networking here, we're talking about
 *  networking between different computers, not UNIX sockets...
 */

/* IP protocol number for TCP! */
#define IP_PROTOCOL_TCP 6

/* This structure defines the IP header format... */
struct ip_header {
    unsigned char version;
    unsigned char ihl;
    unsigned short total_len;
    unsigned short id;
    unsigned short flags;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned int src_ip;
    unsigned int dest_ip;
};

/* This structure defines the TCP header format... */
struct tcp_header {
    unsigned short src_port;
    unsigned short dest_port;
    unsigned int seq_num;
    unsigned int ack_num;
    unsigned char data_offset;
    unsigned char flags;
    unsigned short window_size;
    unsigned short checksum;
    unsigned short urgent_pointer;
};

void net_init();
void net_send_packet(unsigned int dest_ip, unsigned char protocol, unsigned char *data, int len);
void net_recv_packet(unsigned char *packet, int len);