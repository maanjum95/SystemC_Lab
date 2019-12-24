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
	sc_signal<int> dat1, dat2;
	sc_signal<bool> put, f_full, get, f_empty;

	prod_1 prod("producer");
	prod.clk(clk);
	prod.dat1(dat1);
	prod.put(put);
	prod.f_full(f_full);

	fifo_1 fifo("fifo");
	fifo.clk(clk);
	fifo.wr(put);
	fifo.data_in(dat1);
	fifo.rd(get);
	fifo.data_out(dat2);
	fifo.full(f_full);
	fifo.empty(f_empty);

	consum_1 consum("consumer");
	consum.clk(clk);
	consum.dat2(dat2);
	consum.get(get);
	consum.f_empty(f_empty);

	// fill in code to generate traces that can be used to observe the
	// functionality of the model with the waveform viewer gtkwave
	sc_trace_file *tf = sc_create_vcd_trace_file("ex_2");
	sc_trace(tf, clk, "clock");
	sc_trace(tf, dat1, "dat1");
	sc_trace(tf, dat2, "dat2");
	sc_trace(tf, put, "put");
	sc_trace(tf, f_full, "f_full");
	sc_trace(tf, get, "get");
	sc_trace(tf, f_empty, "f_empty");

	// additional signals
	// prod
	sc_trace(tf, prod.send, "send");

	// fifo
	sc_trace(tf, fifo.fill_level, "fill_level");

	// consum
	sc_trace(tf, consum.fetch, "fetch");
	sc_trace(tf, consum.data_valid, "data_valid");

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
	sc_close_vcd_trace_file(tf);

	return 0;
}
