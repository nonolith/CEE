#pragma once

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <delay.h>

#include "Descriptors.h"

#include "usb.h"

#define F_CPU 32000000UL
#define DISABLED 0
#define SVMI 1                                                                                                                                                           
#define SIMV 2
#define A 0
#define B 1

/* Function Prototypes: */
void configHardware(void);
void initDAC(void);
void writeDAC(uint8_t flags, uint16_t value);
void initADC(void);
void readADC(void);
void configChannels(void);
void writeChannelA(uint8_t state);
void writeChannelB(uint8_t state);


bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req);

typedef struct IN_sample{
	unsigned a_v:12;
	unsigned a_i:12;
	unsigned b_v:12;
	unsigned b_i:12;
} __attribute__((packed)) IN_sample;

typedef struct IN_packet{
	unsigned char seqno;
	unsigned char flags;
	unsigned char reserved[2];
	IN_sample data[10];	
} __attribute__((packed)) IN_packet;

typedef struct OUT_sample{
	unsigned a:12;
	unsigned b:12;
} __attribute__((packed)) OUT_sample;

typedef struct OUT_packet{
	unsigned char seqno;
	unsigned char flags;
	OUT_sample data[10];
} __attribute__((packed)) OUT_packet;

// MCP4922 flags
#define DACFLAG_CHANNEL	(1<<3)
#define DACFLAG_BUF		(1<<2)
#define DACFLAG_NO_MULT_REF	(1<<1)
#define DACFLAG_ENABLE	(1<<0)
