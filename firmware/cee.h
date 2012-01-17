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

typedef enum chan_mode{
	DISABLED = 0,
	SVMI = 1,
	SIMV = 2,
} chan_mode;

#define A 0
#define B 1

#define FLAG_PACKET_DROPPED (1<<0)

void configChannelA(chan_mode state);
void configChannelB(chan_mode state);

// type definitions
typedef struct IN_sample{
	uint8_t avl, ail, aih_avh, bvl, bil, bih_bvh;
} IN_sample;

typedef struct IN_packet{
	uint8_t mode_a;
	uint8_t mode_b;
	uint8_t flags;
	uint8_t mode_seq;
	IN_sample data[10];	
} __attribute__((packed)) IN_packet;

typedef struct OUT_sample{
	uint8_t al, bl, bh_ah;
} __attribute__((packed)) OUT_sample;

typedef struct OUT_packet{
	uint8_t mode_a;
	uint8_t mode_b;
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
void configISET(void);
