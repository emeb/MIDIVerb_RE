// AM25L04D.v SAR stub model
// 04-12-21 E. Brombaugh

// uncomment this to generate a sinewave data stream
`define GEN_SIN

module AM25L04D(
	input clk,
	input s,
	input d,
	input e,
	output reg d0,
	output reg [11:0] q,
	output reg cc);
	
	reg [3:0] seq;

`ifdef GEN_SIN
	integer phs;
	initial
		phs = 0;
	reg [11:0] isin;
`endif

	initial
	begin
		seq = 4'h0;
		d0 = 1'b0;
		q = 12'h000;
		cc = 1'b0;
	end
	
	always @(posedge clk)
	begin
		if(!s)
		begin
			seq <= 4'h0;
			q <= 12'h7ff;
			cc <= 1'b1;
		end
		else
		begin
			if(seq < 4'hc)
				seq <= seq + 1;
			d0 <= d;
			cc <= cc;
		
			case(seq)
				4'h0: q <= {         d,q[11:1]};
				4'h1: q <= {q[11],   d,q[10:1]};
				4'h2: q <= {q[11:10],d,q[9:1]};
				4'h3: q <= {q[11:9], d,q[8:1]};
				4'h4: q <= {q[11:8], d,q[7:1]};
				4'h5: q <= {q[11:7], d,q[6:1]};
				4'h6: q <= {q[11:6], d,q[5:1]};
				4'h7: q <= {q[11:5], d,q[4:1]};
				4'h8: q <= {q[11:4], d,q[3:1]};
				4'h9: q <= {q[11:3], d,q[2:1]};
				4'ha: q <= {q[11:2], d,q[1]};
				4'hb:
				begin
`ifdef GEN_SIN
					isin = 1023 * $sin(6.2832 *phs/256);
					q <= isin;
					//q <= 12'd0;
					phs <= 255 & (phs + 1);
`else
					q <= {q[11:1], d};
`endif
					cc <= 1'b0;
				end
			endcase
		end
	end
endmodule
					