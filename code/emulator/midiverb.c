/*
 * midiverb.h - Midiverb I emulator
 * 07-31-2021 E. Brombaugh
 */

#include <string.h>
#include "midiverb.h"
#include "mv_ucode.h"

/*
 * Initialize a Midiverb entity
 */
void midiverb_Init(mvblk *blk)
{
	uint16_t i;
	
	/* init state */
	blk->prog = 255;
	blk->dfile = NULL;
	blk->acc = 0;
	blk->asum = 0;
	
	/* clear data memory */
	memset(blk->dram, 0, 16384*sizeof(int16_t));
}

/*
 * Set program
 */
void midiverb_SetProg(mvblk *blk, uint8_t prog)
{
	blk->prog = prog;
}

/*
 * process one sample
 */
void midiverb_Proc(mvblk *blk, int16_t *in, int16_t *out)
{
	uint16_t addr, instr;
	int16_t ai, sat;
	uint8_t i, op;
	
	/* don't try to execute illegal programs */
	if(blk->prog > 62)
	{
		out[0] = 0;
		out[1] = 0;
		return;
	}
	
	/* loop over microcode */
	for(i=0;i<128;i++)
	{
		/* fetch & split instruction */
		instr = mv_ucode[(blk->prog<<7) + i];
		op = (instr >> 14) & 0x3;
		addr = instr & 0x3fff;		
				
		/* Drive AI bus */
		if(i==0)
		{
			/* scale and mix input channels down to one */
			ai = ((in[0]>>4) + (in[1]>>4)) & 0xFFFE;
		}
		else
		{
			switch(op)
			{
				case 0:
				case 1:
					/* read DRAM */
					ai = blk->dram[blk->asum];
					break;
				
				case 2:
					/* read accumulator */
					ai = blk->acc;
					break;
				
					/* read inverted accumulator */
				case 3:
					ai = ~blk->acc;
					break;
			}
		}
		
		/* grab outputs */
		if((i==0x60)||(i==0x70))
		{
			/* saturate */
			sat = ai;
			if(sat > 4095)
				sat = 4095;
			else if(sat < -4096)
				sat = -4096;
		
			/* scale and route to proper channel */
			out[(i==0x60) ? 1 : 0] = sat << 3;
		}
		
		/* diagnositcs */
		if(blk->dfile)
		{
			fprintf(blk->dfile, "%02x ", i);
			fprintf(blk->dfile, "%1x ", op);
			fprintf(blk->dfile, "%04x ", addr);
			fprintf(blk->dfile, "%04x ", blk->asum);
			fprintf(blk->dfile, "%04x ", ai&0xffff);
			fprintf(blk->dfile, "%04x ", blk->acc&0xffff);
			fprintf(blk->dfile, "\n");
		}
		
		/* DRAM write */
		if((op&2) || (i==0))
			blk->dram[blk->asum] = ai;
		
		/* update Accumulator */
		if((i!=0x60) && (i!=0x70))
			blk->acc = (ai>>1) + ((op & 1) ? 0 : blk->acc) + ((ai < 0) ? 1 : 0);
				
		/* update address */
		blk->asum = (blk->asum + addr)&0x3fff;
	}
}

