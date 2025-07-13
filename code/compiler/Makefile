# Makefile for microcode generator
# 05-01-2021 E. Brombaugh

CC = gcc
ARCH = /opt/launchpad/gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi
CCC = $(ARCH)-gcc
OBJDMP = $(ARCH)-objdump

# output binary names
GEN = mv_gencode
OUT = mv_progs
SIM = sim_mvprogs
TST = tst_mvprogs

CFLAGS = -g -Os

CCCFLAGS += -mlittle-endian -mthumb
#CCCFLAGS += -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16
#CCCFLAGS += -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
CCCFLAGS += -mcpu=cortex-m0

# Targets
all: $(GEN)

$(GEN): $(GEN).c mv_ucode.h
	$(CC) -g -o $@ $<

$(OUT).c: $(GEN)
	./$(GEN)

$(SIM): $(SIM).c $(OUT).o wav_ops.o
	$(CC) -g -o $@ $< $(OUT).o wav_ops.o

$(OUT).arm: $(OUT).c
	$(CCC) $(CCCFLAGS) -Os -c -o $@ $<
	
$(TST): $(TST).c ../emulator/midiverb.c $(OUT).o wav_ops.o
	$(CC) -g -o $@ $< ../emulator/midiverb.c $(OUT).o wav_ops.o

disassemble: $(OUT).arm
	$(OBJDMP) -d -S $< > $(OUT).dis

clean:
	rm -f *.o $(GEN) $(SIM) $(TST) $(OUT).c $(OUT).arm $(OUT).dis
	
