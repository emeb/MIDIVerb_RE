// TMS4416.v DRAM stub model
// 04-13-21 E. Brombaugh

module TMS4416(
	input nras,
	input ncas,
	input nwe,
	input ng,
	input [7:0] a,
	inout [15:0] dq);
	integer ofile;
	
	// grab row address
	reg [7:0] row;
	always @(negedge nras)
		row <= a;
	
	// grab column address
	reg [5:0] col;
	always @(negedge ncas)
		col <= a[6:1];
	
	// memory array
	reg [15:0] mem[16383:0];
	integer i;
	initial
	begin
		for(i=0;i<16384;i=i+1)
			mem[i] = 16'h0000;
		ofile = $fopen("dram.txt", "w");
	end
	
	// read
	reg [15:0] rdat;
	always @(negedge ncas)
		if(!nras & !ncas & nwe)
		begin
			rdat = mem[{col,row}];
			$fwrite(ofile, "Read  %X <- %X\n", rdat, {col,row});
		end
		
	// write latch - MVOP only uses early write
	reg [15:0] wdat;
	wire wstb = !ncas & !nwe;
	always @(posedge wstb)
			wdat <= dq;
	
	// delay wstb for write
	wire dwstb;
	buf #(10) dly(dwstb, wstb);
	always @(posedge dwstb)
		if(!nwe)
		begin
			mem[{col,row}] <= wdat;
			$fwrite(ofile, "Write %X -> %X\n", wdat, {col,row});
		end
		
	// bidir I/O
	reg oe;
	initial
		oe = 1'b0;
	always @(*)
		if(!oe)
			oe = !nras & !ncas;
		else
			oe = !ncas;
	wire re = oe&!ng&nwe;
	bufif1 odrive[15:0](dq, rdat, {16{re}});
endmodule
