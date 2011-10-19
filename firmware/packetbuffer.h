// Bufferred BULK streaming
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
// (C) 2011 Ian Daniher (Nonolith Labs) <ian@nonolithlabs.com>
//
// Licensed under the terms of the GNU GPLv3+

#include <usb.h>

#define IN_PACKET_SIZE 64
#define OUT_PACKET_SIZE 32
#define PACKETS_BUFFER 32

//create slave-to-host buffer
extern unsigned char in_buf[PACKETS_BUFFER][IN_PACKET_SIZE];

//create host-to-slave buffer
extern unsigned char out_buf[PACKETS_BUFFER][OUT_PACKET_SIZE];

extern unsigned char in_start_index;
extern unsigned char in_end_index;
extern unsigned char in_count;

extern unsigned char out_start_index;
extern unsigned char out_end_index;
extern unsigned char out_count;

void packetbuf_endpoint_init(void);
void packetbuf_endpoint_poll(void);

//determine if here are packets to be sent to the host
inline static bool packetbuf_in_can_read(void){return in_count>0;}

//determine if there is space in the slave-to-host ring buffer for another packet
inline static bool packetbuf_in_can_write(void){return in_count<PACKETS_BUFFER;}

//determine if there are packets received from the host
inline static bool packetbuf_out_can_read(void){return out_count>0;}

//determine if there is space in the buffer for another packet from the host
inline static bool packetbuf_out_can_write(void){return out_count<PACKETS_BUFFER;}

//queue packet from host for processing
inline void packetbuf_out_done_read(void){
	out_count--;
	out_start_index = (out_start_index+1)%PACKETS_BUFFER;
}

//queue packet to be sent from the slave to the host
inline void packetbuf_in_done_write(void){
	in_count++;
	in_end_index = (in_end_index+1)%PACKETS_BUFFER;
}
//returns location of buffer to fill
inline unsigned char* packetbuf_in_write_position(void){return in_buf[in_end_index];}
inline unsigned char* packetbuf_out_read_position(void){return out_buf[out_start_index];}
