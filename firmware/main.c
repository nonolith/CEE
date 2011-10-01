// Demo USB device for ATxmega32a4u
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include "cee.h"

#define IN_PACKET_SIZE 64
#define OUT_PACKET_SIZE 32
#define PACKETS_BUFFER 32

unsigned char in_buf[PACKETS_BUFFER][IN_PACKET_SIZE] ATTR_ALIGNED(2);
unsigned char out_buf[PACKETS_BUFFER][OUT_PACKET_SIZE] ATTR_ALIGNED(2);

unsigned char in_start_index=0;
unsigned char in_end_index=0;
unsigned char in_count=0;

unsigned char out_start_index=0;
unsigned char out_end_index=0;
unsigned char out_count=0;



bool in_ringbuf_can_read(void){return in_count>0;}
bool in_ringbuf_can_write(void){return in_count<PACKETS_BUFFER;}

bool out_ringbuf_can_read(void){return out_count>0;}
bool out_ringbuf_can_write(void){return out_count<PACKETS_BUFFER;}

unsigned char in_seqno = 0;
unsigned char pad; // Watch out, GCC ignores aligned!


void configureEndpoint(void){
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

inline void in_ringbuf_next_packet(void){
	if (in_ringbuf_can_read() && (endpoints[1].in.STATUS & USB_EP_TRNCOMPL0_bm)){
		PORTE.OUTSET = (1<<0);
		endpoints[1].in.DATAPTR = (unsigned) in_buf[in_start_index];
		endpoints[1].in.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
		in_start_index = (in_start_index+1)%PACKETS_BUFFER;
		in_count--;
	}
}

inline void out_ringbuf_next_packet(void){
	if (out_ringbuf_can_write() && endpoints[2].out.STATUS & USB_EP_TRNCOMPL0_bm){
		endpoints[2].out.DATAPTR = (unsigned) out_buf[out_end_index];
		endpoints[2].out.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
		out_end_index = (out_end_index+1)%PACKETS_BUFFER;
		out_count++;
	}
}

inline void out_ringbuf_done_read(void){
	out_count--;
	out_start_index = (out_start_index+1)%PACKETS_BUFFER;
}

inline void in_ringbuf_done_write(void){
	in_count++;
	in_end_index = (in_end_index+1)%PACKETS_BUFFER;
}

void pollEndpoint(void){
	in_ringbuf_next_packet();
	out_ringbuf_next_packet();
}

int main(void){
	SetupHardware();
	sei();
	
	TCC0.CTRLA = TC_CLKSEL_DIV8_gc; // 4Mhz
	configureEndpoint();	
	
	while (1){
		do{ 
			USB_Task();
			pollEndpoint();
		} while (TCC0.CNT < 500/*100*4*2*/);
		TCC0.CNT=0;
		
		if (out_ringbuf_can_read()){
			out_ringbuf_done_read();
		}
		
		if (in_ringbuf_can_write()){
			in_buf[in_end_index][0] = in_seqno;
			in_buf[in_end_index][1] = in_count;
			in_buf[in_end_index][2] = out_count;
			in_ringbuf_done_write();
		}
		in_seqno++;
	}
}

/** Configures the board hardware and chip peripherals for the project's functionality. */
void SetupHardware(void){
	PORTE.DIRSET = (1<<0) | (1<<1);
	
	USB_ConfigureClock();
	USB_Init();
}

/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		if (req->bRequest == 0x23){
			//timer = req->wValue;
			USB_ep_send_packet(0, 0);
			return true;
		}
	}
	
	return false;
}

