// Firmware for CEE
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
// (C) 2011 Ian Daniher (Nonolith Labs) <ian@nonolithlabs.com>
// Licensed under the terms of the GNU GPLv3+

#include "cee.h"
#include "packetbuffer.h"

unsigned char in_seqno = 0;

int main(void){
	configHardware();
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

/* Configures the XMEGA's USARTC1 to talk to the digital-analog converter. */ 
void initDAC(void){
	PORTD.DIRSET = 1 << 2; // DAC-SHDN as outputs
	PORTD.OUTSET = 1 << 2; // DAC-SHDN high
	PORTC.DIRSET = 1 << 3 | 1 << 4 | 1 << 5 | 1 << 7; //LDAC, CS, SCK, TXD1 as outputs
	USARTC1.CTRLC = USART_CMODE_MSPI_gc; // SPI master, MSB first, sample on rising clock (UCPHA=0)
	USARTC1.BAUDCTRLA = 15;  // 1MHz SPI clock. XMEGA AU manual 23.15.6 & 23.3.1
	USARTC1.BAUDCTRLB =  0;
	USARTC1.CTRLB = USART_TXEN_bm; // enable TX
	PORTC.OUTSET = 1 << 3 | 1 << 4; // LDAC, CS high
	PORTC.OUTCLR = 1 << 5; // SCK low
}

/* Call me at beginning to set pin conditions for binary state pins */
void initChannels(void){
	PORTD.DIRSET = 1 << 5 | 1 << 3 | 1 << 1 | 1 << 0; // SHDN-INS-A, SWMODE-A, SHDN-INS-B, EN-OPA-A as outputs
	PORTC.DIRSET = 1 << 2 | 1 << 0; // SWMODE-B, EN-OPA-B as outputs
	PORTD.DIRCLR = 1 << 4; // TFLAG-A as input
	PORTC.DIRCLR = 1 << 1; // TFLAG-B as input
	PORTB.DIRSET = 1 << 3; // Iset output low
}

void configChannelA(uint8_t state){
	switch (state) {
		case SVMI:
			PORTD.OUTSET = 1 << 3 | 1 << 0; // SWMODE-A, EN-OPA-A high
			PORTD.OUTCLR = 1 << 5; // SHDN-INS-A low
			break;
		case SIMV:
			PORTD.OUTSET = 1 << 0; // EN-OPA-A high
			PORTD.OUTCLR = 1 << 5 | 1 << 3; // SWMODE-A, SHND-INS-A low
			break;
		case DISABLED:
			PORTD.OUTSET = 1 << 3; // SHDN-INS-A high
			PORTD.OUTCLR = 1 << 5 | 1 << 3; // SWMODE-A, EN-OPA-A low
			break;
	}
}

void configChannelB(uint8_t state){
	switch (state) {
		case SVMI:
			PORTC.OUTSET = 1 << 2 | 1 << 0; // SWMODE-B, EN-OPA-B high
			PORTD.OUTCLR = 1 << 1; // SHDN-INS-B low
			break;
		case SIMV:
			PORTC.OUTSET = 1 << 0; // EN-OPA-B high
			PORTC.OUTCLR = 1 << 2; // SWMODE-B low
			PORTD.OUTCLR = 1 << 1; // SHDN-INS-B low
			break;
		case DISABLED:
			PORTC.OUTCLR = 1 << 2 | 1 << 0; // SWMODE-B, EN-OPA-B low
			PORTD.OUTSET = 1 << 1; // SHDN-INS-B high
			break;
		}
}

/* Write a value to a specified channel of the ADC with specified flags. */
void writeDAC(uint8_t flags, uint16_t value){
	PORTC.OUTCLR = 1 << 4; // CS low
	USARTC1.DATA = ((flags<<4) & 0xF0) | ((value >> 8) & 0x0F); // munge channel, flags, and four MSB of the value into a single byte
	while(!(USARTC1.STATUS & USART_DREIF_bm)); // wait until we can write another byte
	USARTC1.STATUS = USART_TXCIF_bm; // clear TX complete flag
	USARTC1.DATA = value & 0xFF;
	while(!(USARTC1.STATUS & USART_TXCIF_bm)); // wait for TX complete flag
	PORTC.OUTSET = 1 << 4; // CS high
	PORTC.OUTCLR = 1 << 3; // LDAC low
	PORTC.OUTSET = 1 << 3; // LDAC high
}

void writeChannel(uint8_t channel, uint8_t state, uint16_t value){
	uint8_t dacflags = 0;
	if (channel) dacflags |= DACFLAG_CHANNEL;
	if (state != DISABLED) dacflags |= DACFLAG_ENABLE;
	if (state == SIMV) dacflags |= DACFLAG_NO_MULT_REF;

	if (channel == 0) configChannelA(state);
	else              configChannelB(state);

	writeDAC(dacflags, value);
}

/* Configures the board hardware and chip peripherals for the project's functionality. */
void configHardware(void){
	PORTE.DIRSET = 1 << 0 | 1 << 1; //debug LEDs
	initDAC();
	initChannels();
	USB_ConfigureClock();
	USB_Init();
}

void ADC_SampleSynchronous(IN_sample* s, uint8_t a, uint8_t b){
	s->a_v = a;
	s->a_i = a;
	s->b_v = b;
	s->b_i = b;
}


/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		switch(req->bRequest){
			case 0xA0:
				ADC_SampleSynchronous((IN_sample *) ep0_buf_in, req->wIndex, req->wValue);
				USB_ep0_send(sizeof(IN_sample));
				break;
			case 0xB0:
				writeDAC(req->wIndex, req->wValue);
				ep0_buf_in[0] = USARTC1.STATUS;
				USB_ep0_send(1);
				break;
			case 0xCB:
				configChannelB(req->wValue);
				USB_ep0_send(0);
				break;
			case 0xCA:
				configChannelA(req->wValue);
				USB_ep0_send(0);
				break;
			case 0xAA:
				writeChannel(0, req->wIndex, req->wValue);
				USB_ep0_send(0);
				break;
			case 0xAB:
				writeChannel(1, req->wIndex, req->wValue);
				USB_ep0_send(0);
				break;
		}
		return true;
	}
	return false;
}

