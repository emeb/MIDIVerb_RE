/*
 * midiverb.h - Midiverb I emulator
 * 07-31-2021 E. Brombaugh
 */

#ifndef __midiverb__
#define __midiverb__

#include <stdio.h>
#include <stdint.h>

typedef struct
{
	uint8_t prog;					/* program index */
	FILE *dfile;						/* diagnostic file */
	int16_t acc;		 			/* accumulator */
	uint16_t asum;					/* Address Gen */
	int16_t dram[16384];			/* DRAM data store */
} mvblk;

void midiverb_Init(mvblk *blk);
void midiverb_SetProg(mvblk *blk, uint8_t prog);
void midiverb_Proc(mvblk *blk, int16_t *in, int16_t *out);

#endif
