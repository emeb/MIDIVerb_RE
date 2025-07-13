/*
 * wav_ops.h - .WAV routines
 * 05-21-2020 E. Brombaugh
 */

#include "wav_ops.h"
#include <string.h>

void wav_write_hdr(wav_hdr *wh, uint32_t smpls, uint8_t chls, uint8_t bits,
				   uint32_t rate)
{
	/* RIFF header */
	wh->riff[0] = 'R';
	wh->riff[1] = 'I';
	wh->riff[2] = 'F';
	wh->riff[3] = 'F';

	/* total file size */
	wh->fsz = 44 + smpls * chls * (bits / 8);

	/* WAVE format */
	wh->wave[0] = 'W';
	wh->wave[1] = 'A';
	wh->wave[2] = 'V';
	wh->wave[3] = 'E';

	/* fmt header */
	wh->fmt[0] = 'f';
	wh->fmt[1] = 'm';
	wh->fmt[2] = 't';
	wh->fmt[3] = ' ';

	/* fmt chunk size */
	wh->fmt_sz = 16;

	/* PCM type & related info */
	wh->fmt_type = 1;
	wh->fmt_chls = chls;
	wh->fmt_smplrate = rate;
	wh->fmt_byterate = rate * bits * chls / 8;
	wh->fmt_bytesmpl = bits * chls / 8;
	wh->fmt_smplbits = bits;

	/* DATA chunk */
	wh->data[0] = 'd';
	wh->data[1] = 'a';
	wh->data[2] = 't';
	wh->data[3] = 'a';

	wh->data_sz = smpls * chls * (bits / 8);
}

/*
 * check header has expected format
 */
uint8_t wav_check_hdr(wav_hdr *wh, uint8_t chls, uint8_t bits)
{
	if(strncmp(wh->riff, "RIFF", 4))
		return 1;
	if(strncmp(wh->wave, "WAVE", 4))
		return 1;
	if(strncmp(wh->fmt, "fmt ", 4))
		return 1;
	if(wh->fmt_type != 1)
		return 1;
	if(wh->fmt_chls != chls)
		return 1;
	if(wh->fmt_smplbits != bits)
		return 1;

	return 0;
}
