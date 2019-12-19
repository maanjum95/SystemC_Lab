#include "stimul.h"
#include "counter.h"
#include "bcd_decoder.h"

int sc_main(int argc, char *argv[]) {

	sc_signal<bool> clock, reset;
	sc_signal<unsigned short int> count_val;
	sc_signal<char> v_hi, v_lo;

	// fill in here

	int n_cycles;
	if(argc != 2) {
		cout << "default n_cycles = 200\n";
		n_cycles = 200;
	}
	else
		n_cycles = atoi(argv[1]);

	sc_start(n_cycles, SC_NS);

	sc_close_vcd_trace_file(tf);

	return 0;
}
