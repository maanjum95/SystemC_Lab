// main.cpp

#include "consum_1.h"
#include "prod_1.h"
#include "fifo_1.h"

int sc_main(int argc, char *argv[]) {
	// the following instruction generates aclock signal with clock named "clock"
	// with a period of 100 ns, a duty cycle of 50%, and a falling edge after 50 ns
	sc_clock clk("clock", 100, SC_NS, 0.5, 50, SC_NS, false);

	// fill in the required commands to instantiate and connect
	// producer, fifo and cosumer

	// fill in code to generate traces that can be used to observe the
	// functionality of the model with the waveform viewer gtkwave

	sc_time sim_dur = sc_time(5000, SC_NS);
	if(argc != 2) {
		cout << "Default simulation time = " << sim_dur << endl;
	}
	else {
		sim_dur = sc_time(atoi(argv[1]), SC_NS);
	}

	// start simulation
	sc_start(sim_dur);

	// fill in code to close the trace file

	return 0;
}
