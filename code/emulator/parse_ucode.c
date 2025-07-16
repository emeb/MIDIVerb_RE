/* parse_ucode.c - parse midiverb microcode for analysis */
/* 04-30-21 E. Brombaugh */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

char *mnemonic[4] = 
{
	"SUMHALF",
	"LDHALF ",
	"STRPOS ",
	"STRNEG ",
};

char *comment[4] =
{
	"Acc = Acc + src/2 + sgn",
	"Acc = src/2 + sgn",
	"dst = Acc, Acc = Acc + Acc/2 + sgn",
	"dst = ~Acc, Acc = ~Acc/2 + sgn",
};

/*
 * gcd function
 */
int gcd(int a, int b)
{
	int R;
	while((a % b) > 0)
	{
		R = a % b;
		a = b;
		b = R;
	}
	return b;
}

/*
 * compute period of modulo addition
 */
int get_period(int inc)
{
#if 0
	int result = 0;
	int acc = 0;
	while(1)
	{
		acc = (acc + inc)%0x4000;
		result++;
		if(!acc)
			break;
	}
	return result;
#else
	return 16384 / gcd(16384, inc);
#endif
}

int main(int argc, char **argv)
{
	int prog = 21, max_samp = 1;
	char *fname = "midifverb.bin", *txt, txtbuf[16];
	FILE *file;
	int i, base;
	uint8_t rom[256];
	uint16_t microcode[128], instr, op, addr, op_buf[128], addr_buf[128],
		asum = 0;
	uint32_t wsb[16384], rsb[16384], samples;
	
	/* override defaults */
	if(argc > 1)
		prog = atoi(argv[1]);
	
	if(argc > 2)
		fname = argv[2];
	
	if(argc > 3)
		max_samp = atoi(argv[3]);
	
	/* load a program and analyze */
	if(!(file = fopen(fname, "rb")))
	{
		fprintf(stderr, "Couldn't open %s for read\n", fname);
		exit(1);
	}
	
	fprintf(stdout, "Parsing %s\n", fname);

	/* load ROM bytes for selected program */
	base = prog*256;
	fprintf(stdout, "Program %d - base address 0x%04X\n", prog, base);
	fseek(file, base, SEEK_SET);
	if(fread(rom, sizeof(uint8_t), 256, file) != 256)
	{
		fprintf(stderr, "Unexepected EOF.\n");
		fclose(file);
		exit(1);
	}
	fclose(file);
	
	/* format ROM bytes into microcode */
	for(i=0;i<128;i++)
	{
		microcode[i] = rom[(i*2-1)&0xff]<<8 | rom[(i*2-2)&0xff];
	}
	
	/* init scoreboards */
	for(i=0;i<16384;i++)
	{
		rsb[i] = wsb[i] = 0;
	}
	
	/* disassemble */
	asum = 0;//x08b6;
	samples = 0;
	fprintf(stdout, "PC  : OP OFFSET   MEMN    DD SRC   -> DST    DAC ; ACC OPs\n");
	while(samples<max_samp)
	{
		fprintf(stdout, "Sample %d\n", samples);
		for(i=0;i<128;i++)
		{
			/* address */
			fprintf(stdout, "0x%02X : ", i);
			
			/* raw data */
			op = microcode[(i-1)&0x7f] >> 14; // op comes from previous instr
			addr = microcode[i]&0xf3fff;			
			fprintf(stdout, "%1d 0x%04X   ", op, addr);
			
			/* save op, addr in history buf for macro parsing */
			op_buf[i] = op;
			addr_buf[i] = asum;
			//fprintf(stdout, "### %2d %04X\n", op_buf[i], addr_buf[i]);

			/* decode instruction */
			fprintf(stdout, "%s ", mnemonic[op]);
			
			/* decode DRAM operation */
			if((op&2) || (i==0))
				fprintf(stdout, "WR ");
			else
				fprintf(stdout, "RD ");
			
			/* decode source */
			if(i==0)
				txt = " ADC";
			else
			{
				if(op&2)
				{
					if(op&1)
						txt = "~ACC";
					else
						txt = " ACC";
				}
				else
				{
					sprintf(txtbuf, "0x%04X", asum);
					txt = txtbuf;
					rsb[asum]++;
				}
			}
			fprintf(stdout, "%s -> ", txt);
			
			/* decode destination */
			if((op&2) || (i==0))
			{
				sprintf(txtbuf, "0x%04X", asum);
				txt = txtbuf;
				wsb[asum]++;
			}
			else
				txt = "ACC ";
			fprintf(stdout, "%s", txt);
			
			/* DAC also? */
			if(i==96)
				fprintf(stdout, ", DAC R");
			else if(i==112)
				fprintf(stdout, ", DAC L");
			else
				fprintf(stdout, "       ");
			
			/* comment */
			fprintf(stdout, " ;%s ", comment[op]);
						
			fprintf(stdout, "\n");
			asum = (asum + addr)&0x3fff;
			
			/* search for ap macro */
			if(i>2)
			{
				int ap_addr_begin = addr_buf[i-3], ap_addr_end;
				
				if(op_buf[i-3] == 0)
				{
					if(op_buf[i-2] == 3)
					{
						ap_addr_end = addr_buf[i-2];
						if((op_buf[i-1] == 0) && (addr_buf[i-1] == ap_addr_begin))
						{
							if((op_buf[i] == 0) && (addr_buf[i] == ap_addr_begin))
							{
								fprintf(stdout, "; Allpass, len = %d\n", ap_addr_end - ap_addr_begin);
							}
						}
					}
				}
			}
		}
		samples++;
	}
	
done:
#if 1
	/* dump scoreboards */
	fprintf(stdout, "\nscore: loc, w, r\n");
	for(i=0;i<16384;i++)
	{
		if((wsb[i]!=0) || (rsb[i] != 0))
			fprintf(stdout, "%04X: %4d, %4d, %c\n", i, wsb[i], rsb[i], ((wsb[i]!=0) && (rsb[i] != 0)) ? '*': ' ');
	}
#endif
	
	exit(0);
}
