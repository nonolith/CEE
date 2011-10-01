// Bufferred BULK streaming
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include <usb.h>

#define IN_PACKET_SIZE 64
#define OUT_PACKET_SIZE 32
#define PACKETS_BUFFER 32

extern unsigned char in_buf[PACKETS_BUFFER][IN_PACKET_SIZE];
extern unsigned char out_buf[PACKETS_BUFFER][OUT_PACKET_SIZE];

extern unsigned char in_start_index;
extern unsigned char in_end_index;
extern unsigned char in_count;

extern unsigned char out_start_index;
extern unsigned char out_end_index;
extern unsigned char out_count;

void packetbuf_endpoint_init(void);
void packetbuf_endpoint_poll(void);

inline bool in_ringbuf_can_read(void){return in_count>0;}
inline bool in_ringbuf_can_write(void){return in_count<PACKETS_BUFFER;}

inline bool out_ringbuf_can_read(void){return out_count>0;}
inline bool out_ringbuf_can_write(void){return out_count<PACKETS_BUFFER;}

inline void out_ringbuf_done_read(void){
	out_count--;
	out_start_index = (out_start_index+1)%PACKETS_BUFFER;
}

inline void in_ringbuf_done_write(void){
	in_count++;
	in_end_index = (in_end_index+1)%PACKETS_BUFFER;
}

inline unsigned char* in_ringbuf_write_position(void){return in_buf[in_end_index];}
inline unsigned char* out_ringbuf_read_position(void){return out_buf[out_start_index];}
