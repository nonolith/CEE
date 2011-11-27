// Firmware for CEE
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
// (C) 2011 Ian Daniher (Nonolith Labs) <ian@nonolithlabs.com>
// Licensed under the terms of the GNU GPLv3+

#include "cee.h"
#include "packetbuffer.h"
#include "hardware.h"

unsigned char in_seqno = 0;
volatile uint8_t indexDataDAC = 0;

int main(void){
	configHardware();
	packetbuf_endpoint_init();
	
	PMIC.CTRL = PMIC_LOLVLEN_bm;
	sei();	
	
	TCC0.CTRLA = TC_CLKSEL_DIV8_gc; // 4Mhz
	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;
	TCC0.PER = 160;
	TCC0.CNT = 0;
	
	for (;;){
		USB_Task(); // Lower-priority USB polling, like control requests
		packetbuf_endpoint_poll();
	}
}

uint8_t dataDAC[4];

#define MODE_TO_DACFLAGS(m)  ((((m)!=DISABLED)?DACFLAG_ENABLE:0) \
                            |(((m)==SIMV)?DACFLAG_NO_MULT_REF:0))
	
inline void dac_config(OUT_flags flags){
	dataDAC[0] = MODE_TO_DACFLAGS(flags.a_mode) << 4;
	dataDAC[2] = (DACFLAG_CHANNEL | MODE_TO_DACFLAGS(flags.b_mode)) << 4;
}

inline void startWriteDAC(OUT_sample* s){	
	dataDAC[0] = (dataDAC[0] & 0xF0) | ((s->a >> 8) & 0x0F); // go from outSample->a to high byte of SPI transfer to DAC
	dataDAC[1] = s->a & 0xff;
	dataDAC[2] = (dataDAC[2] & 0xF0) | ((s->b >> 8) & 0x0F);
	dataDAC[3] = s->b & 0xff;
	indexDataDAC = 0; // reset index
	PORTC.OUTCLR = CS; // CS low, start transfer
	USARTC1.DATA =  dataDAC[indexDataDAC++]; // write byte 0
	USARTC1.CTRLA = USART_DREINTLVL_LO_gc; // enable DRE
}

/* Read the voltage and current from the two channels, pulling the latest samples off "ADCA.CHx.RES" registers. */
void readADC(IN_sample* s){
	s->a_i = ADCA.CH0.RES; // VS-A
	s->a_v = ADCA.CH1.RES; // ADC-A
	s->b_v = ADCA.CH2.RES; // ADC-B
	s->b_i = ADCA.CH3.RES; // VS-B
}



ISR(TCC0_OVF_vect){
	
	static uint8_t sampleIndex = 0; // Sample index within packet to be written next
	static bool havePacket = 0;
	static IN_packet *inPacket;
	static OUT_packet *outPacket;
	
	if (!havePacket){
		if (packetbuf_in_can_write() && packetbuf_out_can_read()){
			havePacket = 1;
			inPacket = (IN_packet *) packetbuf_in_write_position();
			outPacket = (OUT_packet *) packetbuf_out_read_position();
			dac_config(outPacket->flags);
			sampleIndex = 0;
		}else{
			return;
		}
	}
	
	startWriteDAC(&(outPacket->data[sampleIndex]));	 // start SPI write
	readADC(&(inPacket->data[sampleIndex]));

	PORTC.OUTSET = LDAC; // toggle LDAC
	PORTC.OUTCLR = LDAC;
	
	uint8_t i = sampleIndex++;
	
	if (i == 1){
		// Just latched the 0th sample from this packet to the DAC
		// Apply the mode for this packet
		configChannelA(outPacket->flags.a_mode);
		configChannelB(outPacket->flags.b_mode);
	} else if (i == 5){
		// fill header when there's nothing else going on
		inPacket->seqno = in_seqno++;
		inPacket->reserved[0] = in_count;
		inPacket->reserved[1] = out_count;
	} else if (i >= 9){
		sampleIndex = 0;
		havePacket = 0;
		packetbuf_in_done_write();
		packetbuf_out_done_read();
	}

}

ISR(USARTC1_DRE_vect){
	USARTC1.DATA =  dataDAC[indexDataDAC++]; // write byte 1 or 3
	USARTC1.CTRLA = USART_TXCINTLVL_LO_gc | USART_DREINTLVL_OFF_gc; // enable TXC, disable DRE
	USARTC1.STATUS = USART_TXCIF_bm; // clear TXC
}

ISR(USARTC1_TXC_vect){
	PORTC.OUTSET = CS; // CS high
	if (indexDataDAC == 2){
		PORTC.OUTCLR = CS; // CS low
		USARTC1.CTRLA = USART_DREINTLVL_LO_gc | USART_TXCINTLVL_OFF_gc; // enable DRE, disable TXC
		USARTC1.DATA =  dataDAC[indexDataDAC++]; // write byte 2, increment counter
	}else{
		USARTC1.CTRLA = USART_TXCINTLVL_OFF_gc; // disable TXC
	}
}

/* Configures the XMEGA's USARTC1 to talk to the digital-analog converter. */ 
void initDAC(void){
	PORTD.DIRSET = DAC_SHDN;
	PORTD.OUTSET = DAC_SHDN; 
	PORTC.DIRSET = LDAC | CS | SCK | TXD1;
	USARTC1.CTRLC = USART_CMODE_MSPI_gc; // SPI master, MSB first, sample on rising clock (UCPHA=0)
	USARTC1.BAUDCTRLA =  0;  // 16MHz SPI clock. XMEGA AU manual 23.15.6 & 23.3.1
	USARTC1.BAUDCTRLB =  0;
	USARTC1.CTRLB = USART_TXEN_bm; // enable TX
	PORTC.OUTSET = CS;
	PORTC.OUTCLR = LDAC | CS; // LDAC, SCK low
}

