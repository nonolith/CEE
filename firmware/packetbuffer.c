// Bufferred BULK streaming
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include "packetbuffer.h"

uint8_t in_buf[PACKETS_BUFFER][IN_PACKET_SIZE] ATTR_ALIGNED(2);
uint8_t out_buf[PACKETS_BUFFER][OUT_PACKET_SIZE] ATTR_ALIGNED(2);

uint8_t in_start_index=0;
uint8_t in_end_index=0;
uint8_t in_count=0;

uint8_t out_start_index=0;
uint8_t out_end_index=0;
uint8_t out_count=0;

void packetbuf_endpoint_init(void){
	in_start_index = in_end_index = in_count = 0;
	USB_ep_in_init(1, USB_EP_TYPE_BULK_gc, IN_PACKET_SIZE);

	out_start_index = out_end_index = out_count = 0;
	USB_ep_out_init(2, USB_EP_TYPE_BULK_gc, OUT_PACKET_SIZE);
	USB_ep_out_start(2, out_buf[0]);
}


static void packetbuf_in_next_packet(void){
	if (USB_ep_in_sent(1) && packetbuf_in_can_read()){
		USB_ep_in_start(1, in_buf[in_start_index], IN_PACKET_SIZE);
		in_start_index = (in_start_index+1)%PACKETS_BUFFER;
		in_count--;
	}
}

static void packetbuf_out_next_packet(void){
	if (USB_ep_out_received(2) && packetbuf_out_can_write()){
		USB_ep_out_start(2, out_buf[out_end_index]);
		out_end_index = (out_end_index+1)%PACKETS_BUFFER;
		out_count++;
	}
}

void packetbuf_endpoint_poll(void){
	packetbuf_in_next_packet();
	packetbuf_out_next_packet();
}