// Bufferred BULK streaming
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include "packetbuffer.h"

unsigned char in_buf[PACKETS_BUFFER][IN_PACKET_SIZE] ATTR_ALIGNED(2);
unsigned char out_buf[PACKETS_BUFFER][OUT_PACKET_SIZE] ATTR_ALIGNED(2);

unsigned char in_start_index=0;
unsigned char in_end_index=0;
unsigned char in_count=0;

unsigned char out_start_index=0;
unsigned char out_end_index=0;
unsigned char out_count=0;

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


static void in_ringbuf_next_packet(void){
	if ((endpoints[1].in.STATUS & USB_EP_TRNCOMPL0_bm) && in_ringbuf_can_read()){
		endpoints[1].in.DATAPTR = (unsigned) in_buf[in_start_index];
		endpoints[1].in.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
		in_start_index = (in_start_index+1)%PACKETS_BUFFER;
		in_count--;
	}
}

static void out_ringbuf_next_packet(void){
	if ((endpoints[2].out.STATUS & USB_EP_TRNCOMPL0_bm) && out_ringbuf_can_write()){
		endpoints[2].out.DATAPTR = (unsigned) out_buf[out_end_index];
		endpoints[2].out.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
		out_end_index = (out_end_index+1)%PACKETS_BUFFER;
		out_count++;
	}
}

void packetbuf_endpoint_poll(void){
	in_ringbuf_next_packet();
	out_ringbuf_next_packet();
}