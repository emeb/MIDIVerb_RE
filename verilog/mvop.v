// mvop.v - simulation of Midiverb logic
// 04-12-21 E. Brombaugh

`default_nettype none
`timescale 1ns/1ps

module mvop;
	reg clk;
	reg [7:0] mode_a;
	wire rco_lo, mode_rco;
	reg step_32;
	wire ld_dac;
	reg mode_a3_dly;
	reg mode_rco_dly, nmode_rco_dly;
	wire dac_oe, ndac_oe;
	integer ofile;
	
	initial
    begin
`ifdef icarus
  		//$dumpfile("mvop.vcd");
		//$dumpvars;
`endif
        
		clk = 1'b0;
		mode_a = 8'b0;
		step_32 = 1'b0;
		mode_a3_dly = 1'b0;
		
		ofile = $fopen("samples.txt", "w");
		
`ifdef icarus
        // stop after 1 sec
		#25000000 $finish;
`endif
	end
	
	// 6MHz clock
	always
		#83.3 clk = ~clk;
	
	// --------------------------
	// mode address - program counter
	// --------------------------
	initial
	begin
		mode_a = 8'h00;
		mode_a3_dly = 1'b1; 
		nmode_rco_dly = 1'b1;
		mode_rco_dly = 1'b0;
	end
	always @(posedge clk)
	begin
		#25
		mode_a <= mode_a + 8'd1;
		mode_a3_dly <= !mode_a[3];
		nmode_rco_dly <= !mode_rco;
		mode_rco_dly <= mode_rco;
	end
	assign rco_lo = mode_a[3:0] == 4'hf ? 1'b1 : 1'b0;
	assign mode_rco = (mode_a[7:4] == 4'hf)&rco_lo ? 1'b1 : 1'b0;
	
	always @(posedge clk or negedge mode_a[7])
		if(!mode_a[7])
			step_32 <= 1'b0;
		else
			step_32 <= rco_lo & mode_a[4];
	
	// --------------------------
	// nasty async clock chain for 3MHz clock
	// --------------------------
	wire ras, clk_hireg, clk_cout, ncas, net157, hi_clk;
	initial
	begin
		force clk_hireg = 1'b0;
		force clk_cout = 1'b1;
		force ncas = 1'b0;
		force hi_clk = 1'b1;
		
		#20 release clk_hireg;
		release clk_cout;
		release hi_clk;
		#30 release ncas;
	end
	not #(10) u46_1(ras, mode_a[0]);
	not #(10) u46_2(clk_hireg, ras);
	not #(10) u46_3(clk_cout, clk_hireg);
	not #(10+35) u46_4(ncas, clk_cout);
	not #(10) u46_6(net157, ncas);
	xor #(10) u50_2(hi_clk, mode_a[0], clk_cout);
	
	nand #(10) u41_5(ld_dac, step_32, mode_a[6]);
	not #(10) u15_3(dac_oe, nmode_rco_dly);
	not	#(10) u15_4(ndac_oe, dac_oe);
	initial
	begin
		force ld_dac = 1'b1;
		force dac_oe = 1'b0;
		force ndac_oe = 1'b1;
		
		#20 release ld_dac;
		release dac_oe;
		release ndac_oe;
	end
	
	// --------------------------
	// microcode ROM
	// --------------------------
	reg [5:0] psel_a;
	reg [7:0] u51[16383:0], dsp_d, dsp_dd;
	initial
	begin
		$readmemh("u51.hex", u51);
		psel_a = 6'd21;
	end
	always @(*)
		#100 dsp_d = u51[{psel_a,mode_a}];
	initial
		dsp_dd = 8'h00;
	always @(posedge hi_clk)
		dsp_dd <= dsp_d;
	
	// --------------------------
	// decode the instructions
	// --------------------------
	reg net244, net210, ld_dsp;
	wire ndram_wr, net243, net245, net246, nrd_r0, nrd_r1, nclr_acc;
	
	// top two bits of first byte are instruction type
	// 2'b00 = ACC = DRAM[addr] + ACC/2
	// 2'b01 = ACC = DRAM[addr] + 1
	// 2'b10 = DRAM[addr] =  ACC, ACC = ACC + ACC/2
	// 2'b11 = DRAM[addr] = ~ACC, ACC = ~ACC + 1
	initial
	begin
		net244 = 1'b1;
		net210 = 1'b1;
		force ndram_wr = 1'b1;
		#30 release ndram_wr;
	end
	always @(posedge mode_a[0])
	begin
		net244 <= !dsp_dd[7];
		net210 <= !dsp_dd[6];
	end
	wire [1:0] instr = ~{net244,net210}; 
	nor #(10) u43_3(net245, net244, mode_a[0]);
	nor #(10) u43_4(ndram_wr, net245, mode_rco_dly);		// DRAM Write - low true
	nor #(10) u43_2(net246, net210, mode_a[0]);
	nand #(10) u44_4(nrd_r1, net245, net246);
	nand #(10) u44_1(nclr_acc, net246, ncas);
	nand #(10) u44_2(nrd_r0, net245, nrd_r1);
	nand #(10) u41_4(net243, ld_dac, nmode_rco_dly);
	
	// U42_1 flop
	initial
		ld_dsp = 1'b0;
	always @(posedge net157 or negedge ras)
		if(!ras)
			ld_dsp <= 1'b1;
		else
			ld_dsp <= net243;
	
	// --------------------------
	// ADC input
	// --------------------------
	wire [11:0] sar_q;
	wire nconv_complete, nc1;
	
	// SAR register
	AM25L04D u7(
		.clk(mode_a3_dly),
		.s(ndac_oe),
		.d(1'b1),	// dummy SAR data
		.e(1'b0),
		.d0(),
		.q(sar_q),
		.cc(nconv_complete)
	);
	
	wire ndac_d11, sar_ena, nsar_ena, nrd_r2, rd_r2;
	reg dac_sat;
	nand #(10) u41_6(sar_ena, nmode_rco_dly, conv_complete);
	not #(10) u15_2(nsar_ena, sar_ena);
	nor #(10) u13_1(rd_r2, sar_ena, dac_sat);
	not #(10) u15_5(nrd_r2, rd_r2);
	initial
	begin
		force nsar_ena = 1'b1;
		#30 release nsar_ena;
	end
	// buffer, add lsbit and sign-extend to 16 bits
	wire [15:0] dac_d;
	bufif0 #(10) u17[15:0](dac_d, {{3{sar_q[11]}},sar_q,1'b1}, {16{nsar_ena}});
	not #(10) u15_6(ndac_d11, dac_d[11]);
	
	// mux drives
	wire net238, conv_complete, mux_a, mux_b, mux_c;
	reg dly_mode_a4;
	not #(10) u15_1(conv_complete, nconv_complete);
	always @(posedge mode_a[3] or negedge conv_complete)
		if(!conv_complete)
			dly_mode_a4 <= 1'b1;
		else
			dly_mode_a4 <= mode_a[4];
	nor #(10) u13_2(net238, mode_a[5], mode_a[5]);
	nor #(10) u13_3(mux_b, dly_mode_a4, mode_a[5]);
	nor #(10) u13_4(mux_c, dly_mode_a4, net238);
	buf (pull0, pull1) #(10) r6(mux_a, nconv_complete);
		
	// --------------------------
	// DRAM address calculations
	// --------------------------
	wire [7:0] reg_ms;
	reg [7:0] ms, ms0, ms1;
	reg ms_cout, ms_cin;
		
	initial
	begin
		ms1 = 8'h00;
		ms_cin = 1'b0;
	end
	
	// summation
	always @(*)
		{ms_cout,ms} = {1'b0,dsp_dd} + {1'b0,reg_ms} + {8'h00,ms_cin};
	
	// first register
	always @(posedge clk_cout)
		ms0 <= ms;
	
	// carry pipeline with clear
	wire ms_clrc, net248;
	not #(5) u46_5(net248, clk);
	nand #(10) u44_3(ms_clrc, net248, nras);
	always @(posedge clk_cout or negedge ms_clrc)
		if(!ms_clrc)
			ms_cin <= 1'b0;
		else
			ms_cin <= ms_cout;
	
	// second register
	always @(posedge clk_hireg)
		ms1 <= ms;
	
	// diag - ram addr
	reg [7:0] raddr;
	reg [13:0] ramaddr;
	always @(negedge nras)
		raddr <= reg_ms;
	always @(negedge ncas)
		ramaddr <= {reg_ms[5:0],raddr};
	
	// mux the two registers onto the addr bus
	bufif0 #(10) u31[7:0](reg_ms, ms0, {8{clk_cout}});
	bufif0 #(10) u28[7:0](reg_ms, ms1, {8{clk_hireg|!clk_cout}});
	buf (weak0,weak1) ubk[7:0](reg_ms,reg_ms);	// bus keeper models lag
	
	// clear addr @ start
	initial
	begin
		force ms = 8'h00;
		ms0 = 8'h00;
		ms1 = 8'h00;
		ms_cin = 1'b0;
		ms_cout = 1'b0;
		//#42600 release ms;
		#100 release ms;
	end
	
	// --------------------------
	// DRAM 
	// --------------------------
	// scramble address
	wire nras = mode_a[0];
	wire [7:0] dram_addr = {reg_ms[7],reg_ms[0],reg_ms[3],reg_ms[5],reg_ms[4],reg_ms[2],reg_ms[1],reg_ms[6]};
	wire [15:0] ai;

	// all four chips in one instance
	TMS4416 u14(
		.nras(nras),
		.ncas(ncas),
		.nwe(ndram_wr),
		.ng(1'b0),
		.a(dram_addr),
		.dq(ai)
	);

	// --------------------------
	// DSP Accumulator & regs 
	// --------------------------
	// accumulator
	reg [15:0] acc, r0, r1, r2, ls;
	always @(posedge ld_dsp or negedge nclr_acc)
		if(!nclr_acc)
			acc <= 16'h0000;
		else
			acc <= ls;
	
	// scale, summation with dither
	always @(*)
		ls = acc + {ai[15],ai[15:1]} + {15'd0,ai[15]};
	
	// load registers
	always @(posedge ld_dsp)
	begin
		r0 <= ls;
		r1 <= ~ls;
	end
	
	// init to stable states
	initial
	begin
		acc = 16'h0000;
		r0 = 16'h0000;
		r1 = 16'h0000;
	end
	
	// read registers onto ai bus
	bufif0 #(10) u20[15:0](ai, dac_d, {16{ndac_oe}});
	bufif0 #(10) u26[15:0](ai, r0, {16{nrd_r0}});
	bufif0 #(10) u29[15:0](ai, r1, {16{nrd_r1}});
	
	// keep ai bus
	//buf (pull0,pull1) aikeep[15:0](ai, ai);
	
	// get ai past initial x
	initial
	begin
		force ai = 16'h0000;
		#100 release ai;
	end

	// DAC output register
	initial
		r2 <= 16'h0000;
	always @(posedge ld_dac)
		r2 <= ai;
	
	// Detect top bit state
	reg net128;
	always @(ai)
		net128 = (ai[15:12] == 4'h0) | (ai[15:12] == 4'hf);
	
	reg dac_pull;
	initial
	begin
		dac_sat <= 1'b0;
		dac_pull <= 1'b0;
	end
	always @(posedge ld_dac)
	begin
		dac_sat <= !net128;
		dac_pull <= ai[15];
	end
	
	// read register onto DAC bus
	bufif0 #(10) u23[15:0](dac_d, r2, {16{nrd_r2}});

	// pull DAC bus
	buf (pull0,pull1) sip1[7:0](dac_d[11:4], {dac_pull,{7{!dac_pull}}});
	
	// file output - ADC input and newline
	always @(negedge ncas)
	begin
		if(!ndac_oe)
			$fwrite(ofile, "%5d ", $signed(ai));
		if(mode_a == 8'hfe)
			$fwrite(ofile, "\n");
	end
	
	// file output - DAC R / L
	always @(posedge ld_dac)
	begin
		if(!ncas)
			$fwrite(ofile, "%5d ", $signed(ai));
	end
endmodule
