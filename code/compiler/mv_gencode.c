/* mv_gencode.c - analyze pre-formatted Midiverb emulator microcode */
/* and convert to unrolled C for optimization */
/* 09-20-21 E. Brombaugh */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include "mv_ucode.h"

#define dprintf(...) if(debug) fprintf (stderr, __VA_ARGS__)

int main(int argc, char **argv)
{
	uint8_t prog, pstart = 0, pend = 62;
	char *oname = "mv_progs.c";
	FILE *ofile;
	uint8_t discard, routinst = 0x60, loutinst = 0x70, acnt;
	uint16_t *iptr, i, j, op[128], addr[128], asum[129], raddr, laddr;
	int32_t c;
	uint16_t optbits = 0x00ff;
	uint8_t debug = 0;
	
	/* parse options */
	opterr = 0;

	while((c = getopt (argc, argv, "d:O:o:p:")) != -1)
	{
		switch(c)
		{
			case 'd':
				debug = atoi(optarg);
				break;
			
			case 'O':
				optbits = atoi(optarg);
				break;
			
			case 'o':
				oname = optarg;
				break;
			
			case 'p':
				pstart = pend = atoi(optarg);
				break;
			
			case '?':
				if(optopt == 'b')
					fprintf (stderr, "Option -%c requires a filename.\n", optopt);
				else if(optopt == 'p')
					fprintf (stderr, "Option -%c requires an program number.\n", optopt);
				else if(isprint(optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,
						"Unknown option character `\\x%x'.\n",
						optopt);
				return 1;
				
			default:
				abort();
		}
	}
	
	/* open output file */
	if(!(ofile = fopen(oname, "w")))
	{
		fprintf(stderr, "Couldn't open %s for output\n", oname);
		exit(1);
	}
	
	/* output header */
	fprintf(ofile, "/*\n");
	fprintf(ofile, " * %s - auto-generated C code for Midiverb programs\n", oname);
	fprintf(ofile, " */\n");
	fprintf(ofile, "#include <stdint.h>\n");
	fprintf(ofile, "uint16_t addr;\n");
	fprintf(ofile, "int16_t acc, mem[16384];\n");

	/* loop over all programs */
	for(prog = pstart;prog <= pend;prog++)
	{
		dprintf("Program %d\n", prog);
		acnt = 0;
		
		/* load prog for analysis */
		iptr = &mv_ucode[prog*128];
		for(i=0;i<128;i++)
		{
			/* get operation & address offset */
			op[i] = (*iptr >> 14)&3;
			addr[i] = *iptr++ & 0x3fff;
		}
		
		/* disable acc at fixed special addrs */
		op[0x00] |= 4; // no acc outputs on addr 0x00
		op[0x60] |= 4; // no acc outputs on addr 0x60
		op[0x70] |= 4; // no acc outputs on addr 0x70
		
		if(optbits & 1)
		{
			/*--------------------------------------------------------------*/
			/* Detect and Remove trailing NOPs                              */
			/*--------------------------------------------------------------*/
			/* search forward from zero to see if previous acc is ever used */
			discard = 1;
			for(i=0;i<128;i++)
			{
				if(!(op[i]&4)&&(op[i]&2))
				{
					discard = 0;
					fprintf(stderr, "Warning: Prev acc used @ op %d\n", i);
				}
				else if(!(op[i]&4)&&(op[i]&1))
				{
					dprintf("Prev acc discarded @ op %d\n", i);				
					break;
				}
			}
			
			/* Substitute NOPs */
			if(discard)
			{
				for(i=127;i>0;i--)
				{
					if(!(op[i]&4) && (op[i]&2))
						break;	// not DAC and write operation so acc was used
					else
						op[i] |= 4;	// otherwise unused so NOP
				}
				dprintf("NOPed final %d acc instrs\n", 127-i);
			}
		}
		
		/* determine address offsets for each instr */
		asum[0] = 0;
		for(i=0;i<128;i++)
			asum[i+1] = (asum[i] + addr[i])&0x3fff;
		if(asum[128] != 1)
			fprintf(stderr, "Warning: offset sum = %d\n", asum[128]);
		
		if(optbits & 2)
		{
			/*--------------------------------------------------------------*/
			/* Simplify Outputs                                             */
			/*--------------------------------------------------------------*/
			/* get addr where R & L outputs are read from */
			raddr = asum[0x60];
			laddr = asum[0x70];
			
			/* search for where those are written */
			for(i=1;i<128;i++)
			{
				if(i!=0x60)
				{
					if(asum[i] == raddr)
					{
						dprintf("raddr 0x%04X is %s @ instr 0x%02X\n", raddr,
							op[i]&2?"written":"read", i);
						if(op[i]&2)
						{
							routinst = i;
						}
					}
				}
				if(i!=0x70)
				{			
					if(asum[i] == laddr)
					{
						dprintf("laddr 0x%04X is %s @ instr 0x%02X\n", laddr,
							op[i]&2?"written":"read", i);
						if(op[i]&2)
						{
							loutinst = i;
						}
					}
				}
			}
			
			/* simplify addressing if things were moved */
			if(routinst != 0x60)
			{
				dprintf("rout moved to instr 0x%02X\n", routinst);
				addr[0x5f] = (addr[0x5f] + addr[0x60]) & 0x3fff;
				addr[0x60] = 0;
			}
			if(loutinst != 0x70)
			{
				dprintf("lout moved to instr 0x%02X\n", loutinst);
				addr[0x6f] = (addr[0x6f] + addr[0x70]) & 0x3fff;
				addr[0x70] = 0;
			}
		}

		if(optbits & 4)
		{
			/*--------------------------------------------------------------*/
			/* remove dead-end acc ops                                      */
			/*--------------------------------------------------------------*/
			discard = 1;
			for(i=127;i>1;i--)
			{
				if(discard)
				{
					dprintf("Removed dead-end acc op @ instr %d\n", i);
					op[i] |= 4;
					if(op[i]&2)
						discard = 0;
				}
				else if((op[i]&3)==1)
					discard = 1;
			}
		}
		
		if(optbits & 16)
		{
			/*--------------------------------------------------------------*/
			/* collapse unity accum reads                                   */
			/*--------------------------------------------------------------*/
			discard = 0;
			for(i=0;i<128;i++)
			{
				if(discard == 0)
				{
					//if(((op[i]==0) || (op[i]==1)) && (addr[i]==0)) // doesn't work
					if((op[i]==0) && (addr[i]==0))
						discard = 1;
				}
				else
				{
					if(op[i] == 0)
					{
						dprintf("Unity acc read @ instr %d\n", i);
						op[i-1] |= 4;
						op[i] = 8;
					}
					discard = 0;
				}
			}
		}

		if(optbits & 32)
		{
			/*--------------------------------------------------------------*/
			/* collapse unity clear reads                                   */
			/*--------------------------------------------------------------*/
			discard = 0;
			for(i=0;i<128;i++)
			{
				if(discard == 0)
				{
					if((op[i]==1) && (addr[i]==0))
						discard = 1;
				}
				else
				{
					if(op[i] == 0)
					{
						dprintf("Unity clear read @ instr %d\n", i);
						op[i-1] |= 4;
						op[i] = 9;
					}
					discard = 0;
				}
			}
		}

		if(optbits & 64)
		{
			/*--------------------------------------------------------------*/
			/* remove redundant writes                                      */
			/*--------------------------------------------------------------*/
			discard = 0;
			for(i=127;i>0;i--)
			{
				if(discard == 0)
				{
					if((op[i] & 2) && (addr[i-1] == 0))
						discard = 1;
				}
				else
				{
					if(op[i] & 2)
					{
						dprintf("Discard redundant write @ instr %d\n", i);
						op[i] = 12 + (op[i] & 1);
					}
					if(addr[i-1] != 0)
						discard = 0;
				}
			}
		}

		/* start prog */
		fprintf(ofile, "void prog%02d(int16_t in, int16_t *outl, int16_t *outr) {\n", prog);
		
		/* loop over all instructions, decode and output */
		asum[0] = 0;
		for(i=0;i<128;i++)
		{
			/* start line */
			fprintf(ofile, "\t");
			
			/* decode operation */
			if(i==0)
			{
				/* always write input to current address on instr 0 */
				fprintf(ofile, "mem[addr]=in; ");
			}
			else if(i==0x60)
			{
				if(routinst==i)
				{
					/* default loc, so always read from mem to right out */
					fprintf(ofile, "*outr = mem[addr]; ");
				}
			}
			else if(i==0x70)
			{
				if(loutinst==i)
				{
					/* default loc, so always read from mem to left out */
					fprintf(ofile, "*outl = mem[addr]; ");
				}
			}
			else
			{
				/* handle writes */
				if(op[i] & 2)
				{
					if(i==routinst)
						if(optbits & 8)
							fprintf(ofile, "*outr=%cacc; ", op[i]&1?'-':' ');
						else	
							fprintf(ofile, "*outr=%cacc; ", op[i]&1?'~':' ');
					else if(i==loutinst)
						if(optbits & 8)
							fprintf(ofile, "*outl=%cacc; ", op[i]&1?'-':' ');
						else
							fprintf(ofile, "*outl=%cacc; ", op[i]&1?'~':' ');
					else
						if(optbits & 8)
							fprintf(ofile, "mem[addr]=%cacc; ", op[i]&1?'-':' ');
						else
							fprintf(ofile, "mem[addr]=%cacc; ", op[i]&1?'~':' ');
				}
				
				/* decode acc operation */
				switch(op[i])
				{
					case 0:
						if(optbits & 8)
							fprintf(ofile, "acc=acc+(mem[addr]>>1); ");
						else
							fprintf(ofile, "acc=acc+(mem[addr]>>1)+((mem[addr]>>15)&1); ");
						break;
					
					case 1:
						if(optbits & 8)
							fprintf(ofile, "acc=(mem[addr]>>1); ");
						else
							fprintf(ofile, "acc=(mem[addr]>>1)+((mem[addr]>>15)&1); ");
						break;
					
					case 2:
					case 12:
						if(optbits & 8)
							fprintf(ofile, "acc=acc+(acc>>1); ");
						else
							fprintf(ofile, "acc=acc+(acc>>1)+((acc>>15)&1); ");
						break;
					
					case 3:
					case 13:
						if(optbits & 8)
							fprintf(ofile, "acc=((-acc)>>1); ");
						else
							fprintf(ofile, "acc=((~acc)>>1)+(((~acc)>>15)&1); ");
						break;
					
					case 8:
						/* unity acc */
						fprintf(ofile, "acc=acc+mem[addr]; ");
						break;

					case 9:
						/* unity clr */
						fprintf(ofile, "acc=mem[addr]; ");
						break;

					default:
						break;
				}
			}
			
			/* update address */
			if(addr[i])
				fprintf(ofile, "addr=(addr+0x%04X)&0x3fff;", addr[i]);
			else
				acnt++;

			asum[i+1] = (asum[i] + addr[i])&0x3fff;
			
			fprintf(ofile, " // %d\n", i);
		}
		
		/* end prog */
		fprintf(ofile, "}\n\n");
		
		dprintf("Removed %d null addres ops\n", acnt);
		if(asum[128]!=1)
			fprintf(stderr, "Warning: Final Offset sum = %d\n", asum[128]);
	}
	
	/* generate an array of function pointers to all the programs */
	fprintf(ofile, "void (*mv_progs[63])(int16_t, int16_t *, int16_t *) = {\n");
	j=pstart;
	for(i=0;i<63;i++)
	{
		fprintf(ofile, "\tprog%02d,\n", j);
		if(j<pend)
			j++;
	}
	fprintf(ofile, "};\n\n");
	
	fclose(ofile);
	exit(0);
}
