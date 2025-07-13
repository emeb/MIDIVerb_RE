/*
 * wav_ops.h - .WAV routines
 * 05-21-2020 E. Brombaugh
 */

#ifndef __wav_ops__
#define __wav_ops__

#include <stdint.h>

typedef struct
{
	char riff[4];
	uint32_t fsz;
	char wave[4];
	char fmt[4];
	uint32_t fmt_sz;
	uint16_t fmt_type;
	uint16_t fmt_chls;
	uint32_t fmt_smplrate;
	uint32_t fmt_byterate;
	uint16_t fmt_bytesmpl;
	uint16_t fmt_smplbits;
	char data[4];
	uint32_t data_sz;
} wav_hdr;

void wav_write_hdr(wav_hdr *wh, uint32_t smpls, uint8_t chls, uint8_t bits,
				   uint32_t rate);
uint8_t wav_check_hdr(wav_hdr *wh, uint8_t chls, uint8_t bits);

#endif