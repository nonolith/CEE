#pragma once
#define F_CPU 32000000UL

// includes
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Descriptors.h"
#include "usb.h"

// generic defines
#define DISABLED 0
#define SVMI 1                                                                                                                                                           
#define SIMV 2
#define A 0
#define B 1

// type definitions
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

// function prototypes
void configHardware(void);
void initDAC(void);
void writeDAC(uint8_t flags, uint16_t value);
void initADC(void);
void configChannels(void);
void writeChannelA(uint8_t state);
void writeChannelB(uint8_t state);
void readADC(IN_sample* s);
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req);

