// consum_1.h

#include "systemc.h"

SC_MODULE(consum_1) {
public:
	// ports declaration
	// fill in here

	// internal signal, public to enable tracing
	sc_signal<bool> fetch;

	// internal signal; declared public to make it available to tracing
	sc_signal<bool> data_valid;

private:
	// member variable
	int consumed_data;

	// process declaration
	void fetch_trigger();
	void consumer();

public:
	// constructor
	SC_CTOR(consum_1) {

		// register processes
		SC_THREAD(fetch_trigger);
		SC_THREAD(consumer);
		sensitive << clk.pos();

		// initialization of port
		get.initialize(false);
		data_valid.write(false);
		consumed_data = 0;
	}
};
