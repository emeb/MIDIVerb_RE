/* tst_mprogs.c - test midiverb generated code on .wav audio */
/* 09-31-21 E. Brombaugh */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "wav_ops.h"
#include "../roms/midiverb.h"

/* the array of individual programs */
extern void (*mv_progs[63])(int16_t, int16_t *, int16_t *);

int main(int argc, char **argv)
{
	int prog = 21;
	char *iname = "input.wav", *oname = "output.wav", *dname = "diag.txt";
	FILE *ifile, *ofile, *dfile;
	int16_t in[2], ref[2], out[2];
	wav_hdr wh;
	int32_t samples, scnt, chl, errs = 0;
	mvblk mv;
	
	/* override defaults */
	if(argc > 1)
		prog = atoi(argv[1]);
	
	if(argc > 2)
		iname = argv[2];
	
	if(argc > 3)
		oname = argv[3];
		
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
	
	/* open diag file */
	if(!(dfile = fopen(dname, "w")))
	{
		fprintf(stderr, "Couldn't open diag file %s for write\n", dname);
		fclose(ofile);
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
		
	/* init the midiverb emulator */
	midiverb_Init(&mv);
	midiverb_SetProg(&mv, prog);
	
	/* process the audio data one stereo sample at a time */
	for(scnt=0;scnt<samples;scnt++)
	{
		/* get a stereo sample */
		if(fread(in, sizeof(int16_t), 2, ifile) != 2)
		{
			fprintf(stderr, "Unexepected EOF in input file.\n");
			fclose(ofile);
			fclose(ifile);
			exit(1);
		}
		
		/* process thru midiverb emulator */
		midiverb_Proc(&mv, in, ref);
		
		/* scale for input */
		int16_t mono = ((in[0]>>4) + (in[1]>>4)) & 0xFFFE;
		
		/* process thru midiverb emulator */
		(*mv_progs[prog])(mono, &out[0], &out[1]);
		
		/* unscale and saturate */
		for(chl=0;chl<2;chl++)
		{
			int32_t sat = out[chl];
			sat = sat > 4095 ? 4095 : sat;
			sat = sat < -4096 ? -4096 : sat;
			out[chl] = sat<<3;
		}
		
		/* test against reference */
		fprintf(dfile, "% 6d, % 6d  ", in[0], in[1]);
		fprintf(dfile, "% 6d, % 6d  ", ref[0], ref[1]);
		fprintf(dfile, "% 6d, % 6d  ", out[0], out[1]);
		fprintf(dfile, "%c, %c  ", ref[0] == out[0] ? ' ': 'x',
			ref[1] == out[1] ? ' ': 'x');
		fprintf(dfile, "\n");
		
		if(ref[0] != out[0])
			errs++;
		if(ref[1] != out[1])
			errs++;
		
		/* put a stereo sample */
		if(fwrite(out, sizeof(int16_t), 2, ofile) != 2)
		{
			fprintf(stderr, "Error in output file.\n");
			fclose(dfile);
			fclose(ofile);
			fclose(ifile);
			exit(1);
		}
	}
	
	printf("Samples = %d, Errors = %d\n", samples, errs);
		
	/* done */
	fclose(dfile);
	fclose(ofile);
	fclose(ifile);
	exit(0);
}
