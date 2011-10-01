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


//TODO: move USB code into USB lib
void packetbuf_endpoint_init(void){
	in_start_index = in_end_index = in_count = 0;
	endpoints[1].in.CNT = IN_PACKET_SIZE;
	endpoints[1].in.STATUS = USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm;
	endpoints[1].in.CTRL = USB_EP_TYPE_BULK_gc | USB_EP_size_to_gc(IN_PACKET_SIZE);
	
	out_start_index = out_end_index = out_count = 0;
	endpoints[2].out.DATAPTR = (unsigned) out_buf[0];
	endpoints[2].out.CNT = 0;
	endpoints[2].out.STATUS = USB_EP_TOGGLE_bm;
	endpoints[2].out.CTRL = USB_EP_TYPE_BULK_gc | USB_EP_size_to_gc(OUT_PACKET_SIZE);
}


static void packetbuf_in_next_packet(void){
	if ((endpoints[1].in.STATUS & USB_EP_TRNCOMPL0_bm) && packetbuf_in_can_read()){
		endpoints[1].in.DATAPTR = (unsigned) in_buf[in_start_index];
		endpoints[1].in.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
		in_start_index = (in_start_index+1)%PACKETS_BUFFER;
		in_count--;
	}
}

static void packetbuf_out_next_packet(void){
	if ((endpoints[2].out.STATUS & USB_EP_TRNCOMPL0_bm) && packetbuf_out_can_write()){
		endpoints[2].out.DATAPTR = (unsigned) out_buf[out_end_index];
		endpoints[2].out.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
		out_end_index = (out_end_index+1)%PACKETS_BUFFER;
		out_count++;
	}
}

void packetbuf_endpoint_poll(void){
	packetbuf_in_next_packet();
	packetbuf_out_next_packet();
}