void configISET(void){
	DACB.CTRLA |= DAC_CH1EN_bm | DAC_ENABLE_bm;
	DACB.CTRLB |= DAC_CHSEL1_bm;
	DACB.CTRLC |= DAC_REFSEL_AREFA_gc; //2.5VREF
	DACB.CH1DATA = 0x6B7; // 0x6B7/0xFFF*2.5V = 1.05V, 9800*(1.18V-1.05)/560O = 0.227
}

/* Configure the pin modes for the switches and opamps. */
void initChannels(void){
	PORTD.DIRSET = SHDN_INS_A | SWMODE_A | SHDN_INS_B | EN_OPA_A;
	PORTC.DIRSET = SWMODE_B | EN_OPA_B;
	PORTD.DIRCLR = TFLAG_A;
	PORTC.DIRCLR = TFLAG_B;
	PORTB.DIRSET = ISET;
	PORTD.OUTCLR = EN_OPA_A;
	PORTC.OUTCLR = EN_OPA_B;
	configISET();
}

/* Configure the shutdown/enable pin states and set the SPDT switch states. */
void configChannelA(chan_mode state){
	switch (state) {
		case SVMI:
			PORTD.OUTSET = SWMODE_A | EN_OPA_A;
			PORTD.OUTCLR = SHDN_INS_A;
			break;
		case SIMV:
			PORTD.OUTSET = EN_OPA_A;
			PORTD.OUTCLR = SWMODE_A | SHDN_INS_A;
			break;
		case DISABLED:
			PORTD.OUTSET = SHDN_INS_A;
			PORTD.OUTCLR = SWMODE_A | EN_OPA_A;
			break;
	}
}

void configChannelB(chan_mode state){
	switch (state) {
		case SVMI:
			PORTC.OUTSET = SWMODE_B | EN_OPA_B;
			PORTD.OUTCLR = SHDN_INS_B;
			break;
		case SIMV:
			PORTC.OUTSET = EN_OPA_B;
			PORTC.OUTCLR = SWMODE_B;
			PORTD.OUTCLR = SHDN_INS_B;
			break;
		case DISABLED:
			PORTC.OUTCLR = SWMODE_B | EN_OPA_B;
			PORTD.OUTSET = SHDN_INS_B;
			break;
		}
}

/* Write a value to a specified channel of the DAC with specified flags. */
void writeDAC(uint8_t flags, uint16_t value){
	PORTC.OUTCLR = CS;
	USARTC1.DATA = ((flags<<4) & 0xF0) | ((value >> 8) & 0x0F); // munge channel, flags, and four MSB of the value into a single byte
	while(!(USARTC1.STATUS & USART_DREIF_bm)); // wait until we can write another byte
	USARTC1.STATUS = USART_TXCIF_bm; // clear TX complete flag
	USARTC1.DATA = value & 0xFF;
	while(!(USARTC1.STATUS & USART_TXCIF_bm)); // wait for TX complete flag
	PORTC.OUTSET = CS;
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
	initDAC();
	initADC();
	initChannels();
	USB_ConfigureClock();
	USB_Init();
}

/* Configure the ADC to 12b, differential w/ gain, signed mode with a 2.5VREF. */
void initADC(void){
	ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc | 1 << ADC_CONMODE_bp | 0 << ADC_IMPMODE_bp | ADC_CURRLIMIT_NO_gc | ADC_FREERUN_bm;
	ADCA.REFCTRL = ADC_REFSEL_AREFA_gc; // use 2.5VREF at AREFA
	ADCA.PRESCALER = ADC_PRESCALER_DIV32_gc; // ADC CLK = 1MHz
	ADCA.EVCTRL = ADC_SWEEP_0123_gc ; 
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH1.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH2.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH3.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH0.MUXCTRL = ADC_CH_MUXNEG_PIN5_gc | ADC_CH_MUXPOS_PIN1_gc; // 1.25VREF vs VS-A
	ADCA.CH1.MUXCTRL = ADC_CH_MUXNEG_PIN4_gc |  ADC_CH_MUXPOS_PIN2_gc; // INTGND vs ADC-A
	ADCA.CH2.MUXCTRL = ADC_CH_MUXNEG_PIN4_gc | ADC_CH_MUXPOS_PIN6_gc; // INTGND vs ADC-B
	ADCA.CH3.MUXCTRL = ADC_CH_MUXNEG_PIN5_gc | ADC_CH_MUXPOS_PIN7_gc; // 1.25VREF vs VS-B
	ADCA.CTRLA = ADC_ENABLE_bm;
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
			case 0x65:
				switch (req->wIndex){
					case 0x00:
				    	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | req->wValue; // VS-A
						break;
					case 0x01:
				    	ADCA.CH1.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | req->wValue; // ADC-A
						break;
					case 0x02:
				    	ADCA.CH2.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | req->wValue; // ADC-B
						break;
					case 0x03:
				    	ADCA.CH3.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | req->wValue; // VS-B
						break;
				}
				USB_ep0_send(0);
				break;
			case 0xBB:
				cli();
				PMIC.CTRL = 0;
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

