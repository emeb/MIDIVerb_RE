/* vec_midiverb.c - generate midiverb test vectors */
/* 08-29-21 E. Brombaugh */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "midiverb.h"

#define max(x,y) ((x)<(y)?(y):(x))

int main(int argc, char **argv)
{
	int prog = 21;
	char *oname = "output.txt";
	FILE *ofile;
	int16_t in[2], out[2];
	int32_t samples = 10, scnt;
	mvblk mv;
	
	/* override defaults */
	if(argc > 1)
		prog = atoi(argv[1]);
	
	if(argc > 2)
		oname = argv[2];
	
	if(argc > 3)
		samples = atoi(argv[3]);
	
	/* open output file */
	if(!(ofile = fopen(oname, "w")))
	{
		fprintf(stderr, "Couldn't open output file %s for write\n", oname);
		exit(1);
	}
	
	/* init the midiverb emulator */
	midiverb_Init(&mv);
	midiverb_SetProg(&mv, prog);
	
	/* set output diagnostics */
	mv.dfile = ofile;
	
	/* process the audio data one stereo sample at a time */
	for(scnt=0;scnt<samples;scnt++)
	{
#if 0
		/* get a stereo sample */
		if(scnt%256 == 2)
		{
			in[0] = 8192;
			in[1] = 0;
		}
		else
		{
			in[0] = in[1] = 0;
		}
#else
		in[0] = 8192.0F * sinf((float)(max(scnt-1,0))/256.0F * 6.2832F);
#endif
		
		/* sample, data and header */
		//fprintf(ofile, "%05d % 6d % 6d\n", scnt, in[0], in[1]);
		//fprintf(ofile, "inst op addr asum ai acc\n");
		
		/* process thru midiverb emulator */
		midiverb_Proc(&mv, in, out);		
	}
		
	/* done */
	fclose(ofile);
	exit(0);
}
