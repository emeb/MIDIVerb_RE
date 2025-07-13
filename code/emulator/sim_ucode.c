/* sim_ucode.c - simulate midiverb microcode on .wav audio */
/* 05-03-21 E. Brombaugh */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "wav_ops.h"

//#define TRACE

int main(int argc, char **argv)
{
	int prog = 21;
	char *uname = "midifverb.bin", *iname = "input.wav", *oname = "output.wav";
	FILE *ufile, *ifile, *ofile;
#ifdef TRACE
	FILE *tfile;
#endif
	int i, base;
	uint8_t rom[256];
	uint16_t ucode[128], instr, op, addr, asum = 0;
	int16_t dram[16384], stereo[2], adc, ai, acc, sum, sat, acc_out;
	wav_hdr wh;
	int32_t samples, scnt;
	
	/* override defaults */
	if(argc > 1)
		prog = atoi(argv[1]);
	
	if(argc > 2)
		uname = argv[2];
	
	if(argc > 3)
		iname = argv[3];
	
	if(argc > 4)
		iname = argv[4];
	
	/* load a program and analyze */
	if(!(ufile = fopen(uname, "rb")))
	{
		fprintf(stderr, "Couldn't open microcode file %s for read\n", uname);
		exit(1);
	}
	
	base = prog*256;
	fprintf(stdout, "Program %d - base address 0x%04X\n", prog, base);
	fseek(ufile, base, SEEK_SET);
	if(fread(rom, sizeof(uint8_t), 256, ufile) != 256)
	{
		fprintf(stderr, "Unexepected EOF.\n");
		fclose(ufile);
		exit(1);
	}
	fclose(ufile);
	
#ifdef TRACE
	/* open trace file */
	if(!(tfile = fopen("trace.txt", "w")))
	{
		fprintf(stderr, "Couldn't open trace file for output\n");
		exit(1);
	}
#endif
	
	/* format ROM bytes into microcode */
	for(i=0;i<128;i++)
	{
		ucode[i] = rom[(i*2-1)&0xff]<<8 | rom[(i*2-2)&0xff];
	}
	
	/* init DRAM */
	for(i=0;i<16384;i++)
	{
		dram[i] = 0;
	}
	
	/* open input wav file */
	if(!(ifile = fopen(iname, "rb")))
	{
		fprintf(stderr, "Couldn't open input file %s for read\n", iname);
		exit(1);
	}
	
	/* get WAV header & check if it's valid */
	if(fread(&wh, sizeof(wav_hdr), 1, ifile) != 1)
	{
		fprintf(stderr, "Unexepected EOF in input file.\n");
		fclose(ifile);
		exit(1);
	}
	
	/* check WAV header is valid */
	if(wav_check_hdr(&wh, 2, 16))
	{
		fprintf(stderr, "Incorrect input file format.\n");
		fclose(ifile);
		exit(1);
	}
	samples = wh.data_sz / wh.fmt_bytesmpl;
	
	/* open output file */
	if(!(ofile = fopen(oname, "wb")))
	{
		fprintf(stderr, "Couldn't open output file %s for write\n", oname);
		fclose(ifile);
		exit(1);
	}
	
	/* tack on the wav header */
	if(fwrite(&wh, sizeof(wav_hdr), 1, ofile) != 1)
	{
		fprintf(stderr, "Write WAV header to output file failed.\n");
		fclose(ofile);
		fclose(ifile);
		exit(1);
	}
	
	/* process the audio data one stereo sample at a time */
	asum = 0;
	for(scnt=0;scnt<samples;scnt++)
	{
		/* get a stereo sample */
		if(fread(stereo, sizeof(int16_t), 2, ifile) != 2)
		{
			fprintf(stderr, "Unexepected EOF in input file.\n");
			fclose(ofile);
			fclose(ifile);
			exit(1);
		}
		
		/* convert stereo to mono and format for Midiverb */
		adc = ((stereo[0]>>4) + (stereo[1]>>4)) & 0xFFFE;
		
#if 1
		/* run microcode */
		for(i=0;i<128;i++)
		{
			/* next instruction */
			op = ucode[(i-1)&0x7f] >> 14; // op comes from previous instr
			addr = ucode[i]&0x3fff;		
			
#ifdef TRACE
			/* trace */
			fprintf(tfile, "%05X ", scnt);
			fprintf(tfile, "%02X ", i);
			fprintf(tfile, "%1X ", op);
			fprintf(tfile, "%04X ", addr);
			fprintf(tfile, "%04X ", asum);
#endif
			
			/* AI bus */
			if(i==0)
				ai = adc;
			else
				switch(op)
				{
					case 0:
					case 1:
						ai = dram[asum];
						break;
					case 2:
						ai = acc_out;
						break;
					case 3:
						ai = ~acc_out;
						break;
				}
			
#ifdef TRACE
			fprintf(tfile, "%04X ", ai&0xffff);
#endif
				
			/* DAC out */
			if((i==0x60)||(i==0x70))
			{
				sat = ai;
				if(sat > 4095)
					sat = 4095;
				else if(sat < -4096)
					sat = -4096;
			
				if(i==0x60)
					stereo[1] = sat<<3;
				else
					stereo[0] = sat<<3;
			}
			
			/* DRAM write */
			if((op&2) || (i==0))
				dram[asum] = ai;
			
			/* update Accumulator */
			acc = (op & 1) ? 0 : acc;
			sum = (ai>>1) + acc + ((ai < 0) ? 1 : 0);
			acc_out = sum;
			acc = sum;
			
#ifdef TRACE
			fprintf(tfile, "%04X ", acc&0xffff);
			fprintf(tfile, "\n");
#endif
			
			/* update address */
			asum = (asum + addr)&0x3fff;
		}
#else
		/* pass thru */
		stereo[0] = adc << 3;
		stereo[1] = adc << 3;
#endif
		
		/* put a stereo sample */
		if(fwrite(stereo, sizeof(int16_t), 2, ofile) != 2)
		{
			fprintf(stderr, "Error in output file.\n");
			fclose(ofile);
			fclose(ifile);
			exit(1);
		}
	}
	
#if 0
	/* dump dram */
	for(i=0;i<16384;i+=8)
	{
		fprintf(stdout, "%04X: ", i);
		for(uint16_t j=0;j<8;j++)
			fprintf(stdout, "%04X ", dram[i+j] & 0xffff);
		fprintf(stdout, "\n");
	}
#endif
	
	/* done */
	fclose(ofile);
	fclose(ifile);
	exit(0);
}
