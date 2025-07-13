# Makefile for microcode parser
# 05-01-2021 E. Brombaugh

CC = gcc

# output binary names
PARSE = parse_ucode
SIM = sim_ucode
MKUC = mk_mvucode
EMU = sim_midiverb
VEC = vec_midiverb

# hex files
HEX = midifex.hex midifverb.hex

# Targets
all: $(PARSE)

$(SIM): $(SIM).c wav_ops.o
	$(CC) -g -o $@ $< wav_ops.o
	
$(PARSE): $(PARSE).c
	$(CC) -g -o $@ $<
	
$(MKUC): $(MKUC).c
	$(CC) -g -o $@ $<
	
$(EMU): $(EMU).c wav_ops.o midiverb.o
	$(CC) -g -o $@ $< wav_ops.o midiverb.o
	
$(VEC): $(VEC).c midiverb.o
	$(CC) -g -o $@ $< midiverb.o -lm
	
# generate hex files
%.hex: %.bin
	xxd -c 1 -ps $< $@

clean:
	rm -f *.o $(PARSE) $(SIM)
	
