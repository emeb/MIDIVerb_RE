/* sim_midiverb.c - test midiverb emulator on .wav audio */
/* 07-31-21 E. Brombaugh */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "wav_ops.h"
#include "midiverb.h"

int main(int argc, char **argv)
{
	int prog = 21;
	char *iname = "input.wav", *oname = "output.wav";
	FILE *ifile, *ofile;
	int16_t in[2], out[2];
	wav_hdr wh;
	int32_t samples, scnt;
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
		midiverb_Proc(&mv, in, out);
		
		/* put a stereo sample */
		if(fwrite(out, sizeof(int16_t), 2, ofile) != 2)
		{
			fprintf(stderr, "Error in output file.\n");
			fclose(ofile);
			fclose(ifile);
			exit(1);
		}
	}
		
	/* done */
	fclose(ofile);
	fclose(ifile);
	exit(0);
}
