/*
 * mk_mvucode.c - parse raw microcode binary into C header
 * 07-31-2021 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv)
{
	char *uname = "midifverb.bin", *oname = "mv_ucode.h";
	FILE *ufile, *ofile;
	uint8_t rom[16384], prog, op;
	uint16_t i, addr, instr;
	
	/* override defaults */
	if(argc > 1)
		uname = argv[1];
	
	if(argc > 2)
		oname = argv[2];
	
	/* load microcode */
	if(!(ufile = fopen(uname, "rb")))
	{
		fprintf(stderr, "Couldn't open microcode file %s for read\n", uname);
		exit(1);
	}
	
	if(fread(rom, sizeof(uint8_t), 16384, ufile) != 16384)
	{
		fprintf(stderr, "Unexepected EOF in microcode file.\n");
		fclose(ufile);
		exit(1);
	}
	fclose(ufile);

	/* open output file */
	if(!(ofile = fopen(oname, "w")))
	{
		fprintf(stderr, "Couldn't open output file %s for write\n", oname);
		exit(1);
	}
	
	/* tack on the header */
	fprintf(ofile, "/* mv_ucode.h - processed Midiverb I microcode */\n");
 	fprintf(ofile, "/* generated by mk_mvucode */\n");
 	fprintf(ofile, "uint16_t mv_ucode[16384] = {");
	
	/* loop over all programs */
	for(prog=0;prog<63;prog++)
	{
		/* insert program delimiters */
		fprintf(ofile, "\n\t// Program %d", prog);
		
		/* loop over all instructions */
		for(i=0;i<128;i++)
		{
			/* format 8 instr / line */
			if(i%8 == 0)
				fprintf(ofile, "\n\t");
			
			/* build instruction */
			op = (rom[(prog<<8) + ((i*2-3)&0xff)] >> 6) & 0x03;
			addr = rom[(prog<<8) + ((i*2-2)&0xff)] +
					((rom[(prog<<8) + ((i*2-1)&0xff)] & 0x3f) << 8);
			instr = (op<<14) + addr;
			fprintf(ofile, "0x%04X, ", instr);
#if 0
			printf("%2x %2x ", prog, i);
			printf("rom[%4x] = %2x, ", (prog<<8) + ((i*2-3)&0xff), rom[(prog<<8) + ((i*2-3)&0xff)]);
			printf("op = %1x, ", op);
			printf("rom[%4x] = %2x, ", (prog<<8) + ((i*2-2)&0xff), rom[(prog<<8) + ((i*2-2)&0xff)]);
			printf("rom[%4x] = %2x, ", (prog<<8) + ((i*2-1)&0xff), rom[(prog<<8) + ((i*2-1)&0xff)]);
			printf("addr = %4x, ", addr);
			printf("instr = %4x\n", instr);			
#endif
		}
	}

	/* wrap up */
 	fprintf(ofile, "\n};\n");
	fclose(ofile);
}

	