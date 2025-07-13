# Makefile for Icarus Verilog simulation
# 03-19-2021 E. Brombaugh

# sources
SOURCES = 	mvop.v AM25L04D.v TMS4416.v 
        
# top level
TOP = mvop
			
# Executables
VLOG = iverilog
WAVE = gtkwave

# targets
all: $(TOP).vcd

wave: $(TOP).vcd $(TOP).gtkw
	$(WAVE) $(TOP).gtkw
	
$(TOP).vcd: $(TOP)
	./$(TOP)

$(TOP): $(SOURCES)
	$(VLOG) -D icarus -o $(TOP) $(SOURCES)
	
clean:
	rm -rf a.out *.obj $(TOP) $(TOP).vcd
	
