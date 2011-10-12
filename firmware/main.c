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

/* Configure the pin modes for the switches and opamps. */
void initChannels(void){
	PORTD.DIRSET = 1 << 5 | 1 << 3 | 1 << 1 | 1 << 0; // SHDN-INS-A, SWMODE-A, SHDN-INS-B, EN-OPA-A as outputs
	PORTC.DIRSET = 1 << 2 | 1 << 0; // SWMODE-B, EN-OPA-B as outputs
	PORTD.DIRCLR = 1 << 4; // TFLAG-A as input
	PORTC.DIRCLR = 1 << 1; // TFLAG-B as input
	PORTB.DIRSET = 1 << 3; // Iset output low
}

/* Configure the shutdown/enable pin states and set the SPTDT switch states. */
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

/* Write a value to a specified channel of the DAC with specified flags. */
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

/* Take a channel, state, and value and configure the switches, shutdowns, and DACs. High level abstraction. */
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
	PORTE.DIRSET = 1 << 0 | 1 << 1; // debug LEDs
	initDAC();
	initADC();
	initChannels();
	USB_ConfigureClock();
	USB_Init();
}

/* Configure the ADC to 12b, single-ended, signed mode with a 2.5VREF. */
void initADC(void){
	ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc | 0 << ADC_CONMODE_bp | 0 << ADC_IMPMODE_bp | ADC_CURRLIMIT_NO_gc | ADC_FREERUN_bm;
	ADCA.REFCTRL = ADC_REFSEL_AREFA_gc; // use 2.5VREF at AREFA
	ADCA.PRESCALER = ADC_PRESCALER_DIV32_gc; // ADC CLK = 1MHz
	ADCA.EVCTRL = ADC_SWEEP_0123_gc ; 
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH1.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH2.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH3.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH0.MUXCTRL = ADC_CH_MUXNEG_PIN5_gc | ADC_CH_MUXPOS_PIN1_gc; // 1.25VREF vs VS-A
	ADCA.CH1.MUXCTRL = 0b100 |  ADC_CH_MUXPOS_PIN2_gc; // INTGND vs ADC-A
	ADCA.CH2.MUXCTRL = 0b100 | ADC_CH_MUXPOS_PIN6_gc; // INTGND vs ADC-B
	ADCA.CH3.MUXCTRL = ADC_CH_MUXNEG_PIN5_gc | ADC_CH_MUXPOS_PIN7_gc; // 1.25VREF vs VS-B
	ADCA.CTRLA = ADC_ENABLE_bm;
}

uint8_t readOffset(void){
    ADCA.CH0.MUXCTRL = 0b100 | ADC_CH_MUXPOS_PIN4_gc; // measure INTGND vs PIN4(GND)
    ADCA.CTRLA |= ADC_CH0START_bm;
    while (!ADCA.CH0.INTFLAGS);
    ADCA.INTFLAGS = ADC_CH0IF_bm;
	return (0xFF - ADCA.CH0.RESL); // return DC offset between internal and external ground
}

/* Read the voltage and current from the two channels, using while(!..) loops to wait for conversions. */
void readADC(IN_sample* s){

	uint8_t offset = 0x16;

	s->a_i = ADCA.CH0.RES; //measure CS-A, monitoring OPA-B

	s->a_v = ADCA.CH1.RES + offset;

	s->b_v = ADCA.CH2.RES + offset;

	s->b_i = ADCA.CH3.RES; // measure CS-B monitoring OPA-A
}


/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		switch(req->bRequest){
			case 0xA0:
				readADC((IN_sample *) ep0_buf_in);
				USB_ep0_send(sizeof(IN_sample));
				break;
			case 0xAA:
				writeChannel(0, req->wIndex, req->wValue);
				USB_ep0_send(0);
				break;
			case 0xAB:
				writeChannel(1, req->wIndex, req->wValue);
				USB_ep0_send(0);
				break;
			case 0xBB:
				USB_ep0_send(0);
				USB_ep0_wait_for_complete();
				_delay_us(10000);
				USB_Detach();
				_delay_us(100000);
				void (*enter_bootloader)(void) = (void *) 0x47fc /*0x8ff8/2*/;
				enter_bootloader();
				break;
		}
		return true;
	}
	return false;
}

