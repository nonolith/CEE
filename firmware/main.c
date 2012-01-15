// Firmware for CEE
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
// (C) 2011 Ian Daniher (Nonolith Labs) <ian@nonolithlabs.com>
// Licensed under the terms of the GNU GPLv3+

#include "cee.h"
#include "packetbuffer.h"
#include "hardware.h"
#include "dac.c"

unsigned char in_seqno = 0;

int main(void){
	configHardware();
	
	PMIC.CTRL = PMIC_LOLVLEN_bm;
	sei();	
	
	for (;;){
		USB_Task(); // Lower-priority USB polling, like control requests
		packetbuf_endpoint_poll();
	}
}

/* Read the voltage and current from the two channels, pulling the latest samples off "ADCA.CHx.RES" registers. */
void readADC(IN_sample* const s){
	uint8_t A_Il = ADCA.CH0.RESL, A_Ih = ADCA.CH0.RESH;
	uint8_t A_Vl = ADCA.CH1.RESL, A_Vh = ADCA.CH1.RESH;

	s->avl = A_Vl;
	s->ail = A_Il;
	s->aih_avh = (A_Ih << 4) | (A_Vh&0x0f);

	uint8_t B_Vl = ADCA.CH2.RESL, B_Vh = ADCA.CH2.RESH;
	uint8_t B_Il = ADCA.CH3.RESL, B_Ih = ADCA.CH3.RESH;

	s->bvl = B_Vl;
	s->bil = B_Il;
	s->bih_bvh = (B_Ih << 4) | (B_Vh&0x0f);
}


uint8_t sampleIndex = 0; // Sample index within packet to be written next
bool havePacket = 0;
IN_packet *inPacket;
OUT_packet *outPacket;

void configureSampling(uint16_t mode, uint16_t period){
	TCC0.INTCTRLA = TC_OVFINTLVL_OFF_gc;
	TCC0.CTRLA = 0;
	sampleIndex = 0;
	havePacket = 0;
	
	if (mode == 1 && period > 80){
		packetbuf_endpoint_init(); // clear buffers
		TCC0.CTRLA = TC_CLKSEL_DIV8_gc; // 4Mhz
		TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;
		TCC0.PER = period;
		TCC0.CNT = 0;
	}else{
		configChannelA(DISABLED);
		configChannelB(DISABLED);
		PORTR.OUTCLR = 1 << 1;
	}
}

ISR(TCC0_OVF_vect){
	if (!havePacket){
		if (packetbuf_in_can_write() && packetbuf_out_can_read()){
			PORTR.OUTSET = 1 << 1;
			havePacket = 1;
			inPacket = (IN_packet *) packetbuf_in_write_position();
			outPacket = (OUT_packet *) packetbuf_out_read_position();
			DAC_config(outPacket->flags);
			sampleIndex = 0;
		}else{
			PORTR.OUTCLR = 1 << 1;
			return;
		}
	}
	
	DAC_startWrite(&(outPacket->data[sampleIndex]));	 // start SPI write
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


void configISET(void){
	DACB.CTRLA |= DAC_CH0EN_bm | DAC_CH1EN_bm | DAC_ENABLE_bm;
	DACB.CTRLB |= DAC_CHSEL_DUAL_gc; 
	DACB.CTRLC |= DAC_REFSEL_AREFA_gc; //2.5VREF
	DACB.CH1DATA = 0x6B7; // 0x6B7/0xFFF*2.5V = 1.05V, 9800*(1.18V-1.05)/560O = 0.227
	DACB.CH0DATA = 0x6B7; // 0x6B7/0xFFF*2.5V = 1.05V, 9800*(1.18V-1.05)/560O = 0.227
}

/* Configure the pin modes for the switches and opamps. */
void initChannels(void){
	PORTD.DIRSET = SHDN_INS_A | SWMODE_A | SHDN_INS_B | EN_OPA_A;
	PORTC.DIRSET = SWMODE_B | EN_OPA_B;
	PORTD.DIRCLR = TFLAG_A;
	PORTC.DIRCLR = TFLAG_B;
	PORTB.DIRSET = ISET_A | ISET_B;
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



/* Take a channel, state, and value and configure the switches, shutdowns, and DACs. High level abstraction. */
void writeChannel(uint8_t channel, uint8_t state, uint16_t value){
	uint8_t dacflags = 0;
	if (channel) dacflags |= DACFLAG_CHANNEL;
	if (state != DISABLED) dacflags |= DACFLAG_ENABLE;
	if (state == SIMV) dacflags |= DACFLAG_NO_MULT_REF;

	if (channel == 0) configChannelA(state);
	else              configChannelB(state);

	DAC_write(dacflags, value);
}

/* Configures the board hardware and chip peripherals for the project's functionality. */
void configHardware(void){
	USB_ConfigureClock();
	PORTR.DIRSET = 1 << 1;
	PORTR.OUTSET = 1 << 1;
	_delay_ms(50);
	PORTR.OUTCLR = 1 << 1;
	DAC_init();
	initADC();
	initChannels();
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

#define xstringify(s) stringify(s)
#define stringify(s) #s

const char PROGMEM hwversion[] = xstringify(HW_VERSION);
const char PROGMEM fwversion[] = xstringify(FW_VERSION);

/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		switch(req->bRequest){
			case 0x00: // Info
				if (req->wIndex == 0){
					USB_ep0_send_progmem((uint8_t*)hwversion, sizeof(hwversion));
				}else if (req->wIndex == 1){
					USB_ep0_send_progmem((uint8_t*)fwversion, sizeof(fwversion));
				}
				break;
			case 0xA0: // read ADC
				readADC((IN_sample *) ep0_buf_in);
				USB_ep0_send(sizeof(IN_sample));
				break;
			case 0xAA: // write to channel A
				writeChannel(0, req->wIndex, req->wValue);
				USB_ep0_send(0);
				break;
			case 0xAB: // write to channel B
				writeChannel(1, req->wIndex, req->wValue);
				USB_ep0_send(0);
				break;
			case 0x65: // set gains
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
			case 0x15: // ISet
				switch (req->wIndex){
					case 0x0A:
			    		DACB.CH0DATA = req->wValue;
						break;
					case 0x0B:
						DACB.CH1DATA = req->wValue;
						break;
				}
				USB_ep0_send(0);
				break;
				
			case 0x80: // Configure sampling	
				configureSampling(req->wIndex /*mode*/ , req->wValue /*period*/);
				USB_ep0_send(0);
				break;
			
			case 0xBB: // disconnect from USB, jump to bootloader
				cli();
				PMIC.CTRL = 0;
				USB_ep0_send(0);
				USB_ep0_wait_for_complete();
				_delay_us(10000);
				USB_Detach();
				void (*enter_bootloader)(void) = (void *) 0x47fc /*0x8ff8/2*/;
				enter_bootloader();
				break;
		}
		return true;
	}
	return false;
}

