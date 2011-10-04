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

/* Configures the XMEGA's USARTC1 to talk to the digital-analog converter */ 
void configSPI(void){
	PORTC.DIRSET = (1<<3)|(1<<4)|(1<<5)|(1<<7); //LDAC, CS, SCK, TXD1 as outputs
	USARTC1.CTRLC = USART_CMODE_MSPI_gc; // SPI master, MSB first, sample on rising clock (UCPHA=0)
	USARTC1.BAUDCTRLA = 0; // 2MHz
	USARTC1.BAUDCTRLB = 0b10100000; // 2MHz continued
	USARTC1.CTRLB = USART_TXEN_bm; // enable TX
	PORTC.OUTSET = (1<<3)|(1<<4); // CS, LDAC high
}

/* Configures the board hardware and chip peripherals for the project's functionality. */
void SetupHardware(void){
	PORTE.DIRSET = (1<<0) | (1<<1);
	USB_ConfigureClock();
	USB_Init();
}

void ADC_SampleSynchronous(IN_sample* s){
	s->a_v = 444;
	s->a_i = 555;
	s->b_v = 888;
	s->b_i = 999;
}

/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		if(req->bRequest == 0xA0){
			//req->wIndex and req->wValue are input data
			ADC_SampleSynchronous((IN_sample *) ep0_buf_in);
			USB_ep0_send(sizeof(IN_sample));
			return true;
		}
	}
	
	return false;
}

