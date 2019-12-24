#include "stimul.h"
#include "counter.h"
#include "bcd_decoder.h"

int sc_main(int argc, char *argv[]) {

	sc_signal<bool> clock, reset;
	sc_signal<unsigned short int> count_val;
	sc_signal<char> v_hi, v_lo;

	// the stimuli
	stimul stiml("stimuli");
	stiml.clk(clock);
	stiml.res(reset);

	// the counter
	counter cntr("counter");
	cntr.clk(clock);
	cntr.res(reset);
	cntr.cnt(count_val);

	// the bcd decoder
	bcd_decoder bd_dcr("bcd_decoder");
	bd_dcr.val(count_val);
	bd_dcr.hi(v_hi);
	bd_dcr.lo(v_lo);

	int n_cycles;
	if(argc != 2) {
		cout << "default n_cycles = 200\n";
		n_cycles = 200;
	}
	else
		n_cycles = atoi(argv[1]);


	// Initializing traces
	sc_trace_file *tf = sc_create_vcd_trace_file("ex_1b");
	sc_trace(tf, clock, "clock");
	sc_trace(tf, reset, "reset");
	sc_trace(tf, count_val, "count_val");
	sc_trace(tf, v_hi, "v_hi");
	sc_trace(tf, v_lo, "v_lo");

	// adding the extra trace of a variable
	sc_trace(tf, cntr.cnt_int, "cnt_int");

	sc_start(n_cycles, SC_NS);

	sc_close_vcd_trace_file(tf);

	return 0;
}
