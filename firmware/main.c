// Firmware for CEE
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include "cee.h"
#include "packetbuffer.h"

unsigned char in_seqno = 0;

int main(void){
	SetupHardware();
	sei();
	
	TCC0.CTRLA = TC_CLKSEL_DIV8_gc; // 4Mhz
	packetbuf_endpoint_init();	
	
	while (1){
		do{ 
			USB_Task();
			packetbuf_endpoint_poll();
		} while (TCC0.CNT < 500/*100*4*/);
		TCC0.CNT=0;
		
		if (packetbuf_out_can_read()){
			packetbuf_out_done_read();
		}
		
		if (packetbuf_in_can_write()){
			uint8_t* buf = packetbuf_in_write_position();
			buf[0] = in_seqno;
			buf[1] = in_count;
			buf[2] = out_count;
			packetbuf_in_done_write();
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
			USB_ep0_send(0);
			return true;
		}
	}
	
	return false;
}

